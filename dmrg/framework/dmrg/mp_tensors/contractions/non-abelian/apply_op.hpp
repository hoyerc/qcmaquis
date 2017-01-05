/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
 *                    Laboratory for Physical Chemistry, ETH Zurich
 *               2014-2014 by Sebastian Keller <sebkelle@phys.ethz.ch>
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

#ifndef CONTRACTIONS_SU2_APPLY_OP_HPP
#define CONTRACTIONS_SU2_APPLY_OP_HPP

#include "dmrg/block_matrix/symmetry/gsl_coupling.h"
#include "dmrg/block_matrix/block_matrix.h"
#include "dmrg/mp_tensors/mpstensor.h"
#include "dmrg/mp_tensors/mpotensor.h"
#include "dmrg/mp_tensors/contractions/non-abelian/functors.h"
#include "dmrg/mp_tensors/contractions/non-abelian/micro_kernels.hpp"

#include "dmrg/mp_tensors/contractions/non-abelian/gemm.hpp"

namespace contraction {
namespace SU2 {

    template<class Matrix, class OtherMatrix, class SymmGroup>
    void lbtm_kernel(size_t b2,
                     ContractionGrid<Matrix, SymmGroup>& contr_grid,
                     Boundary<OtherMatrix, SymmGroup> const & left,
                     BoundaryMPSProduct<Matrix, OtherMatrix, SymmGroup, ::SU2::SU2Gemms> const & left_mult_mps,
                     MPOTensor<Matrix, SymmGroup> const & mpo,
                     DualIndex<SymmGroup> const & ket_basis,
                     Index<SymmGroup> const & right_i,
                     Index<SymmGroup> const & out_left_i,
                     ProductBasis<SymmGroup> const & in_right_pb,
                     ProductBasis<SymmGroup> const & out_left_pb)
    {
        typedef typename MPOTensor<OtherMatrix, SymmGroup>::index_type index_type;
        typedef typename MPOTensor<OtherMatrix, SymmGroup>::row_proxy row_proxy;
        typedef typename MPOTensor<OtherMatrix, SymmGroup>::col_proxy col_proxy;
        typedef typename DualIndex<SymmGroup>::const_iterator const_iterator;
        typedef typename SymmGroup::charge charge;

        col_proxy col_b2 = mpo.column(b2);
        for (typename col_proxy::const_iterator col_it = col_b2.begin(); col_it != col_b2.end(); ++col_it) {
            index_type b1 = col_it.index();

            block_matrix<Matrix, SymmGroup> local;
            block_matrix<Matrix, SymmGroup> const & T = left_mult_mps.at(b1, local);

            MPOTensor_detail::term_descriptor<Matrix, SymmGroup, true> access = mpo.at(b1,b2);

        for (std::size_t op_index = 0; op_index < access.size(); ++op_index)
        {
            typename operator_selector<Matrix, SymmGroup>::type const & W = access.op(op_index);
            block_matrix<Matrix, SymmGroup>& ret = contr_grid(b1,b2);

            int a = mpo.left_spin(b1).get(), k = W.spin().get(), ap = mpo.right_spin(b2).get();

            for (size_t t_block = 0; t_block < T.n_blocks(); ++t_block) {

                charge lc = T.basis().left_charge(t_block);
                charge rc = T.basis().right_charge(t_block);

                const_iterator it = ket_basis.left_lower_bound(rc); //ket_basis comes transposed!
                charge mc = it->rc;

                for (size_t w_block = 0; w_block < W.basis().size(); ++w_block)
                {
                    charge phys_in = W.basis().left_charge(w_block);
                    charge phys_out = W.basis().right_charge(w_block);

                    charge out_r_charge = SymmGroup::fuse(rc, phys_in);
                    size_t rb = right_i.position(out_r_charge);
                    if (rb == right_i.size()) continue;

                    charge out_l_charge = SymmGroup::fuse(lc, phys_out);
                    if (!::SU2::triangle(SymmGroup::spin(out_r_charge), ap, SymmGroup::spin(out_l_charge))) continue;
                    if (!right_i.has(out_l_charge)) continue; // can also probe out_left_i, but right_i has the same charges

                    size_t r_size = right_i[rb].second;

                    size_t o = ret.find_block(out_l_charge, out_r_charge);
                    if ( o == ret.n_blocks() )
                        o = ret.insert_block(Matrix(out_left_i.size_of_block(out_l_charge),r_size), out_l_charge, out_r_charge);

                    int i = SymmGroup::spin(lc), ip = SymmGroup::spin(out_l_charge);
                    int j = SymmGroup::spin(mc), jp = SymmGroup::spin(out_r_charge);
                    int two_sp = std::abs(i - ip), two_s  = std::abs(j - jp);

                    typename Matrix::value_type couplings[4];
                    ::SU2::set_coupling(j, two_s, jp, a,k,ap, i, two_sp, ip, access.scale(op_index), couplings);

                    size_t in_right_offset = in_right_pb(phys_in, out_r_charge);
                    size_t out_left_offset = out_left_pb(phys_out, lc);
                    size_t l_size = T.basis().left_size(t_block);

                    detail::lbtm<Matrix, SymmGroup>(T[t_block], ret[o], W, in_right_offset, out_left_offset, l_size, r_size, w_block, couplings);
                } // wblock
            } // lblock
        } // op_index
        } // b1
    }

    template <typename T>
    struct micro_task
    {
        typedef unsigned I;
        typedef unsigned short IS;

        micro_task(I b_, char oi, char wb, IS a, IS b, IS c, IS d, IS e, IS f, IS g, T s[])
        : b(b_), op_index(oi), w_block(wb), t_block(a), in_left_offset(b), in_right_offset(c), out_offset(d), l_size(e), r_size_cache(f), r_size(g)
        {
            scale[0] = s[0];
            scale[1] = s[1];
            scale[2] = s[2];
            scale[3] = s[3];
        }

        I b;
        IS t_block;
        IS in_left_offset;
        IS in_right_offset;
        IS out_offset;
        IS l_size;
        IS r_size_cache;
        IS r_size;
        char op_index;
        char w_block;
        T scale[4];
    };

    template <typename T>
    struct micro_task2
    {
        typedef unsigned short IS;

        micro_task2(T const* s, IS a, IS b, IS c, IS d, IS e) : source(s), l_size(a), r_size_cache(b), r_size(c), stripe(d), out_offset(e), size(0) {}
        
        T const* source;
        T scale[10];
        IS l_size, r_size_cache, r_size, stripe, out_offset;
        char ss1[10];
        char ss2[10];
        unsigned short size;
    };

    template <typename T>
    struct task_compare
    {
        bool operator ()(micro_task<T> const & t1, micro_task<T> const & t2)
        {
            return t1.out_offset < t2.out_offset;
        }
    };

    template <class Matrix, class SymmGroup>
    void op_iterate(typename operator_selector<Matrix, SymmGroup>::type const & W, std::size_t w_block, typename Matrix::value_type couplings[],
                    micro_task2<typename Matrix::value_type> & task)
    {
        typedef typename SparseOperator<Matrix, SymmGroup>::const_iterator block_iterator;
        std::pair<block_iterator, block_iterator> blocks = W.get_sparse().block(w_block);
        for( block_iterator it = blocks.first; it != blocks.second; ++it)
        {
            std::size_t index = it - blocks.first;
            std::size_t ss1 = it->row;
            std::size_t ss2 = it->col;
            std::size_t rspin = it->row_spin;
            std::size_t cspin = it->col_spin;
            std::size_t casenr = 0;
            if (rspin == 2 && cspin == 2) casenr = 3;
            else if (rspin == 2) casenr = 1;
            else if (cspin == 2) casenr = 2;

            typename Matrix::value_type alfa_t = it->coefficient * couplings[casenr];
            task.ss1[index] = ss1;
            task.ss2[index] = ss2;
            task.scale[index] = alfa_t;
            task.size++;
        }
        if (task.size > 10)
            throw std::runtime_error("too many nano ops: " + boost::lexical_cast<std::string>(task.size) + "\n");
    }

    template<class Matrix, class OtherMatrix, class SymmGroup>
    void rbtm_kernel(size_t b1,
                block_matrix<Matrix, SymmGroup> & ret,
                Boundary<OtherMatrix, SymmGroup> const & right,
                MPSBoundaryProduct<Matrix, OtherMatrix, SymmGroup, ::SU2::SU2Gemms> const & right_mult_mps,
                MPOTensor<Matrix, SymmGroup> const & mpo,
                DualIndex<SymmGroup> const & ket_basis,
                Index<SymmGroup> const & left_i,
                Index<SymmGroup> const & out_right_i,
                ProductBasis<SymmGroup> const & in_left_pb,
                ProductBasis<SymmGroup> const & out_right_pb)
    {
        typedef typename MPOTensor<OtherMatrix, SymmGroup>::index_type index_type;
        typedef typename MPOTensor<OtherMatrix, SymmGroup>::row_proxy row_proxy;
        typedef typename MPOTensor<OtherMatrix, SymmGroup>::col_proxy col_proxy;
        typedef typename DualIndex<SymmGroup>::const_iterator const_iterator;
        typedef typename SymmGroup::charge charge;
        typedef typename Matrix::value_type value_type;

        std::map<value_type*, std::vector<micro_task<value_type> > > tasks;
        std::map<value_type*, std::vector<micro_task2<value_type> > > tasks2;

        row_proxy row_b1 = mpo.row(b1);
        for (typename row_proxy::const_iterator row_it = row_b1.begin(); row_it != row_b1.end(); ++row_it) {
            index_type b2 = row_it.index();

            block_matrix<Matrix, SymmGroup> const & T = right_mult_mps.at(b2);

            MPOTensor_detail::term_descriptor<Matrix, SymmGroup, true> access = mpo.at(b1,b2);

            for (size_t op_index = 0; op_index < access.size(); ++op_index)
            {
                typename operator_selector<Matrix, SymmGroup>::type const & W = access.op(op_index);
                int a = mpo.left_spin(b1).get(), k = W.spin().get(), ap = mpo.right_spin(b2).get();

                for (size_t t_block = 0; t_block < T.n_blocks(); ++t_block){

                    charge lc = T.basis().left_charge(t_block);
                    charge rc = T.basis().right_charge(t_block);

                    const_iterator it = ket_basis.left_lower_bound(lc);
                    charge mc = it->rc;

                    for (size_t w_block = 0; w_block < W.basis().size(); ++w_block)
                    {   
                        charge phys_in = W.basis().left_charge(w_block);
                        charge phys_out = W.basis().right_charge(w_block);

                        charge out_l_charge = SymmGroup::fuse(lc, -phys_in);
                        size_t lb = left_i.position(out_l_charge);
                        if (lb == left_i.size()) continue;

                        charge out_r_charge = SymmGroup::fuse(rc, -phys_out);
                        if (!::SU2::triangle(SymmGroup::spin(out_l_charge), a, SymmGroup::spin(out_r_charge))) continue;
                        if (!left_i.has(out_r_charge)) continue;

                        size_t l_size = left_i[lb].second; 
                        size_t o = ret.find_block(out_l_charge, out_r_charge);
                        if ( o == ret.n_blocks() )
                            o = ret.insert_block(Matrix(l_size, out_right_i.size_of_block(out_r_charge)), out_l_charge, out_r_charge);

                        std::vector<micro_task<value_type> > & otasks = tasks[&ret[o](0,0)];
                        std::vector<micro_task2<value_type> > & otasks2 = tasks2[&ret[o](0,0)];

                        int i = SymmGroup::spin(out_r_charge), ip = SymmGroup::spin(rc);
                        int j = SymmGroup::spin(out_l_charge), jp = SymmGroup::spin(mc);
                        int two_sp = std::abs(i - ip), two_s  = std::abs(j - jp);

                        typename Matrix::value_type couplings[4];
                        ::SU2::set_coupling(j, two_s, jp, a,k,ap, i, two_sp, ip, access.scale(op_index), couplings);

                        size_t in_left_offset = in_left_pb(phys_in, out_l_charge);
                        size_t out_right_offset = out_right_pb(phys_out, rc);
                        size_t r_size = T.basis().right_size(t_block);

                        unsigned short r_size_cache = 16384 / (l_size * W.basis().right_size(w_block)); // 128 KB
                        for (unsigned short slice = 0; slice < r_size/r_size_cache; ++slice)
                        {
                            size_t right_offset_cache = slice * r_size_cache;
                            otasks.push_back(micro_task<value_type>(b2, op_index, w_block, t_block, in_left_offset,
                                             right_offset_cache, out_right_offset + right_offset_cache, l_size, r_size_cache, r_size, couplings));

                            micro_task2<value_type> task(&T[t_block](in_left_offset, right_offset_cache), l_size, r_size_cache, r_size,
                                                         num_rows(T[t_block]), out_right_offset + right_offset_cache);

                            op_iterate<Matrix, SymmGroup>(W, w_block, couplings, task);
                            otasks2.push_back(task);
                        }

                        unsigned short r_size_remain = r_size % r_size_cache;
                        unsigned short right_offset_remain = r_size - r_size_remain;
                        if (r_size_remain == 0) continue;

                        otasks.push_back(micro_task<value_type>(b2, op_index, w_block, t_block, in_left_offset,
                                         right_offset_remain, out_right_offset + right_offset_remain, l_size, r_size_remain, r_size, couplings));

                        micro_task2<value_type> task(&T[t_block](in_left_offset, right_offset_remain), l_size, r_size_remain, r_size,
                                                     num_rows(T[t_block]), out_right_offset + right_offset_remain);

                        op_iterate<Matrix, SymmGroup>(W, w_block, couplings, task);
                        otasks2.push_back(task);

                } // wblock
                } // ket block
            } // op_index
        } // b2

        for (typename std::map<value_type*, std::vector<micro_task<value_type> > >::iterator it = tasks.begin(); it != tasks.end(); ++it)
        {
            std::vector<micro_task<value_type> > & otasks = it->second;
            //std::sort(otasks.begin(), otasks.end(), task_compare<value_type>()); 
            std::vector<micro_task2<value_type> > & otasks2 = tasks2[it->first];
            assert(otasks.size() == otasks2.size());

            for (typename std::vector<micro_task<value_type> >::const_iterator it2 = otasks.begin(); it2 != otasks.end(); ++it2)
            {
                MPOTensor_detail::term_descriptor<Matrix, SymmGroup, true> access = mpo.at(b1, it2->b);
                typename operator_selector<Matrix, SymmGroup>::type const & W = access.op(it2->op_index); 
                block_matrix<Matrix, SymmGroup> const & T = right_mult_mps.at(it2->b);
                value_type * oblock = it->first;

                detail::rbtm<Matrix, SymmGroup>(T[it2->t_block], oblock, W, it2->in_left_offset, it2->in_right_offset, it2->out_offset,
                                                it2->l_size, it2->r_size_cache, it2->r_size, it2->w_block, it2->scale);

                size_t index = it2-otasks.begin();
                micro_task2<value_type> mt2 = otasks2[index];
                
                assert(mt2.out_offset == it2->out_offset);
                assert(mt2.r_size_cache == it2->r_size_cache);
                assert(mt2.l_size == it2->l_size);
                assert(mt2.r_size == it2->r_size);
                //maquis::cout << mt2.out_offset << "\t" << it2->out_offset << std::endl;
            }
        }

        right_mult_mps.free(b1);
    }
} // namespace SU2
} // namespace contraction

#endif
