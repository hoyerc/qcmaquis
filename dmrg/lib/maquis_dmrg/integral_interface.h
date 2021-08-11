/*****************************************************************************
*
* ALPS MPS DMRG Project
*
* Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
*               2011-2011 by Bela Bauer <bauerb@phys.ethz.ch>
*               2011-2013    Michele Dolfi <dolfim@phys.ethz.ch>
*               2014-2014    Sebastian Keller <sebkelle@phys.ethz.ch>
*               2019         Leon Freitag <lefreita@ethz.ch>
*               2020- by Robin Feldmann <robinfe@phys.chem.ethz.ch>
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
#ifndef INTEGRAL_INTERFACE_H
#define INTEGRAL_INTERFACE_H

/* internal include */
#include "dmrg/utils/align.h"

/* external include */
#include <unordered_map>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/serialization/utility.hpp>
#include <boost/serialization/complex.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/functional/hash.hpp>

namespace chem {

    enum class Hamiltonian {Electronic, Vibrational, PreBO};

    constexpr int getIndexDim (const Hamiltonian& type) {
        int indexDim=0;
        switch (type) {
            case Hamiltonian::Electronic:
                indexDim = 4;
                break;
            case Hamiltonian::Vibrational:
                indexDim = 6;
                break;
            case Hamiltonian::PreBO:
                indexDim = 8;
                break;
        }
        return indexDim;
    }

    template <Hamiltonian HamiltonianType=Hamiltonian::Electronic, int N = getIndexDim(HamiltonianType)>
    using index_type = std::array<int, N>;

    // Classes to represent integrals

    template <class V, Hamiltonian HamiltonianType=Hamiltonian::Electronic>
    using integral_tuple = std::pair<index_type<HamiltonianType>, V>;

    template <class V, Hamiltonian HamiltonianType=Hamiltonian::Electronic>
    using integrals = std::vector<integral_tuple<V, HamiltonianType> >; // TODO: use a map later

    // structs needed for distinguishing whether we have a complex type or not
    // required for proper integral permutation rules in the integral_map
    template<typename T>
    struct is_complex_t : public std::false_type {};
    template<typename T>
    struct is_complex_t<std::complex<T> > : public std::true_type {};


    template <Hamiltonian HamiltonianType=Hamiltonian::Electronic>
    struct integral_hash
    {
        public:
            std::size_t operator()(const index_type<HamiltonianType>& id) const
            {
                return boost::hash_range(id.begin(), id.end());
            }
    };

    // Map that maps the four indices to an integral value
    // and handles index permutations internally.
    // Indexing is as in the FCIDUMP file, i.e.
    // Orbital indices start from 1 and 2e integrals use all four indices
    // 1e integrals use the first two indices and 0,0 as the 3rd and the 4th index
    // Nuclear repulsion energy uses an index 0,0,0,0

    template <class V, Hamiltonian HamiltonianType=Hamiltonian::Electronic>
    class integral_map
    {
        public:
            typedef std::unordered_map<index_type<HamiltonianType>, V, integral_hash<HamiltonianType>> map_t;
            typedef typename map_t::size_type size_type;

            // Type which returns std::abs(V), for the integral cutoff
            // Not very clean but std::conditional seems not to work here
            typedef typename std::complex<V>::value_type value_type;

            integral_map() = default;

            // Explicit copy using this->operator[]() to avoid potential doubling due to symmetry permutation
            // Not implementing it in the move constructor yet
            explicit integral_map(const map_t & map, value_type cutoff=0.0) : cutoff_(cutoff)
            {
                for (auto&& it: map)
                    (*this)[it->first] = it->second;
            }

            explicit integral_map(map_t && map, value_type cutoff=0.0) : map_(map), cutoff_(cutoff) {};

            // allow initializer lists for construction
            integral_map(std::initializer_list<typename map_t::value_type> l, value_type cutoff=0.0) : integral_map(map_t(l), cutoff) {};

            typedef typename map_t::iterator iterator;
            typedef typename map_t::const_iterator const_iterator;

            iterator begin() { return map_.begin(); };
            const_iterator begin() const { return map_.begin(); };
            iterator end() { return map_.end(); };
            const_iterator end() const { return map_.end(); };

            // For complex integrals, use relativistic permutation. Otherwise, use nonrelativistic permutation
            // Maybe these two properties should be decoupled in the future
            V& operator[](const index_type<HamiltonianType> & key) { return map_[maquis::detail::align<is_complex_t<V>::value>(key)]; };
            const V& operator[](const index_type<HamiltonianType> & key) const { return map_[maquis::detail::align<is_complex_t<V>::value>(key)]; };
            V& at(const index_type<HamiltonianType> & key) { return map_.at(maquis::detail::align<is_complex_t<V>::value>(key)); };
            const V& at(const index_type<HamiltonianType> & key) const { return map_.at(maquis::detail::align<is_complex_t<V>::value>(key)); };

            size_type size() const { return map_.size(); }
        private:
            friend class boost::serialization::access;

            map_t map_;

            // Integral cutoff
            value_type cutoff_;

            template <typename Archive>
            friend void serialize(Archive& ar, integral_map &i, const unsigned int version)
            {
                ar & i.map_;
            }
    };

    // Serialize the integral into a string

    template <class V, Hamiltonian HamiltonianType=Hamiltonian::Electronic>
    std::string serialize(const integral_map<V, HamiltonianType>& ints)
    {
        std::stringstream ss;
        boost::archive::text_oarchive oa{ss};
        oa << ints;

        return ss.str();
    }

}

#endif
