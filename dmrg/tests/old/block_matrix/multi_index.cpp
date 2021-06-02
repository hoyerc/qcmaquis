#include <iostream>

#include <vector>
#include <iterator>
#include <algorithm>

#include "dmrg/block_matrix/block_matrix.h"
#include "dmrg/block_matrix/multi_index.h"
#include "dmrg/block_matrix/detail/alps.hpp"
typedef alps::numeric::matrix<double> Matrix;

typedef U1 symm;

std::ostream& operator<< (std::ostream& os, std::pair<symm::charge, std::size_t> const& p)
{
    os << "(" << p.first << " : " << p.second << ")";
    return os;
}

std::ostream& operator<< (std::ostream& os, index_product_iterator<symm>::value_type const& v)
{
    //std::copy(v.begin(), v.end(), std::ostream_iterator<std::pair<symm::charge, std::size_t> >(os, " "));
    for (int i=0; i<v.size(); ++i)
        os << v[i] << " ";
    return os;
}

std::ostream& operator<< (std::ostream& os, std::pair<MultiIndex<symm>::coord_t, MultiIndex<symm>::coord_t> const& p)
{
    os << p.first << ", " << p.second;
    return os;
}

//template <class SymmGroup>
//std::ostream& operator<< (std::ostream& os, std::pair<typename SymmGroup::charge, std::size_t> const& p)
//{
//    os << "(" << p.first << " : " << p.second << ")";
//    return os;
//}
//
//template<typename T> struct is_mystruct{
//    typedef std::false_type type;
//};
//template<class SymmGroup> struct is_mystruct<std::vector<std::pair<typename SymmGroup::charge, std::size_t> > > {
//    typedef std::true_type type;
//};
//
//template <class SymmGroupPairVector>
//typename std::enable_if<is_mystruct<SymmGroupPairVector>::value,
//std::ostream&
//>::type
//operator<< (std::ostream& os, SymmGroupPairVector const& v)
//{
//    for (int i=0; i<v.size(); ++i)
//        os << v[i] << " ";
//    return os;
//}
//
//template <class SymmGroup>
//std::ostream& operator<< (std::ostream& os, std::pair<typename MultiIndex<SymmGroup>::coord_t, typename MultiIndex<SymmGroup>::coord_t> const& p)
//{
//    os << p.first << ", " << p.second;
//    return os;
//}


void test_iterator() {

    Index<symm> ix1, ix2;
    ix1.insert(std::make_pair(0, 1));
    ix1.insert(std::make_pair(1, 2));
    ix1.insert(std::make_pair(2, 1));

    ix2.insert(std::make_pair(0, 1));
    ix2.insert(std::make_pair(1, 1));
    ix2.insert(std::make_pair(2, 2));

    std::vector<Index<symm> > vec;
    vec.push_back(ix1);
    vec.push_back(ix2);


    for(index_product_iterator<symm> it(vec);
        it != index_product_iterator<symm>();
        it++)
    {
        maquis::cout << *it << std::endl;
    }

}

void test_multi_index() {

    Index<symm> ix1;
    ix1.insert(std::make_pair(0, 1));
    ix1.insert(std::make_pair(1, 1));

    Index<symm> ix2(ix1), ix3(ix1), ix4(ix1);


    maquis::cout << "* contruct and fill idx_" << std::endl;
    MultiIndex<symm> midx;
    MultiIndex<symm>::index_id s1 = midx.insert_index(ix1);
    MultiIndex<symm>::index_id s2 = midx.insert_index(ix2);
    MultiIndex<symm>::index_id s3 = midx.insert_index(ix3);
    MultiIndex<symm>::index_id s4 = midx.insert_index(ix4);


    maquis::cout << "* create sets" << std::endl;
    std::vector< std::pair<MultiIndex<symm>::index_id, bool> > set1_left(2), set1_right(2);
    set1_left[0] = std::make_pair(s1, true); set1_left[1] = std::make_pair(s2, true);
    set1_right[0] = std::make_pair(s3, true); set1_right[1] = std::make_pair(s4, true);
    MultiIndex<symm>::set_id set1 = midx.create_set(set1_left, set1_right);

    std::vector< std::pair<MultiIndex<symm>::index_id, bool> > set2_left(2), set2_right(2);
    set2_left[0] = std::make_pair(s1, true); set2_left[1] = std::make_pair(s3, false);
    set2_right[0] = std::make_pair(s4, true); set2_right[1] = std::make_pair(s2, false);
    MultiIndex<symm>::set_id set2 = midx.create_set(set2_left, set2_right);


    maquis::cout << "* common iterator" << std::endl;
    for(index_product_iterator<symm> it = midx.begin();
        it != midx.end();
        it++)
    {
        maquis::cout << *it << " = " << midx.get_coords(set1, *it)
                             << " or " << midx.get_coords(set2, *it) << std::endl;
    }


}


int main () {

    maquis::cout << "*** index_product_iterator()" << std::endl;
    test_iterator();


    maquis::cout << "*** MultiIndex" << std::endl;
    test_multi_index();


}

