/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
 *               2011-2013 by Michele Dolfi <dolfim@phys.ethz.ch>
 *                            Bela Bauer <bauerb@comp-phys.org>
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

#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/shared_ptr.hpp>

#include "dmrg/block_matrix/detail/alps.hpp"

#include "dmrg/models/factory.h"
#include "dmrg/models/measurements.h"
#include "dmrg/mp_tensors/mpo.h"

#include "dmrg/mp_tensors/mps.h"
#include "dmrg/optimize/ss_optimize.hpp"

#include "dmrg/utils/DmrgParameters.h"

#include "dmrg/sim/matrix_types.h"

typedef U1 symm;


int main(int argc, char ** argv)
{
    try {
        if (argc != 3)
        {
            maquis::cout << "Usage: <parms> <model_parms>" << std::endl;
            exit(1);
        }

        maquis::cout.precision(10);

        /// Loading parameters
        std::ifstream param_file(argv[1]);
        if (!param_file) {
            maquis::cerr << "Could not open parameter file." << std::endl;
            exit(1);
        }
        DmrgParameters parms(param_file);

        /// Loading model
        std::ifstream model_file(argv[2]);
        if (!model_file) {
            maquis::cerr << "Could not open model file." << std::endl;
            exit(1);
        }
        ModelParameters model_parms(model_file);


        /// Parsing model
        std::shared_ptr<Lattice> lattice;
        std::shared_ptr<Model<matrix, symm> > model;
        model_parser<matrix, symm>(parms["lattice_library"], parms["model_library"], model_parms,
                                        lattice, model);

        Hamiltonian<matrix, symm> H = model->H();
        MPO<matrix, symm> mpo = make_mpo(lattice->size(), H);


        /// Initialize MPS
        MPS<matrix, symm> mps(lattice->size(), parms["init_bond_dimension"],
                                   H.get_phys(), model->initc(model_parms),
                                   *(model->initializer(parms)));

        /// Initialize optimizer
        storage::setup(parms);

        ss_optimize<matrix, symm, storage::disk> optimizer(mps, mpo, parms);
        int sweeps = parms["nsweeps"];

        /// Optimize
        for (int sweep=0; sweep<parms["nsweeps"]; ++sweep) {
            int exit_site = optimizer.sweep(sweep, Both, -1 /* starting site */, -1 /* runlimit */);

            /// Write iteration results
            {
                std::stringstream iteration_path;
                iteration_path << "/simulation/sweep" << sweep;

                storage::archive ar(parms["resultfile"].str(), "w");

                ar[iteration_path.str() + "/parameters"] << parms;
                ar[iteration_path.str() + "/parameters"] << model_parms;

                ar[iteration_path.str() + "/results"] << storage::log;
            }
        }
        mps = optimizer.get_current_mps();


        /// Measurement on final MPS
        measure_on_mps(mps, *lattice, model->measurements(), parms["resultfile"]);

        /// Write parameters
        {
            storage::archive ar(parms["resultfile"].str(), "w");

            ar["/parameters"] << parms;
            ar["/parameters"] << model_parms;
        }

        /// Finalize worker thread
        storage::disk::sync();

    } catch (std::exception & e) {
        maquis::cerr << "Exception thrown!" << std::endl;
        maquis::cerr << e.what() << std::endl;
        exit(1);
    }
}

