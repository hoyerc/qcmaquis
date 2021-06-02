/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
 *               2011-2013 by Michele Dolfi <dolfim@phys.ethz.ch>
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

#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include <cmath>
#include <iterator>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;

#include "dmrg/block_matrix/detail/alps.hpp"

#include "dmrg/mp_tensors/mps.h"
#include "dmrg/mp_tensors/state_mps.h"
#include "dmrg/mp_tensors/mps_initializers.h"
#include "dmrg/mp_tensors/coherent_init.h"
#include "dmrg/mp_tensors/mpo.h"
#include "dmrg/mp_tensors/super_mpo.h"
#include "dmrg/mp_tensors/mps_mpo_ops.h"
#include "dmrg/models/generate_mpo.hpp"
#include "dmrg/models/coded/lattice.hpp"

#include <boost/math/special_functions/factorials.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

typedef alps::numeric::matrix<double> matrix;
typedef TrivialGroup SymmGroup;
typedef SymmGroup::charge charge;
typedef boost::tuple<charge, size_t> local_state;


std::vector<double> measure_local(MPS<matrix, SymmGroup> const& mps,
                                  operator_selector<matrix, SymmGroup>::type const& ident,
                                  operator_selector<matrix, SymmGroup>::type const& op)
{
    typedef operator_selector<matrix, SymmGroup>::type op_t;
    typedef std::vector<op_t> op_vec;
    std::shared_ptr<lattice_impl> lat_ptr(new ChainLattice(mps.length()));
    Lattice lattice(lat_ptr);

    std::vector<double> vals(mps.size());
    for (int p=0; p<mps.size(); ++p) {
    	generate_mpo::MPOMaker<matrix, SymmGroup> mpom(lattice, op_vec(1,ident), op_vec(1,ident));
        generate_mpo::OperatorTerm<matrix, SymmGroup> term;
        term.operators.push_back( std::make_pair(p, op) );
        term.fill_operator = ident;
        mpom.add_term(term);
        MPO<matrix, SymmGroup> mpo = mpom.create_mpo();
        MPS<matrix, SymmGroup> super_mpo = mpo_to_smps(mpo, ident.left_basis());

        vals[p] = overlap(super_mpo, mps);
    }
    return vals;
}

std::vector<double> measure_local_multi(MPS<matrix, SymmGroup> const& mps,
                                        operator_selector<matrix, SymmGroup>::type const& ident,
                                        operator_selector<matrix, SymmGroup>::type const& op)
{
    typedef operator_selector<matrix, SymmGroup>::type op_t;
    typedef std::vector<op_t> op_vec;
    std::vector<std::pair<op_vec, bool> > ops(1, std::make_pair(op_vec(1,op), false));

    std::shared_ptr<lattice_impl> lat_ptr(new ChainLattice(mps.length()));
    Lattice lattice(lat_ptr);
    generate_mpo::CorrMaker<matrix, SymmGroup> dcorr(lattice, op_vec(1,ident), op_vec(1,ident), ops, -1);

    MPO<matrix, SymmGroup> mpo = dcorr.create_mpo();

    Index<SymmGroup> phys_i = ident.left_basis();
    double nn = dm_trace(mps, phys_i);
    MPS<matrix, SymmGroup> super_mpo = mpo_to_smps(mpo, phys_i);
    std::vector<double> vals = multi_overlap(super_mpo, mps);
    std::reverse(vals.begin(), vals.end());
    return vals;
}


BOOST_AUTO_TEST_CASE( density_trivial_init )
{
    typedef operator_selector<matrix, SymmGroup>::type op_t;
    maquis::cout << std::endl << "** TESTING density_trivial_init **" << std::endl;
    int L = 7;

    /// Bosons with Nmax=2
    const int Nmax = 2;
    SymmGroup::charge C = SymmGroup::IdentityCharge;
    Index<SymmGroup> phys_psi;
    phys_psi.insert(std::make_pair(C, Nmax+1));
    Index<SymmGroup> phys_rho = phys_psi * adjoin(phys_psi);

    /*
     * phys_rho = [
     *              0 --> 0,
     *              0 --> 1,
     *              0 --> 2,
     *              1 --> 0,
     *              1 --> 1,
     *              1 --> 2,
     *              2 --> 0,
     *              2 --> 1,
     *              2 --> 2,
     *            ]
     */

    /// building state
    std::vector<local_state> state(L);
    state[0] = local_state(C, 0);
    state[1] = local_state(C, 4);
    state[2] = local_state(C, 4);
    state[3] = local_state(C, 8);
    state[4] = local_state(C, 4);
    state[5] = local_state(C, 4);
    state[6] = local_state(C, 0);

    /// corresponding density
    std::vector<double> dens(L, 0.);
    dens[1] = 1.;
    dens[2] = 1.;
    dens[3] = 2.;
    dens[4] = 1.;
    dens[5] = 1.;

    MPS<matrix,SymmGroup> mps = state_mps<matrix>(state, std::vector<Index<SymmGroup> >(1,phys_rho), std::vector<int>(L,0));
    double nn = dm_trace(mps, phys_psi);
    std::cout << "norm = " << nn << std::endl;

    /// operators for meas
    op_t ident = identity_matrix<op_t>(phys_psi);
    op_t densop;
    {
        matrix tmp(Nmax+1, Nmax+1, 0.);
        for (int i=1; i<Nmax+1; ++i) tmp(i,i) = i;
        densop.insert_block(tmp, C,C);
    }

    /// meas
    std::vector<double> meas_dens = measure_local(mps, ident, densop);
    std::vector<double> meas_dens_multi = measure_local_multi(mps, ident, densop);
    for (int p=0; p<L; ++p) {
        maquis::cout << "site " << p << ": " << meas_dens[p]/nn << "  " << meas_dens_multi[p]/nn << std::endl;
        BOOST_CHECK_CLOSE(dens[p], meas_dens[p]/nn, 1e-8 );
        BOOST_CHECK_CLOSE(dens[p], meas_dens_multi[p]/nn, 1e-8 );
    }
}

BOOST_AUTO_TEST_CASE( density_join_init )
{
    typedef operator_selector<matrix, SymmGroup>::type op_t;
    maquis::cout << std::endl << "** TESTING density_join_init **" << std::endl;
    int L = 7;

    // Bosons with Nmax=2
    const int Nmax = 2;
    SymmGroup::charge C = SymmGroup::IdentityCharge;
    Index<SymmGroup> phys_psi;
    phys_psi.insert(std::make_pair(C, Nmax+1));
    Index<SymmGroup> phys_rho = phys_psi * adjoin(phys_psi);

    MPS<matrix,SymmGroup> mps;
    {
    std::vector<local_state> state(L);
    state[0] = local_state(C, 4);
    state[1] = local_state(C, 4);
    state[2] = local_state(C, 4);
    state[3] = local_state(C, 4);
    state[4] = local_state(C, 4);
    state[5] = local_state(C, 4);
    state[6] = local_state(C, 4);

    mps = state_mps<matrix>(state, std::vector<Index<SymmGroup> >(1,phys_rho), std::vector<int>(L,0));
    }

    {
        std::vector<local_state> state(L);
        state[0] = boost::make_tuple(C, 0);
        state[1] = boost::make_tuple(C, 0);
        state[2] = boost::make_tuple(C, 4);
        state[3] = boost::make_tuple(C, 4);
        state[4] = boost::make_tuple(C, 4);
        state[5] = boost::make_tuple(C, 0);
        state[6] = boost::make_tuple(C, 0);

        MPS<matrix,SymmGroup> tmp = state_mps<matrix>(state, std::vector<Index<SymmGroup> >(1,phys_rho), std::vector<int>(L,0));
        mps = join(tmp, mps);
    }

    double nn = dm_trace(mps, phys_psi);
    std::cout << "norm = " << nn << std::endl;

    /// operators for meas
    op_t ident = identity_matrix<op_t>(phys_psi);
    op_t densop;
    {
        matrix tmp(Nmax+1, Nmax+1, 0.);
        for (int i=1; i<Nmax+1; ++i) tmp(i,i) = i;
        densop.insert_block(tmp, C,C);
    }

    /// meas
    std::vector<double> meas_dens = measure_local(mps, ident, densop);
    std::vector<double> meas_dens_multi = measure_local_multi(mps, ident, densop);
    for (int p=0; p<L; ++p) {
        maquis::cout << "site " << p << ": " << meas_dens[p]/nn << "  " << meas_dens_multi[p]/nn << std::endl;
        BOOST_CHECK_CLOSE(meas_dens[p], meas_dens_multi[p], 1e-8 );
    }
}

BOOST_AUTO_TEST_CASE( density_coherent_init )
{
    typedef operator_selector<matrix, SymmGroup>::type op_t;
    maquis::cout << std::endl << "** TESTING density_coherent_init **" << std::endl;

    using std::sqrt;

    int L = 2;
    const int Nmax = 2;

    SymmGroup::charge C = SymmGroup::IdentityCharge;
    Index<SymmGroup> phys_psi;
    phys_psi.insert(std::make_pair(C, Nmax+1));
    Index<SymmGroup> phys_rho = phys_psi * adjoin(phys_psi);

    /// desired density
    std::vector<double> coeff(L, 0.);
    coeff[0] = sqrt(0.0075);
    coeff[1] = sqrt(0.0025);

    /// build coherent init MPS
    MPS<matrix,SymmGroup> mps = coherent_init_dm<matrix>(coeff, phys_psi, phys_rho);

    double nn = dm_trace(mps, phys_psi);
    std::cout << "norm = " << nn << std::endl;

    /// operators for meas
    op_t ident = identity_matrix<op_t>(phys_psi);
    op_t densop;
    {
        matrix tmp(Nmax+1, Nmax+1, 0.);
        for (int i=1; i<Nmax+1; ++i) tmp(i,i) = i;
        densop.insert_block(tmp, C,C);
    }

    /// meas
    std::vector<double> meas_dens = measure_local(mps, ident, densop);
    std::vector<double> meas_dens_multi = measure_local_multi(mps, ident, densop);
    for (int p=0; p<L; ++p) {
        maquis::cout << "site " << p << ": " << meas_dens[p]/nn << "  " << meas_dens_multi[p]/nn << std::endl;
        BOOST_CHECK_CLOSE(meas_dens[p], meas_dens_multi[p], 1e-8 );
        BOOST_CHECK_CLOSE(coeff[p]*coeff[p], meas_dens[p]/nn, 5. );
        BOOST_CHECK_CLOSE(coeff[p]*coeff[p], meas_dens_multi[p]/nn, 5. );
    }
}


BOOST_AUTO_TEST_CASE( density_random_init )
{
    typedef operator_selector<matrix, SymmGroup>::type op_t;
    maquis::cout << std::endl << "** TESTING density_random_init **" << std::endl;

    int L = 6;
    int M = 20;

    DmrgParameters parms;
    parms.set("max_bond_dimension", M);

    // Bosons with Nmax=2
    const int Nmax = 2;
    SymmGroup::charge C = SymmGroup::IdentityCharge;
    Index<SymmGroup> phys_psi;
    phys_psi.insert(std::make_pair(C, 3));
    Index<SymmGroup> phys_rho = phys_psi * adjoin(phys_psi);

    /// building random state
    default_mps_init<matrix, SymmGroup> initializer(parms, std::vector<Index<SymmGroup> >(1, phys_rho), C, std::vector<int>(L,0));

    MPS<matrix,SymmGroup> mps;
    mps.resize(L); initializer(mps);

    double nn = dm_trace(mps, phys_psi);
    std::cout << "norm = " << nn << std::endl;

    /// operators for meas
    op_t ident = identity_matrix<op_t>(phys_psi);
    op_t densop;
    {
        matrix tmp(Nmax+1, Nmax+1, 0.);
        for (int i=1; i<Nmax+1; ++i) tmp(i,i) = i;
        densop.insert_block(tmp, C,C);
    }

    /// meas
    std::vector<double> meas_dens = measure_local(mps, ident, densop);
    std::vector<double> meas_dens_multi = measure_local_multi(mps, ident, densop);
    for (int p=0; p<L; ++p) {
        maquis::cout << "site " << p << ": " << meas_dens[p]/nn << "  " << meas_dens_multi[p]/nn << std::endl;
        BOOST_CHECK_CLOSE(meas_dens[p], meas_dens_multi[p], 1e-8 );
    }
}



