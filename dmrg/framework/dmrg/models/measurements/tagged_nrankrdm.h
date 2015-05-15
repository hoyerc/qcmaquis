/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
 *               2011-2011 by Bela Bauer <bauerb@phys.ethz.ch>
 *               2011-2013    Michele Dolfi <dolfim@phys.ethz.ch>
 *               2014-2014    Sebastian Keller <sebkelle@phys.ethz.ch>
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

#ifndef MEASUREMENTS_TAGGED_NRANKRDM_H
#define MEASUREMENTS_TAGGED_NRANKRDM_H

#include <algorithm>
#include <functional>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/filesystem.hpp>

#include "dmrg/models/term_descriptor.h"
#include "dmrg/models/measurement.h"
#include <dmrg/block_matrix/symmetry/nu1pg.h>

namespace measurements_details {

    template <class symm, class = void>
    class checkpg
    {
    public:

        //typedef typename Model<Matrix, SymmGroup>::term_descriptor term_descriptor;
        template <class matrix>
        bool operator()(term_descriptor<typename matrix::value_type> const & term, boost::shared_ptr<TagHandler<matrix, symm> > tag_handler, Lattice const & lat) 
        {
            return true;
        }
    };

    template <class symm>
    class checkpg<symm, typename boost::enable_if<symm_traits::HasPG<symm> >::type>
    {
    public:
        typedef typename symm::charge charge;
        typedef typename symm::subcharge subcharge;
        //typedef typename Model<Matrix, SymmGroup>::term_descriptor term_descriptor;

        template <class matrix>
        bool operator()(term_descriptor<typename matrix::value_type> const & term,
				boost::shared_ptr<TagHandler<matrix, symm> > tag_handler,
				Lattice const & lat) 
        {
            typedef typename TagHandler<matrix, symm>::op_t op_t;

		charge acc = symm::IdentityCharge;
            for (std::size_t p = 0; p < term.size(); ++p) {
		    charge local = symm::IdentityCharge;
		    if (tag_handler->is_fermionic(term.operator_tag(p)))
		        symm::irrep(local) = lat.get_prop<subcharge>("type", term.position(p));
		    acc = symm::fuse(acc, local);
            }
         
            //maquis::cout << " accumulate charge is " << acc << std::endl;

		if (acc == symm::IdentityCharge)
            	return true;
		//else
	//		maquis::cout << "accumulated charge is " << acc << std::endl;

		return false;
        }
    };

    template <class T>
    T get_indx_contr(std::vector<T> const & positions)
    {
        T contr_indx;
        return (((positions[0]+1-1)*(positions[0]+1)*(positions[0]+1+1)*(positions[0]+1+2))/24
               +((positions[1]+1-1)*(positions[1]+1)*(positions[1]+1+1))/6
               +((positions[2]+1-1)*(positions[2]+1))/2
               +  positions[3]+1
               );
    };

    template <class T>
    class compare_norm
    {
    public:
        bool operator()(std::vector<T> const & positions)
        {
            std::vector<T> positions_lhs;
            std::vector<T> positions_rhs;

            // set positions:
            for (int i=0; i<4; ++i) positions_lhs.push_back(positions[i]); 
            for (int i=4; i<8; ++i) positions_rhs.push_back(positions[i]); 

            // reverse sorting to ensure maximum norm 
            std::sort(positions_lhs.begin(), positions_lhs.end(), std::greater<T>());
            std::sort(positions_rhs.begin(), positions_rhs.end(), std::greater<T>());

            T norm_lhs = (((positions_lhs[0]+1-1)*(positions_lhs[0]+1)*(positions_lhs[0]+1+1)*(positions_lhs[0]+1+2))/24
                   +((positions_lhs[1]+1-1)*(positions_lhs[1]+1)*(positions_lhs[1]+1+1))/6
                   +((positions_lhs[2]+1-1)*(positions_lhs[2]+1))/2
                   +  positions_lhs[3]+1
                   );
            T norm_rhs = (((positions_rhs[0]+1-1)*(positions_rhs[0]+1)*(positions_rhs[0]+1+1)*(positions_rhs[0]+1+2))/24
                   +((positions_rhs[1]+1-1)*(positions_rhs[1]+1)*(positions_rhs[1]+1+1))/6
                   +((positions_rhs[2]+1-1)*(positions_rhs[2]+1))/2
                   +  positions_rhs[3]+1
                   );
         
            //maquis::cout << "lhs norm "  << norm_lhs << " <--> rhs norm " << norm_rhs << std::endl;

            if(norm_rhs > norm_lhs)
                return true;

            return false;
        }
    };
}

namespace measurements {
    
    template <class Matrix, class SymmGroup>
    class TaggedNRankRDM : public measurement<Matrix, SymmGroup> {

        typedef measurement<Matrix, SymmGroup> base;

        typedef typename Model<Matrix, SymmGroup>::term_descriptor term_descriptor;

        typedef Lattice::pos_t pos_t;
        typedef std::vector<pos_t> positions_type;

        typedef typename base::op_t op_t;
        typedef typename OPTable<Matrix, SymmGroup>::tag_type tag_type;
        typedef typename Matrix::value_type value_type;

        typedef std::vector<tag_type> tag_vec;
        typedef std::vector<tag_vec> bond_term;
        typedef std::pair<std::vector<tag_vec>, value_type> scaled_bond_term;
    
    public:
        TaggedNRankRDM(std::string const& name_, const Lattice & lat,
                       boost::shared_ptr<TagHandler<Matrix, SymmGroup> > tag_handler_,
                       tag_vec const & identities_, tag_vec const & fillings_, std::vector<scaled_bond_term> const& ops_,
                       bool half_only_, positions_type const& positions_ = positions_type(),
                       std::string const& ckp_ = std::string(""))
        : base(name_)
        , lattice(lat)
        , tag_handler(tag_handler_)
        , positions_first(positions_)
        , identities(identities_)
        , fillings(fillings_)
        , operator_terms(ops_)
        , bra_ckp(ckp_)
        {
            pos_t extent = operator_terms.size() > 2 ? lattice.size() : lattice.size()-1;
            if (positions_first.size() == 0)
                std::copy(boost::counting_iterator<pos_t>(0), boost::counting_iterator<pos_t>(extent),
                          back_inserter(positions_first));
            
            //this->cast_to_real = is_hermitian_meas(ops[0]);
            this->cast_to_real = false;
        }
        
        void evaluate(MPS<Matrix, SymmGroup> const& ket_mps, boost::optional<reduced_mps<Matrix, SymmGroup> const&> rmps = boost::none)
        {
            this->vector_results.clear();
            this->labels.clear();

            MPS<Matrix, SymmGroup> bra_mps;
            if (bra_ckp != "") {
                if(boost::filesystem::exists(bra_ckp))
                    load(bra_ckp, bra_mps);
                else
                    throw std::runtime_error("The bra checkpoint file " + bra_ckp + " was not found\n");
            }

            if (operator_terms[0].first.size() == 2)
                measure_correlation(bra_mps, ket_mps);
            else if (operator_terms[0].first.size() == 4)
                measure_2rdm(bra_mps, ket_mps);
            else if (operator_terms[0].first.size() == 6)
                measure_3rdm(bra_mps, ket_mps);
            else if (operator_terms[0].first.size() == 8)
                if (positions_first.size() == lattice.size())
                    measure_4rdm(bra_mps, ket_mps);
                else if(positions_first.size() == 3)
                    measure_4rdm_fixed_pos(bra_mps, ket_mps);
                else
                    throw std::runtime_error("4-RDM measurement at the moment supported with 3 fixed positions, #pos is "
                                             + boost::lexical_cast<std::string>(positions_first.size()));
            else
                throw std::runtime_error("correlation measurements at the moment supported with 2, 4, 6 and 8 operators, size is "
                                          + boost::lexical_cast<std::string>(operator_terms[0].first.size()));
        }
        
    protected:

        measurement<Matrix, SymmGroup>* do_clone() const
        {
            return new TaggedNRankRDM(*this);
        }
        
        void measure_correlation(MPS<Matrix, SymmGroup> const & dummy_bra_mps,
                                 MPS<Matrix, SymmGroup> const & ket_mps,
                                 std::vector<pos_t> const & order = std::vector<pos_t>())
        {
            // Test if a separate bra state has been specified
            bool bra_neq_ket = (dummy_bra_mps.length() > 0);
            MPS<Matrix, SymmGroup> const & bra_mps = (bra_neq_ket) ? dummy_bra_mps : ket_mps;

            #ifdef MAQUIS_OPENMP
            #pragma omp parallel for
            #endif
            for (std::size_t i = 0; i < positions_first.size(); ++i) {
                pos_t p1 = positions_first[i];
                boost::shared_ptr<TagHandler<Matrix, SymmGroup> > tag_handler_local(new TagHandler<Matrix, SymmGroup>(*tag_handler));

                std::vector<typename MPS<Matrix, SymmGroup>::scalar_type> dct;
                std::vector<std::vector<pos_t> > num_labels;
                bool measured = false;
                for (pos_t p2 = p1+1; p2 < lattice.size(); ++p2)
                { 
                    pos_t pos_[2] = {p1, p2};
                    std::vector<pos_t> positions(pos_, pos_ + 2);

                    tag_vec operators(2);
                    operators[0] = operator_terms[0].first[0][lattice.get_prop<typename SymmGroup::subcharge>("type", p1)];
                    operators[1] = operator_terms[0].first[1][lattice.get_prop<typename SymmGroup::subcharge>("type", p2)];

                    // check if term is allowed by symmetry
                    term_descriptor term = generate_mpo::arrange_operators(positions, operators, tag_handler_local);
                    if(not measurements_details::checkpg<SymmGroup>()(term, tag_handler_local, lattice))
                          continue;
                    measured = true;
                    
                    //MPO<Matrix, SymmGroup> mpo = generate_mpo::make_1D_mpo(positions, operators, identities, fillings, tag_handler_local, lattice);
                    MPO<Matrix, SymmGroup> mpo = generate_mpo::sign_and_fill(term, identities, fillings, tag_handler_local, lattice);
                    typename MPS<Matrix, SymmGroup>::scalar_type value = operator_terms[0].second * expval(bra_mps, ket_mps, mpo);

                    if(measured)
                    {
                         dct.push_back(value);
                         num_labels.push_back(positions);
                    }
                }

                std::vector<std::string> lbt = label_strings(lattice,  (order.size() > 0)
                                            ? detail::resort_labels(num_labels, order, false) : num_labels );
                // save results and labels
                #ifdef MAQUIS_OPENMP
                #pragma omp critical
                #endif
                {
                    this->vector_results.reserve(this->vector_results.size() + dct.size());
                    std::copy(dct.rbegin(), dct.rend(), std::back_inserter(this->vector_results));

                    this->labels.reserve(this->labels.size() + dct.size());
                    std::copy(lbt.rbegin(), lbt.rend(), std::back_inserter(this->labels));
                }
            }
        }

        void measure_2rdm(MPS<Matrix, SymmGroup> const & dummy_bra_mps,
                          MPS<Matrix, SymmGroup> const & ket_mps,
                          std::vector<pos_t> const & order = std::vector<pos_t>())
        {
            // Test if a separate bra state has been specified
            bool bra_neq_ket = (dummy_bra_mps.length() > 0);
            MPS<Matrix, SymmGroup> const & bra_mps = (bra_neq_ket) ? dummy_bra_mps : ket_mps;

            #ifdef MAQUIS_OPENMP
            #pragma omp parallel for collapse(1)
            #endif
            for (pos_t p1 = 0; p1 < lattice.size(); ++p1)
            for (pos_t p2 = 0; p2 < lattice.size(); ++p2)
            {
                boost::shared_ptr<TagHandler<Matrix, SymmGroup> > tag_handler_local(new TagHandler<Matrix, SymmGroup>(*tag_handler));

                // Permutation symmetry for bra == ket: pqrs == rspq == qpsr == srqp
                // if bra != ket, pertmutation symmetry is only pqrs == qpsr
                pos_t subref = 0;
                if (bra_neq_ket)
                    subref = 0;
                else
                    subref = std::min(p1, p2);

                for (pos_t p3 = subref; p3 < lattice.size(); ++p3)
                { 
                    std::vector<typename MPS<Matrix, SymmGroup>::scalar_type> dct;
                    std::vector<std::vector<pos_t> > num_labels;

                    for (pos_t p4 = p3; p4 < lattice.size(); ++p4)
                    { 
                        pos_t pos_[4] = {p1, p2, p3, p4};
                        std::vector<pos_t> positions(pos_, pos_ + 4);

                        // Loop over operator terms that are measured synchronously and added together
                        // Used e.g. for the four spin combos of the 2-RDM
                        typename MPS<Matrix, SymmGroup>::scalar_type value = 0;
                        bool measured = false;
                        for (std::size_t synop = 0; synop < operator_terms.size(); ++synop) {

                            tag_vec operators(4);
                            operators[0] = operator_terms[synop].first[0][lattice.get_prop<typename SymmGroup::subcharge>("type", p1)];
                            operators[1] = operator_terms[synop].first[1][lattice.get_prop<typename SymmGroup::subcharge>("type", p2)];
                            operators[2] = operator_terms[synop].first[2][lattice.get_prop<typename SymmGroup::subcharge>("type", p3)];
                            operators[3] = operator_terms[synop].first[3][lattice.get_prop<typename SymmGroup::subcharge>("type", p4)];

                            // check if term is allowed by symmetry
                            term_descriptor term = generate_mpo::arrange_operators(positions, operators, tag_handler_local);
                            if(not measurements_details::checkpg<SymmGroup>()(term, tag_handler_local, lattice))
                                  continue;

                            measured = true;
                            
                            //MPO<Matrix, SymmGroup> mpo = generate_mpo::make_1D_mpo(positions, operators, identities, fillings, tag_handler_local, lattice);
                            MPO<Matrix, SymmGroup> mpo = generate_mpo::sign_and_fill(term, identities, fillings, tag_handler_local, lattice);
                            value += operator_terms[synop].second * expval(bra_mps, ket_mps, mpo);
                        }
                        if(measured)
                        {
                             dct.push_back(value);
                             num_labels.push_back(positions);
                        }
                    }

                    std::vector<std::string> lbt = label_strings(lattice,  (order.size() > 0)
                                                ? detail::resort_labels(num_labels, order, false) : num_labels );

                    // save results and labels
                    #ifdef MAQUIS_OPENMP
                    #pragma omp critical
                    #endif
                    {
                        this->vector_results.reserve(this->vector_results.size() + dct.size());
                        std::copy(dct.rbegin(), dct.rend(), std::back_inserter(this->vector_results));

                        this->labels.reserve(this->labels.size() + dct.size());
                        std::copy(lbt.rbegin(), lbt.rend(), std::back_inserter(this->labels));
                    }
                }
            }
        }

        void measure_3rdm(MPS<Matrix, SymmGroup> const & dummy_bra_mps,
                          MPS<Matrix, SymmGroup> const & ket_mps,
                          std::vector<pos_t> const & order = std::vector<pos_t>())
        {
            // Test if a separate bra state has been specified bool bra_neq_ket = (dummy_bra_mps.length() > 0);
            bool bra_neq_ket = (dummy_bra_mps.length() > 0);
            MPS<Matrix, SymmGroup> const & bra_mps = (bra_neq_ket) ? dummy_bra_mps : ket_mps;

            // if bra != ket, no transpose symmetry
            #ifdef MAQUIS_OPENMP
            #pragma omp parallel for collapse(2) schedule (dynamic,1)
            #endif
            for (pos_t p1 = 0; p1 < lattice.size(); ++p1)
            for (pos_t p2 = 0; p2 < lattice.size(); ++p2)
            {
                if(p2 > p1) continue;

                boost::shared_ptr<TagHandler<Matrix, SymmGroup> > tag_handler_local(new TagHandler<Matrix, SymmGroup>(*tag_handler));

                for (pos_t p3 = std::min(p1, p2); p3 < lattice.size(); ++p3)
                {
                    if(p1 == p2 && p1 == p3)
                        continue;

                    for (pos_t p4 = ((bra_neq_ket) ? 0 : std::min(p1, p2)); p4 < lattice.size(); ++p4)
                    {
    
                        for (pos_t p5 = ((bra_neq_ket) ? 0 : std::min(p1, p2)); p5 < lattice.size(); ++p5)
                        { 
                            std::vector<typename MPS<Matrix, SymmGroup>::scalar_type> dct;
                            std::vector<std::vector<pos_t> > num_labels;
    
                            for (pos_t p6 = std::min(p4, p5); p6 < lattice.size(); ++p6)
                            {
                                // sixth index must be different if p4 == p5 
                                if(p4 == p5 && p4 == p6)
                                      continue;
    
                                // defines position vector for spin-free 3-RDM element
                                pos_t pos_[6] = {p1, p2, p3, p4, p5, p6};
                                std::vector<pos_t> positions(pos_, pos_ + 6);
    
                                // Loop over operator terms that are measured synchronously and added together
                                // Used e.g. for the spin combos of the 3-RDM
                                typename MPS<Matrix, SymmGroup>::scalar_type value = 0;
                                bool measured = false;
                                for (std::size_t synop = 0; synop < operator_terms.size(); ++synop) {
    
                                    tag_vec operators(6);
                                    operators[0] = operator_terms[synop].first[0][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[0])];
                                    operators[1] = operator_terms[synop].first[1][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[1])];
                                    operators[2] = operator_terms[synop].first[2][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[2])];
                                    operators[3] = operator_terms[synop].first[3][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[3])];
                                    operators[4] = operator_terms[synop].first[4][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[4])];
                                    operators[5] = operator_terms[synop].first[5][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[5])];
    
                                    //maquis::cout << " point group check: term is allowed? --> " << measurements_details::checkpg<SymmGroup>()(operators, tag_handler_local) << std::endl;
    
                                    // check if term is allowed by symmetry
                                    term_descriptor term = generate_mpo::arrange_operators(positions, operators, tag_handler_local);
                                    if(not measurements_details::checkpg<SymmGroup>()(term, tag_handler_local, lattice))
                                         continue;
                                    measured = true;
    
                                    //MPO<Matrix, SymmGroup> mpo = generate_mpo::make_1D_mpo(positions, operators, identities, fillings, tag_handler_local, lattice);
                                    MPO<Matrix, SymmGroup> mpo = generate_mpo::sign_and_fill(term, identities, fillings, tag_handler_local, lattice);
                                value += operator_terms[synop].second * expval(bra_mps, ket_mps, mpo);
    
                                }
                                // debug print
                                //if (std::abs(value) > 0)
                                //{
                                //    std::transform(positions.begin(), positions.end(), std::ostream_iterator<pos_t>(std::cout, " "), boost::lambda::_1 + 1);
                                //    maquis::cout << " " << value << std::endl;
                                //}
                                if(measured)
                                {
                                     dct.push_back(value);
                                     num_labels.push_back(positions);
                                }
                            }
    
                            std::vector<std::string> lbt = label_strings(lattice,  (order.size() > 0)
                                                        ? detail::resort_labels(num_labels, order, false) : num_labels );

                            // save results and labels
                            #ifdef MAQUIS_OPENMP
                            #pragma omp critical
                            #endif
                            {
                                this->vector_results.reserve(this->vector_results.size() + dct.size());
                                std::copy(dct.rbegin(), dct.rend(), std::back_inserter(this->vector_results));

                                this->labels.reserve(this->labels.size() + dct.size());
                                std::copy(lbt.rbegin(), lbt.rend(), std::back_inserter(this->labels));
                            }
                        }
                    }
                }
            }
        }

        void measure_4rdm(MPS<Matrix, SymmGroup> const & dummy_bra_mps,
                          MPS<Matrix, SymmGroup> const & ket_mps,
                          std::vector<pos_t> const & order = std::vector<pos_t>())
        {
            // Test if a separate bra state has been specified bool bra_neq_ket = (dummy_bra_mps.length() > 0);
            bool bra_neq_ket = (dummy_bra_mps.length() > 0);
            MPS<Matrix, SymmGroup> const & bra_mps = (bra_neq_ket) ? dummy_bra_mps : ket_mps;

            #ifdef MAQUIS_OPENMP
            //#pragma omp parallel for collapse(1) schedule(dynamic)
            #pragma omp parallel for collapse(3) schedule(dynamic,1)
            #endif

            for (pos_t p4 = 0               ; p4 < lattice.size()-1; ++p4)
            for (pos_t p3 = 0               ; p3 < lattice.size()-1; ++p3)
            {
                 for (pos_t p1 = lattice.size()-1; p1 >= 0; --p1)
                 {
                      if(p4 > p3 || p4 > p1 || p3 > p1) continue;

                      //if(p1 == p2 && p4 == p1) continue;

                      boost::shared_ptr<TagHandler<Matrix, SymmGroup> > tag_handler_local(new TagHandler<Matrix, SymmGroup>(*tag_handler));
                      MPS<Matrix, SymmGroup> ket_mps_local = ket_mps;

                      std::vector<typename MPS<Matrix, SymmGroup>::scalar_type> dct;
                      std::vector<std::vector<pos_t> > num_labels;

                      for (pos_t p2 = p1              ; p2 >= 0; --p2)
                      //for (pos_t p3 = 0               ; p3 < lattice.size()-1; ++p3)
                      {
                          if(p3 > p2) continue;
                          //if(p3 > p2 || p4 > p3) continue;

                          // third index must be different if p1 == p2 
                          if(p1 == p2 && p3 == p1) continue;

                          // fourth index must be different if p1 == p2 or p1 == p3 or p2 == p3
                          if((p1 == p2 && p4 == p1) || (p1 == p3 && p4 == p1) || (p2 == p3 && p4 == p2)) continue;
                          //if((p1 == p3 && p4 == p1) || (p2 == p3 && p4 == p2)) continue;

                          for (pos_t p5 = 0; p5 < p1+1; ++p5)
                          for (pos_t p6 = 0; p6 < p1+1; ++p6)
                          for (pos_t p7 = 0; p7 < p1+1; ++p7)
                          {
                              // seventh index must be different if p5 == p6
                              if(p5 == p6 && p7 == p5) continue;

                              {

                                  for (pos_t p8 = 0; p8 < p1+1; ++p8)
                                  {
                                      // eighth index must be different if p5 == p6 or p5 == p7 or p6 == p7
                                      if((p5 == p6 && p8 == p5) || (p5 == p7 && p8 == p5) || (p6 == p7 && p8 == p6))
                                          continue;
     
                                      // defines position vector for spin-free 4-RDM element
                                      pos_t pos_[8] = {p1, p2, p3, p4, p5, p6, p7, p8};
                                      std::vector<pos_t> positions(pos_, pos_ + 8);

                                      // check norm of lhs and rhs - skip if norm of rhs > lhs
                                      if(measurements_details::compare_norm<pos_t>()(positions)) continue;

                                      // Loop over operator terms that are measured synchronously and added together
                                      // Used e.g. for the spin combos of the 4-RDM
                                      typename MPS<Matrix, SymmGroup>::scalar_type value = 0;
                                      bool measured = false;
                                      for (std::size_t synop = 0; synop < operator_terms.size(); ++synop) {
     
                                          tag_vec operators(8);
                                          operators[0] = operator_terms[synop].first[0][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[0])];
                                          operators[1] = operator_terms[synop].first[1][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[1])];
                                          operators[2] = operator_terms[synop].first[2][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[2])];
                                          operators[3] = operator_terms[synop].first[3][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[3])];
                                          operators[4] = operator_terms[synop].first[4][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[4])];
                                          operators[5] = operator_terms[synop].first[5][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[5])];
                                          operators[6] = operator_terms[synop].first[6][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[6])];
                                          operators[7] = operator_terms[synop].first[7][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[7])];
     
                                          // check if term is allowed by symmetry
                                          term_descriptor term = generate_mpo::arrange_operators(positions, operators, tag_handler_local);
                                          if(not measurements_details::checkpg<SymmGroup>()(term, tag_handler_local, lattice))
                                               continue;
                                          measured = true;
     
                                          MPO<Matrix, SymmGroup> mpo = generate_mpo::sign_and_fill(term, identities, fillings, tag_handler_local, lattice);
                                          typename MPS<Matrix, SymmGroup>::scalar_type local_value = expval(ket_mps_local, ket_mps_local, mpo);
                                          //maquis::cout << "synop term " << synop+1 << "--> local value: " << local_value << std::endl;
                                          //value += operator_terms[synop].second * expval(ket_mps_local, ket_mps_local, mpo);
                                          value += operator_terms[synop].second * local_value;
     
                                      }

                                      // debug print
                                      if (std::abs(value) > 0)
                                      {
                                          std::transform(positions.begin(), positions.end(), std::ostream_iterator<pos_t>(std::cout, " "), boost::lambda::_1 + 1);
                                          maquis::cout << " " << value << std::endl;
                                      }

                                      if(measured)
                                      {
                                          // defines position vector for contracted spin-free 4-RDM element
                                          //pos_t pcontr = measurements_details::get_indx_contr<pos_t>(positions);
     
                                          //pos_t pos_f_[5] = {pcontr, p5, p6, p7, p8};
                                          //std::vector<pos_t> positions_f(pos_f_, pos_f_ + 5);
                                          
                                          dct.push_back(value);
                                          //num_labels.push_back(positions_f);
                                          num_labels.push_back(positions_f);
                                      }
                                  }
                              }
                          }
                      }
                      std::vector<std::string> lbt = label_strings(lattice,  (order.size() > 0)
                                                  ? detail::resort_labels(num_labels, order, false) : num_labels );
     
                      // save results and labels
                      #ifdef MAQUIS_OPENMP
                      #pragma omp critical
                      #endif
                      {
                          this->vector_results.reserve(this->vector_results.size() + dct.size());
                          std::copy(dct.rbegin(), dct.rend(), std::back_inserter(this->vector_results));
     
                          this->labels.reserve(this->labels.size() + dct.size());
                          std::copy(lbt.rbegin(), lbt.rend(), std::back_inserter(this->labels));
                      }
                 }
             }
         }

        void measure_4rdm_fixed_pos(MPS<Matrix, SymmGroup> const & dummy_bra_mps,
                                    MPS<Matrix, SymmGroup> const & ket_mps,
                                    std::vector<pos_t> const & order = std::vector<pos_t>())
        {
            // Test if a separate bra state has been specified bool bra_neq_ket = (dummy_bra_mps.length() > 0);
            bool bra_neq_ket = (dummy_bra_mps.length() > 0);
            MPS<Matrix, SymmGroup> const & bra_mps = (bra_neq_ket) ? dummy_bra_mps : ket_mps;

            //#ifdef MAQUIS_OPENMP
            //#pragma omp parallel for collapse(3) schedule(dynamic,1)
            //#endif

            pos_t p4 = positions_first[0];
            pos_t p3 = positions_first[1];
            {
                 pos_t p1 = positions_first[2];

                 {
                      boost::shared_ptr<TagHandler<Matrix, SymmGroup> > tag_handler_local(new TagHandler<Matrix, SymmGroup>(*tag_handler));
                      MPS<Matrix, SymmGroup> ket_mps_local = ket_mps;

                      std::vector<typename MPS<Matrix, SymmGroup>::scalar_type> dct;
                      std::vector<std::vector<pos_t> > num_labels;

                      for (pos_t p2 = p1              ; p2 >= 0; --p2)
                      {
                          if(p4 > p3 || p4 > p1 || p3 > p1 || p3 > p2) continue;

                          // third index must be different if p1 == p2 
                          if(p1 == p2 && p3 == p1) continue;

                          // fourth index must be different if p1 == p2 or p1 == p3 or p2 == p3
                          if((p1 == p2 && p4 == p1) || (p1 == p3 && p4 == p1) || (p2 == p3 && p4 == p2)) continue;

                          for (pos_t p5 = 0; p5 < p1+1; ++p5)
                          for (pos_t p6 = 0; p6 < p1+1; ++p6)
                          for (pos_t p7 = 0; p7 < p1+1; ++p7)
                          {
                              // seventh index must be different if p5 == p6
                              if(p5 == p6 && p7 == p5) continue;

                              {

                                  for (pos_t p8 = 0; p8 < p1+1; ++p8)
                                  {
                                      // eighth index must be different if p5 == p6 or p5 == p7 or p6 == p7
                                      if((p5 == p6 && p8 == p5) || (p5 == p7 && p8 == p5) || (p6 == p7 && p8 == p6))
                                          continue;
     
                                      // defines position vector for spin-free 4-RDM element
                                      pos_t pos_[8] = {p1, p2, p3, p4, p5, p6, p7, p8};
                                      std::vector<pos_t> positions(pos_, pos_ + 8);

                                      // check norm of lhs and rhs - skip if norm of rhs > lhs
                                      if(measurements_details::compare_norm<pos_t>()(positions)) continue;

                                      // Loop over operator terms that are measured synchronously and added together
                                      // Used e.g. for the spin combos of the 4-RDM
                                      typename MPS<Matrix, SymmGroup>::scalar_type value = 0;
                                      bool measured = false;
                                      for (std::size_t synop = 0; synop < operator_terms.size(); ++synop) {
     
                                          tag_vec operators(8);
                                          operators[0] = operator_terms[synop].first[0][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[0])];
                                          operators[1] = operator_terms[synop].first[1][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[1])];
                                          operators[2] = operator_terms[synop].first[2][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[2])];
                                          operators[3] = operator_terms[synop].first[3][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[3])];
                                          operators[4] = operator_terms[synop].first[4][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[4])];
                                          operators[5] = operator_terms[synop].first[5][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[5])];
                                          operators[6] = operator_terms[synop].first[6][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[6])];
                                          operators[7] = operator_terms[synop].first[7][lattice.get_prop<typename SymmGroup::subcharge>("type", positions[7])];
     
                                          // check if term is allowed by symmetry
                                          term_descriptor term = generate_mpo::arrange_operators(positions, operators, tag_handler_local);
                                          if(not measurements_details::checkpg<SymmGroup>()(term, tag_handler_local, lattice))
                                               continue;
                                          measured = true;
     
                                          MPO<Matrix, SymmGroup> mpo = generate_mpo::sign_and_fill(term, identities, fillings, tag_handler_local, lattice);
                                          typename MPS<Matrix, SymmGroup>::scalar_type local_value = expval(ket_mps_local, ket_mps_local, mpo);
                                          //maquis::cout << "synop term " << synop+1 << "--> local value: " << local_value << std::endl;
                                          //value += operator_terms[synop].second * expval(ket_mps_local, ket_mps_local, mpo);
                                          value += operator_terms[synop].second * local_value;
     
                                      }

                                      // debug print
                                      if (std::abs(value) > 0)
                                      {
                                          std::transform(positions.begin(), positions.end(), std::ostream_iterator<pos_t>(std::cout, " "), boost::lambda::_1 + 1);
                                          maquis::cout << " " << value << std::endl;
                                      }

                                      if(measured)
                                      {
                                          // defines position vector for contracted spin-free 4-RDM element
                                          //pos_t pcontr = measurements_details::get_indx_contr<pos_t>(positions);
     
                                          //pos_t pos_f_[5] = {pcontr, p5, p6, p7, p8};
                                          //std::vector<pos_t> positions_f(pos_f_, pos_f_ + 5);
                                          
                                          dct.push_back(value);
                                          //num_labels.push_back(positions_f);
                                          num_labels.push_back(positions);
                                      }
                                  }
                              }
                          }
                      }
                      std::vector<std::string> lbt = label_strings(lattice,  (order.size() > 0)
                                                  ? detail::resort_labels(num_labels, order, false) : num_labels );
     
                      // save results and labels
                      #ifdef MAQUIS_OPENMP
                      #pragma omp critical
                      #endif
                      {
                          this->vector_results.reserve(this->vector_results.size() + dct.size());
                          std::copy(dct.rbegin(), dct.rend(), std::back_inserter(this->vector_results));
     
                          this->labels.reserve(this->labels.size() + dct.size());
                          std::copy(lbt.rbegin(), lbt.rend(), std::back_inserter(this->labels));
                      }
                 }
             }
         }

    private:
        Lattice lattice;
        boost::shared_ptr<TagHandler<Matrix, SymmGroup> > tag_handler;
        positions_type positions_first;
        tag_vec identities, fillings;
        std::vector<scaled_bond_term> operator_terms;

        std::string bra_ckp;
    };
}

#endif
