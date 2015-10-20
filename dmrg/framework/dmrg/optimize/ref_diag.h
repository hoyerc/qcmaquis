/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
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

#ifndef REF_H_DIAG_H
#define REF_H_DIAG_H

    template<class Matrix, class SymmGroup>
    void ref_diag(SiteProblem<Matrix, SymmGroup> const & H,
                  MPSTensor<Matrix, SymmGroup> x)
    {
        typedef typename Matrix::value_type value_type;

        x.make_left_paired();
        x.multiply_by_scalar(.0);
        block_matrix<Matrix, SymmGroup> & bm = x.data();

        for (size_t b = 0; b < bm.n_blocks(); ++b)
        {
            maquis::cout << bm.basis().left_charge(b) << bm.basis().right_charge(b) << std::endl;
            for (size_t i = 0; i < num_rows(bm[b]); ++i)
            for (size_t j = 0; j < num_cols(bm[b]); ++j)
            {
                bm[b](i,j) = 1;    
                MPSTensor<Matrix, SymmGroup> prod;
                ietl::mult(H, x, prod);
                maquis::cout << "  " << i << "," << j << "  " << prod.data()[b](i,j) << std::endl;;
                bm[b](i,j) = 0;    
            }
        }
    }

#endif 
