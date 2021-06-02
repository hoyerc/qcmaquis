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
#include "dmrg/mp_tensors/mpo.h"
#include "dmrg/mp_tensors/super_mpo.h"
#include "dmrg/mp_tensors/mps_mpo_ops.h"
#include "dmrg/mp_tensors/coherent_init.h"
#include "dmrg/models/generate_mpo.hpp"
#include "dmrg/models/coded/lattice.hpp"

#include <boost/tuple/tuple.hpp>
#include <boost/math/special_functions/factorials.hpp>

typedef alps::numeric::matrix<double> matrix;


template <class SymmGroup>
std::vector<double> measure_local(MPS<matrix, SymmGroup> const& mps,
                                  typename operator_selector<matrix, SymmGroup>::type const& ident,
                                  typename operator_selector<matrix, SymmGroup>::type const& op)
{
    typedef typename operator_selector<matrix, SymmGroup>::type op_t;
    typedef std::vector<op_t> op_vec;
    std::shared_ptr<lattice_impl> lat_ptr(new ChainLattice(mps.length()));
    Lattice lattice(lat_ptr);

    std::vector<double> vals(mps.size());
    for (int p=0; p<mps.size(); ++p) {
        generate_mpo::MPOMaker<matrix, SymmGroup> mpom(lattice, op_vec(1,ident),op_vec(1,ident));
        generate_mpo::OperatorTerm<matrix, SymmGroup> term;
        term.operators.push_back( std::make_pair(p, op) );
        term.fill_operator = ident;
        mpom.add_term(term);
        MPO<matrix, SymmGroup> mpo = mpom.create_mpo();

        vals[p] = maquis::real(expval(mps, mpo));
    }
    return vals;
}


BOOST_AUTO_TEST_CASE( manual_superposition )
{
    std::cout << "=== RUNNING manual_superposition ===" << std::endl;

    typedef TrivialGroup SymmGroup;
    typedef SymmGroup::charge charge;
    typedef boost::tuple<charge, size_t, double> local_state;
    typedef operator_selector<matrix, SymmGroup>::type op_t;

    using std::exp; using std::sqrt; using std::pow;
    using boost::math::factorial;

    int L = 4;

    // Bosons with Nmax=2
    const int Nmax = 2;
    charge C = SymmGroup::IdentityCharge;
    Index<SymmGroup> phys;
    phys.insert(std::make_pair(C, 3));

    MPS<matrix,SymmGroup> mps(L);

    Index<SymmGroup> left_i, right_i;
    left_i.insert(std::make_pair(C,1));
    int p=0;
    {
        matrix m(Nmax+1, 2, 0.);
        m(0,0) = 1.;
        m(1,1) = 1.;
        block_matrix<matrix, SymmGroup> block;
        block.insert_block(m, C,C);

        right_i = block.right_basis();
        MPSTensor<matrix, SymmGroup> mt(phys, left_i, right_i, false, 0);
        mt.data() = block;

        mps[p] = mt;
        p++;
        std::swap(left_i, right_i);
    }
    {
        matrix m(6, 2, 0.);
        m(0,0) = 1.;
        m(1,1) = 1.;
        block_matrix<matrix, SymmGroup> block;
        block.insert_block(m, C,C);

        right_i = block.right_basis();
        MPSTensor<matrix, SymmGroup> mt(phys, left_i, right_i, false, 0);
        mt.data() = block;

        mps[p] = mt;
        p++;
        std::swap(left_i, right_i);
    }
    {
        matrix m(6, 2, 0.);
        m(0,0) = 1.;
        m(3,1) = 1.;
        block_matrix<matrix, SymmGroup> block;
        block.insert_block(m, C,C);

        right_i = block.right_basis();
        MPSTensor<matrix, SymmGroup> mt(phys, left_i, right_i, false, 0);
        mt.data() = block;

        mps[p] = mt;
        p++;
        std::swap(left_i, right_i);
    }
    {
        matrix m(6, 1, 0.);
        m(0,0) = 1.;
        m(1,0) = 1.;
        block_matrix<matrix, SymmGroup> block;
        block.insert_block(m, C,C);

        right_i = block.right_basis();
        MPSTensor<matrix, SymmGroup> mt(phys, left_i, right_i, false, 0);
        mt.data() = block;

        mps[p] = mt;
        p++;
        std::swap(left_i, right_i);
    }

    std::cout << "norm = " << norm(mps) << std::endl;
    mps.normalize_left();
    double nn = norm(mps);
    std::cout << "norm = " << nn << std::endl;

    /// operators for meas
    op_t ident = identity_matrix<op_t>(phys);
    op_t densop;
    {
        matrix tmp(Nmax+1, Nmax+1, 0.);
        for (int i=1; i<=Nmax; ++i) tmp(i,i) = i;
        densop.insert_block(tmp, C,C);
    }

    /// meas
    std::vector<double> meas_dens = measure_local(mps, ident, densop);
    for (int p=0; p<L; ++p) {
        maquis::cout << "site " << p << ": " << meas_dens[p]/nn << std::endl;
    }

    /// checking results
    BOOST_CHECK_CLOSE(0.5, meas_dens[0]/nn, 1e-8 );
    BOOST_CHECK_CLOSE(0.0, meas_dens[1]/nn, 1e-8 );
    BOOST_CHECK_CLOSE(0.5, meas_dens[2]/nn, 1e-8 );
    BOOST_CHECK_CLOSE(0.0, meas_dens[3]/nn, 1e-8 );
}

BOOST_AUTO_TEST_CASE( coherent_init_L2Nmax2 )
{
    std::cout << "=== RUNNING coherent_init_L2Nmax2 ===" << std::endl;

    typedef TrivialGroup SymmGroup;
    typedef SymmGroup::charge charge;
    typedef boost::tuple<charge, size_t, double> local_state;
    typedef operator_selector<matrix, SymmGroup>::type op_t;
    using std::sqrt;

    int L = 2;

    // Bosons with Nmax=2
    const int Nmax = 2;
    charge C = SymmGroup::IdentityCharge;
    Index<SymmGroup> phys;
    phys.insert(std::make_pair(C, Nmax+1));

    /// desired density
    std::vector<double> coeff(L);
    coeff[0] = sqrt(0.0075);
    coeff[1] = sqrt(0.0025);

    MPS<matrix,SymmGroup> mps = coherent_init<matrix>(coeff, phys);

    double nn = norm(mps);
    std::cout << "norm = " << nn << std::endl;

    /// operators for meas
    op_t ident = identity_matrix<op_t>(phys);
    op_t densop;
    {
        matrix tmp(Nmax+1, Nmax+1, 0.);
        for (int i=1; i<Nmax+1; ++i) tmp(i,i) = i;
        densop.insert_block(tmp, C,C);
    }

    /// meas
    std::vector<double> meas_dens = measure_local(mps, ident, densop);
    for (int p=0; p<L; ++p) {
        maquis::cout << "site " << p << ": " << meas_dens[p]/nn << std::endl;
    }
    for (int p=0; p<L; ++p) {
        BOOST_CHECK_CLOSE(coeff[p]*coeff[p], meas_dens[p]/nn, 1. );
    }
}


BOOST_AUTO_TEST_CASE( coherent_init_Nmax2 )
{
    std::cout << "=== RUNNING coherent_init_Nmax2 ===" << std::endl;

    typedef TrivialGroup SymmGroup;
    typedef SymmGroup::charge charge;
    typedef boost::tuple<charge, size_t, double> local_state;
    typedef operator_selector<matrix, SymmGroup>::type op_t;
    using std::sqrt;

    int L = 4;

    // Bosons with Nmax=2
    const int Nmax = 2;
    charge C = SymmGroup::IdentityCharge;
    Index<SymmGroup> phys;
    phys.insert(std::make_pair(C, Nmax+1));

    /// desired density
    std::vector<double> coeff(L);
    coeff[0] = sqrt(0.005);
    coeff[1] = sqrt(0.015);
    coeff[2] = sqrt(0.015);
    coeff[3] = sqrt(0.005);

    MPS<matrix,SymmGroup> mps = coherent_init<matrix>(coeff, phys);

    double nn = norm(mps);
    std::cout << "norm = " << nn << std::endl;

    /// operators for meas
    op_t ident = identity_matrix<op_t>(phys);
    op_t densop;
    {
        matrix tmp(Nmax+1, Nmax+1, 0.);
        for (int i=1; i<Nmax+1; ++i) tmp(i,i) = i;
        densop.insert_block(tmp, C,C);
    }

    /// meas
    std::vector<double> meas_dens = measure_local(mps, ident, densop);
    for (int p=0; p<L; ++p) {
        maquis::cout << "site " << p << ": " << meas_dens[p]/nn << std::endl;
    }
    for (int p=0; p<L; ++p) {
        BOOST_CHECK_CLOSE(coeff[p]*coeff[p], meas_dens[p]/nn, 1. );
    }
}

