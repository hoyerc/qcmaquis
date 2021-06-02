/*****************************************************************************
*
* ALPS MPS DMRG Project
*
* Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
*               2011-2011 by Bela Bauer <bauerb@phys.ethz.ch>
*               2011-2013    Michele Dolfi <dolfim@phys.ethz.ch>
*               2014-2014    Sebastian Keller <sebkelle@phys.ethz.ch>
*               2018         Leon Freitag <lefreita@ethz.ch>
*
* This software is part of the ALPS Applications, published under the ALPS
* Application License; you can use, redistribute it and/or modify it under
* the terms of the license, either version 1 or (at your option) any later
* version.
*
* You should have received a copy of the ALPS Application License along with
* the ALPS Applications; see the file LICENSE.txt. If not, the license is also
* available from http://alps.comp-phys.org/.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/
#ifndef INTERFACE_SIM_H
#define INTERFACE_SIM_H
#include <cmath>
#include <iterator>
#include <iostream>
#include <sys/stat.h>

#include "dmrg/sim/sim.h"
#include "dmrg/optimize/optimize.h"
#include "dmrg/models/chem/measure_transform.hpp"
#include "integral_interface.h"
#include "dmrg/utils/results_collector.h"


// The sim class for interface-based DMRG runs and measurements
template <class Matrix, class SymmGroup>
class interface_sim : public sim<Matrix, SymmGroup>, public abstract_interface_sim<Matrix> {

    typedef sim<Matrix, SymmGroup> base;
    typedef abstract_interface_sim<Matrix> interface_base;
    typedef optimizer_base<Matrix, SymmGroup, storage::disk> opt_base_t;
    typedef typename base::status_type status_type;
    typedef typename base::measurements_type measurements_type;
    typedef typename interface_base::meas_with_results_type meas_with_results_type;
    typedef typename interface_base::results_map_type results_map_type;

    using base::mps;
    using base::mpo;
    using base::parms;
    using base::all_measurements;
    using base::sweep_measurements;
    using base::stop_callback;
    using base::init_sweep;
    using base::init_site;
    using base::rfile;
    using base::lat;
    using base::model;

public:

    interface_sim (DmrgParameters & parms_)
    : base(parms_), last_sweep_(init_sweep-1)
    { }

    void run()
    {
        optimize();
        // or whatever
        // throw std::runtime_error("run() shouldn't be called from interface_sim");
    }

    void optimize()
    {
        int meas_each = parms["measure_each"];
        int chkp_each = parms["chkp_each"];

        /// MPO creation
        if (parms["MODEL"] == std::string("quantum_chemistry") && parms["use_compressed"])
            throw std::runtime_error("chem compression has been disabled");
        MPO<Matrix, SymmGroup> mpoc = mpo;
        if (parms["use_compressed"])
            mpoc.compress(1e-12);

        /// Optimizer initialization
        std::shared_ptr<opt_base_t> optimizer;
        if (parms["optimization"] == "singlesite")
        {
            optimizer.reset( new ss_optimize<Matrix, SymmGroup, storage::disk>
                            (mps, mpoc, parms, stop_callback, init_site) );
        }
        else if(parms["optimization"] == "twosite")
        {
            optimizer.reset( new ts_optimize<Matrix, SymmGroup, storage::disk>
                            (mps, mpoc, parms, stop_callback, init_site) );
        }
        else {
            throw std::runtime_error("Don't know this optimizer");
        }

        measurements_type always_measurements = this->iteration_measurements(init_sweep);

        try {
            for (int sweep=init_sweep; sweep < parms["nsweeps"]; ++sweep) {
                // TODO: introduce some timings

                optimizer->sweep(sweep, Both);
                storage::disk::sync();

                bool converged = false;

                if ((sweep+1) % meas_each == 0 || (sweep+1) == parms["nsweeps"])
                {
                    iteration_results_ = optimizer->iteration_results();

                    /// write iteration results if result files are specified
                    if (!rfile().empty())
                    {
                        storage::archive ar(rfile(), "w");
                        ar[results_archive_path(sweep) + "/parameters"] << parms;
                        ar[results_archive_path(sweep) + "/results"] << iteration_results_;

                        // ar[results_archive_path(sweep) + "/results/Runtime/mean/value"] << std::vector<double>(1, elapsed_sweep + elapsed_measure);

                        // stop simulation if an energy threshold has been specified
                        // FIXME: this does not work for complex numbers - stknecht feb 2016
                        // FIXME 2: this reads the previous energies from the result file and so does not work if
                        // results/checkpoints are not specified. The minimum energy should be stored separately and initialised/read
                        // at the beginning of the simulation instead -- Leon
                        int prev_sweep = sweep - meas_each;
                        if (prev_sweep >= 0 && parms["conv_thresh"] > 0.)
                        {
                            typedef typename maquis::traits::real_type<Matrix>::type real_type;
                            std::vector<real_type> energies;

                            ar[results_archive_path(sweep) + "/results/Energy/mean/value"] >> energies;
                            real_type emin = *std::min_element(energies.begin(), energies.end());
                            ar[results_archive_path(prev_sweep) + "/results/Energy/mean/value"] >> energies;
                            real_type emin_prev = *std::min_element(energies.begin(), energies.end());
                            real_type e_diff = std::abs(emin - emin_prev);

                            if (e_diff < parms["conv_thresh"])
                                converged = true;
                        }

                        /// measure observables specified in 'always_measure'
                        if (always_measurements.size() > 0)
                            this->measure(this->results_archive_path(sweep) + "/results/", always_measurements);
                    }
                }
                last_sweep_ = sweep;

                /// write checkpoint
                bool stopped = stop_callback() || converged;
                if (stopped || (sweep+1) % chkp_each == 0 || (sweep+1) == parms["nsweeps"])
                    checkpoint_simulation(mps, sweep, -1);

                if (stopped) break;
            }
        } catch (dmrg::time_limit const& e) {
            maquis::cout << e.what() << " checkpointing partial result." << std::endl;
            checkpoint_simulation(mps, e.sweep(), e.site());

            {
                iteration_results_ = optimizer->iteration_results();
                if (!rfile().empty())
                {
                    storage::archive ar(rfile(), "w");
                    ar[results_archive_path(e.sweep()) + "/parameters"] << parms;
                    ar[results_archive_path(e.sweep()) + "/results"] << iteration_results_;

                    // ar[results_archive_path(e.sweep()) + "/results/Runtime/mean/value"] << std::vector<double>(1, elapsed_sweep + elapsed_measure);
                }
            }
        }
    }

    void run_measure()
    {
        if (this->get_last_sweep() < 0)
            throw std::runtime_error("Tried to measure before a sweep");
        this->measure("/spectrum/results/", all_measurements);

        /// MPO creation
        MPO<Matrix, SymmGroup> mpoc = mpo;
        if (parms["use_compressed"])
            mpoc.compress(1e-12);

        double energy;

        if (parms["MEASURE[Energy]"]) {
            energy = maquis::real(expval(mps, mpoc)) + maquis::real(mpoc.getCoreEnergy());
            maquis::cout << "Energy: " << energy << std::endl;

            if (!rfile().empty())
            {
                storage::archive ar(rfile(), "w");
                ar["/spectrum/results/Energy/mean/value"] << std::vector<double>(1, energy);
            }
        }

        if (parms["MEASURE[EnergyVariance]"] > 0) {
            MPO<Matrix, SymmGroup> mpo2 = square_mpo(mpoc);
            mpo2.compress(1e-12);

            if (!parms["MEASURE[Energy]"]) energy = maquis::real(expval(mps, mpoc)) + maquis::real(mpoc.getCoreEnergy());
            double energy2 = maquis::real(expval(mps, mpo2, true));

            maquis::cout << "Energy^2: " << energy2 << std::endl;
            maquis::cout << "Variance: " << energy2 - energy*energy << std::endl;

            if (!rfile().empty())
            {
                storage::archive ar(rfile(), "w");
                ar["/spectrum/results/Energy^2/mean/value"] << std::vector<double>(1, energy2);
                ar["/spectrum/results/EnergyVariance/mean/value"] << std::vector<double>(1, energy2 - energy*energy);
            }
        }

        #if defined(HAVE_TwoU1) || defined(HAVE_TwoU1PG)
        if (!rfile().empty())
        {
            BaseParameters parms_meas;
            parms_meas = parms.twou1_measurements();
            if (!parms_meas.empty())
                measure_transform<Matrix, SymmGroup>()(rfile(), "/spectrum/results", base::lat, mps, parms_meas);
        }
        else
            throw std::runtime_error("Transformed measurements not implemented yet without checkpoints");
        #endif
    }

    results_map_type measure_out()
    {
        results_map_type ret;

        // Do not measure before a sweep
        if (this->get_last_sweep() < 0)
            throw std::runtime_error("Tried to measure before a sweep");

        // Run all measurements and fill the result map
        for(auto&& meas: all_measurements)
                ret[meas.name()] = measure_and_save<Matrix,SymmGroup>(rfile(), "/spectrum/results", mps).meas_out(meas);

        // Measurements that require SU2U1->2U1 transformation
        #if defined(HAVE_TwoU1) || defined(HAVE_TwoU1PG)
            BaseParameters parms_meas;
            parms_meas = parms.twou1_measurements();

            if (!parms_meas.empty())
            {
                // Obtain a map with transformed measurements
                results_map_type transformed_meas = measure_transform<Matrix, SymmGroup>().meas_out(base::lat, mps, parms_meas, rfile(), "/spectrum/results");

                // Merge transformed measurements with the remaining results
                ret.insert(transformed_meas.begin(), transformed_meas.end());
            }
        #endif

        return ret;
    }

    typename Matrix::value_type get_energy()
    {
        return expval(mps, mpo) + mpo.getCoreEnergy();
    }

    void update_integrals(const chem::integral_map<typename Matrix::value_type> & integrals)
    {
        if (parms.is_set("integral_file")||parms.is_set("integrals"))
            throw std::runtime_error("updating integrals in the interface not supported yet in the FCIDUMP format");
        parms.set("integrals_binary", chem::serialize(integrals));

        //std::cout << " parms are set (and ints are updated) -> " << std::endl;
        //std::cout << parms << std::endl;

        // construct new model and mpo with new integrals
        // hope this doesn't give any memory leaks
        model = Model<Matrix, SymmGroup>(lat, parms);
        mpo = make_mpo(lat, model);

        // check if MPS is still OK
        maquis::checks::right_end_check(mps, model.total_quantum_numbers(parms));

        all_measurements = model.measurements();
        all_measurements << overlap_measurements<Matrix, SymmGroup>(parms);

    }

    results_collector& get_iteration_results()
    {
        // If iteration_results is empty, we didn't perform the sweep yet, but possibly loaded the MPS from a checkpoint
        // so we need to load also iteration results
        if (iteration_results_.empty())
        {
            // If we are not loading from a checkpoint, last_sweep is set to -1
            // so we need to return an empty iteration_results vector
            if (get_last_sweep() < 0)
                return iteration_results_;

            // otherwise, we are restarting but there's something wrong with the checkpoint
            if (!rfile().empty())
            {
                try // Load the iteration results from the last sweep
                {
                    storage::archive ar(rfile(), "r");
                    ar[results_archive_path(last_sweep_) + "/results"] >> iteration_results_;
                }
                catch (std::exception& e)
                {
                    maquis::cerr << e.what() << std::endl;
                    throw std::runtime_error("Error reading iteration results from checkpoint.");
                }
            }
            else
                throw std::runtime_error("No result file specified for restart -- cannot read iteration results!");

        }

        return iteration_results_;
    }

    virtual typename Matrix::value_type get_overlap(const std::string & aux_filename)
    {
        maquis::checks::symmetry_check(parms, aux_filename);

        MPS<Matrix, SymmGroup> aux_mps;
        load(aux_filename, aux_mps);

        return overlap(aux_mps, this->mps);
    }

    int get_last_sweep() { return last_sweep_; };

    ~interface_sim()
    {
        storage::disk::sync();
    }

private:

    results_collector iteration_results_;
    int last_sweep_;

    std::string results_archive_path(int sweep) const
    {
        status_type status;
        status["sweep"] = sweep;
        return base::results_archive_path(status);
    }

    void checkpoint_simulation(MPS<Matrix, SymmGroup> const& state, int sweep, int site)
    {
        status_type status;
        status["sweep"] = sweep;
        status["site"]  = site;
        return base::checkpoint_simulation(state, status);
    }


};

#endif
