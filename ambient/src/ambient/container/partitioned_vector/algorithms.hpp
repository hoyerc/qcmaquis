/*
 * Copyright Institute for Theoretical Physics, ETH Zurich 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef AMBIENT_CONTAINER_PARTITIONED_VECTOR_ALGORITHMS_HPP
#define AMBIENT_CONTAINER_PARTITIONED_VECTOR_ALGORITHMS_HPP

#include "ambient/container/iterator/block_iterator.hpp"
#include "ambient/container/iterator/block_tuple_iterator.hpp"
#include "ambient/container/partitioned_vector/bitonic_sort.hpp"

namespace ambient {

    template<class InputIterator, class Function>
    void for_each(InputIterator begin, InputIterator end, Function fn){
        typedef block_iterator<typename InputIterator::container_type> iterator;
        for(iterator bit(begin,end); bit != end; ++bit)
            ambient::async([fn](typename iterator::block_type& block, size_t first, size_t second){
                                  std::for_each(block.begin()+first, block.begin()+second, fn);
                              }, *bit, bit.first, bit.second);
    }

    template<class InputIterator>
    void sequence(InputIterator begin, InputIterator end){
        typedef block_iterator<typename InputIterator::container_type> iterator;
        for(iterator bit(begin,end); bit != end; ++bit){
            size_t offset = bit.offset();
            ambient::async([offset](typename iterator::block_type& block, size_t first, size_t second){
                                  typename iterator::block_type::iterator it = block.begin()+first;
                                  std::for_each(it, block.begin()+second, [offset,it](typename iterator::block_type::value_type& e){
                                      e = offset + (&e - it);
                                  });
                              }, *bit, bit.first, bit.second);
        }
    }

    template<class ForwardIterator, class T>
    void fill(ForwardIterator begin, ForwardIterator end, const T& val){
        typedef block_iterator<typename ForwardIterator::container_type> iterator;
        for(iterator bit(begin,end); bit != end; ++bit){
            ambient::async([&val](typename iterator::block_type& block, size_t first, size_t second){
                                  std::fill(block.begin()+first, block.begin()+second, val);
                              }, *bit, bit.first, bit.second);
        }
    }

    template <class ForwardIterator, class Generator>
    void generate(ForwardIterator begin, ForwardIterator end, Generator gen){
        typedef block_iterator<typename ForwardIterator::container_type> iterator;
        for(iterator bit(begin,end); bit != end; ++bit){
            ambient::async([gen](typename iterator::block_type& block, size_t first, size_t second){
                                  std::generate(block.begin()+first, block.begin()+second, gen);
                              }, *bit, bit.first, bit.second);
        }
    }

    template <class InputIterator, class OtherInputIterator, class OutputIterator, class BinaryOperation>
    void transform(InputIterator begin1, InputIterator end1,
                   OtherInputIterator begin2, OutputIterator result,
                   BinaryOperation binary_op)
    {
        typedef block_tuple_iterator<InputIterator,
                                     OtherInputIterator,
                                     OutputIterator> iterator;

        for(iterator bit(begin1,begin2,result,end1-begin1); bit != end1; ++bit){
            ambient::async([binary_op](const typename iterator::template get_block_type<0>& block1, size_t first1, size_t second1,
                                          const typename iterator::template get_block_type<1>& block2, size_t first2,
                                                typename iterator::template get_block_type<2>& block3, size_t first3){
                                           std::transform(block1.cbegin()+first1, block1.cbegin()+second1,
                                                          block2.cbegin()+first2, block3.begin()+first3, binary_op);
                                         }, bit.template locate<0>(), bit.first[0], bit.second[0],
                                            bit.template locate<1>(), bit.first[1],
                                            bit.template locate<2>(), bit.first[2]);
        }
    }

    template <class InputIterator, class OutputIterator, class UnaryOperation>
    void transform(InputIterator begin, InputIterator end,
                   OutputIterator result, UnaryOperation op)
    {
        typedef block_tuple_iterator<InputIterator,
                                     OutputIterator> iterator;

        for(iterator bit(begin,result,end-begin); bit != end; ++bit){
            ambient::async([op](const typename iterator::template get_block_type<0>& block1, size_t first1, size_t second1,
                                         typename iterator::template get_block_type<1>& block2, size_t first2){
                                    std::transform(block1.cbegin()+first1, block1.cbegin()+second1,
                                                   block2.begin()+first2, op);
                                  }, bit.template locate<0>(), bit.first[0], bit.second[0],
                                     bit.template locate<1>(), bit.first[1]);
        }
    }

    template <class InputIterator, class OutputIterator>
    OutputIterator copy(InputIterator begin, InputIterator end, OutputIterator result){
        typedef block_tuple_iterator<InputIterator,
                                     OutputIterator> iterator;

        for(iterator bit(begin,result,end-begin); bit != end; ++bit){
            ambient::async([](const typename iterator::template get_block_type<0>& block1, size_t first1, size_t second1,
                                       typename iterator::template get_block_type<1>& block2, size_t first2){
                                  std::copy(block1.cbegin()+first1, block1.cbegin()+second1, block2.begin()+first2);
                                }, bit.template locate<0>(), bit.first[0], bit.second[0],
                                   bit.template locate<1>(), bit.first[1]);
        }
        return result+(end-begin);
    }

    template <class ForwardIterator, class T>
    void replace(ForwardIterator begin, ForwardIterator end, T old_value, T new_value){
        typedef block_iterator<typename ForwardIterator::container_type> iterator;
        for(iterator bit(begin,end); bit != end; ++bit){
            ambient::async([old_value,new_value](typename iterator::block_type& block, size_t first, size_t second){
                                  std::replace(block.begin()+first, block.begin()+second, old_value, new_value);
                              }, *bit, bit.first, bit.second);
        }
    }

    template <class InputIterator, class T>
    ambient::atomic<T> reduce(InputIterator begin, InputIterator end, T init){
        typedef block_iterator<typename InputIterator::container_type> iterator;
        typedef typename iterator::block_type block_type;

        iterator bit(begin,end);
        block_type reduced(bit.n_blocks());

        for(size_t b = 0; bit != end; ++bit){
            ambient::async([](const block_type& block, size_t first, size_t second, const block_type& res, size_t idx){
                                  const typename block_type::value_type* array = block.cbegin();
                                  const_cast<block_type&>(res)[idx] = __sec_reduce_add(array[first:second]);
                              }, *bit, bit.first, bit.second, reduced, b);
            b++;
        }

        ambient::atomic<T> res(init);
        ambient::async([](const block_type& block, ambient::atomic<T>& value){
                              const typename block_type::value_type* array = block.cbegin();
                              while(!ambient::locked_once(block)) continue;
                              value.set(value.get()+__sec_reduce_add(array[0:block.size()]));
                          }, reduced, res);
        return res; 
    }

    template <class InputIterator, class T, class BinaryOperation>
    ambient::atomic<T> reduce(InputIterator begin, InputIterator end, T init, BinaryOperation op){
        typedef block_iterator<typename InputIterator::container_type> iterator;
        typedef typename iterator::block_type block_type;

        iterator bit(begin,end);
        block_type reduced(bit.n_blocks());

        for(size_t b = 0; bit != end; ++bit){
            ambient::async([op](const block_type& block, size_t first, size_t second, const block_type& res, size_t idx){
                                  const typename block_type::value_type* array = block.cbegin();
                                  const_cast<block_type&>(res)[idx] = __sec_reduce(0, array[first:second], op);
                              }, *bit, bit.first, bit.second, reduced, b);
            b++;
        }

        ambient::atomic<T> res(init);
        ambient::async([op](const block_type& block, ambient::atomic<T>& value){
                              const typename block_type::value_type* array = block.cbegin();
                              while(!ambient::locked_once(block)) continue;
                              value.set(__sec_reduce(value.get(), array[0:block.size()], op));
                          }, reduced, res);
        return res;
    }

    template <class RandomAccessIterator, class Compare>
    void sort(RandomAccessIterator begin, RandomAccessIterator end, Compare comp){
        detail::bitonic_sort<RandomAccessIterator,Compare>::sort(begin, begin + (1 << __a_ceil(std::log2(end-begin))), end, comp);
    }

    template <class RandomAccessIterator>
    void sort(RandomAccessIterator begin, RandomAccessIterator end){
        struct {
            typedef typename std::iterator_traits<RandomAccessIterator>::value_type value_type;
            inline bool operator()(const value_type& a, const value_type& b){
                return a < b;
            }
        } compare;
        return sort(begin, end, compare);
    }

    template <class InputIterator, class T>
    ambient::atomic<InputIterator> find(InputIterator begin, InputIterator end, const T& val){
        typedef block_iterator<typename InputIterator::container_type> iterator;
        typedef typename iterator::block_type block_type;

        iterator bit(begin,end);
        ambient::vector<int> positions(bit.n_blocks(), -1);

        for(size_t b = 0; bit != end; ++bit){
            size_t offset = bit.offset();
            ambient::async([val,offset](const block_type& block, size_t first, size_t second, const ambient::vector<int>& res, size_t idx){
                                  const typename block_type::value_type* array = block.cbegin();
                                  for(size_t i = first; i < second; i++) if(array[i] == val)
                                      const_cast<ambient::vector<int>&>(res)[idx] = (int)(offset + i - first);
                              }, *bit, bit.first, bit.second, positions, b);
            b++;
        }
        ambient::atomic<InputIterator> res(end);
        ambient::async([val,begin](const ambient::vector<int>& positions, ambient::atomic<InputIterator>& match){
                              while(!ambient::locked_once(positions)) continue;
                              for(size_t i = 0; i < positions.size(); i++) if(positions[i] != -1){
                                  match.set(begin+positions[i]);
                                  return;
                              }
                          }, positions, res);
        return res;
    }

    // NOTE: input vector is denormalized after this operation.
    // Call "normalize" method or use async sizes of parts that are valid (caution).
    template <class InputIterator, class T>
    void remove(InputIterator begin, InputIterator end, const T& val){
        typedef block_iterator<typename InputIterator::container_type> iterator;
        typedef typename iterator::block_type block_type;

        for(iterator bit(begin,end); bit != end; ++bit){
            ambient::async([val](block_type& block, size_t first, size_t second){
                                  typename block_type::iterator r_end = std::remove(block.begin()+first, block.begin()+second, val);
                                  if(r_end == block.begin()+second) return;
                                  if(block.begin()+second != block.end())
                                      std::copy(block.begin()+second, block.end(), r_end);
                                  block.resize(block.size() - ((block.begin()+second)-r_end));
                              }, *bit, bit.first, bit.second);
        }
    }

    // NOTE: input vector is denormalized after this operation.
    // Call "normalize" method or use async sizes of parts that are valid (caution).
    // Known issue: will not check uniqueness of adjacent blocks boundary elements.
    template <class InputIterator, class BinaryPredicate>
    void unique(InputIterator begin, InputIterator end, BinaryPredicate pred){
        typedef block_iterator<typename InputIterator::container_type> iterator;
        typedef typename iterator::block_type block_type;

        for(iterator bit(begin,end); bit != end; ++bit){
            ambient::async([pred](block_type& block, size_t first, size_t second){
                                  typename block_type::iterator r_end = std::unique(block.begin()+first, block.begin()+second, pred);
                                  if(r_end == block.begin()+second) return;
                                  if(block.begin()+second != block.end())
                                      std::copy(block.begin()+second, block.end(), r_end);
                                  block.resize(block.size() - ((block.begin()+second)-r_end));
                              }, *bit, bit.first, bit.second);
        }
    }

    template <class InputIterator>
    void unique(InputIterator begin, InputIterator end){
        struct {
            typedef typename std::iterator_traits<InputIterator>::value_type value_type;
            inline bool operator()(const value_type& a, const value_type& b){
                return a == b;
            }
        } compare;
        return unique(begin, end, compare);
    }

}

#endif
