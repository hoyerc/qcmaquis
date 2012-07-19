/*
*Very Large Integer Library, License - Version 1.0 - May 3rd, 2012
*
*Timothee Ewart - University of Geneva, 
*Andreas Hehn - Swiss Federal Institute of technology Zurich.
*
*Permission is hereby granted, free of charge, to any person or organization
*obtaining a copy of the software and accompanying documentation covered by
*this license (the "Software") to use, reproduce, display, distribute,
*execute, and transmit the Software, and to prepare derivative works of the
*Software, and to permit third-parties to whom the Software is furnished to
*do so, all subject to the following:
*
*The copyright notices in the Software and this entire statement, including
*the above license grant, this restriction and the following disclaimer,
*must be included in all copies of the Software, in whole or in part, and
*all derivative works of the Software, unless such copies or derivative
*works are solely in the form of machine-executable object code generated by
*a source language processor.
*
*THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
*SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
*FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
*ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*DEALINGS IN THE SOFTWARE.
*/

#ifndef INNER_PRODUCT_GPU_BOOSTER_HPP
#define INNER_PRODUCT_GPU_BOOSTER_HPP

#include "vli/utils/gpu_manager.h"
#include "utils/timings.h"

#include "vli/detail/kernels_gpu.h"
#include <iostream>

namespace vli
{

    // a lot of forward declaration    
    template <class Coeff, class OrderSpecification, class Var0, class Var1, class Var2, class Var3>
    class polynomial;

    template <class Polynomial>
    class vector_polynomial;
   
    template <class VectorPolynomial>
    struct inner_product_result_type;
   
    template <class Coeff, class OrderSpecification, class Var0, class Var1, class Var2, class Var3>
    struct inner_product_result_type< vector_polynomial<polynomial<Coeff,OrderSpecification,Var0,Var1,Var2,Var3> > >; 

    namespace detail {

        template <class Coeff, class OrderSpecification, class Var0, class Var1, class Var2, class Var3>
        inline typename inner_product_result_type<vector_polynomial<polynomial<Coeff,OrderSpecification,Var0,Var1,Var2,Var3> > >::type
        inner_product_gpu(
             vector_polynomial<polynomial<Coeff,OrderSpecification,Var0,Var1,Var2,Var3> > const& v1,
             vector_polynomial<polynomial<Coeff,OrderSpecification,Var0,Var1,Var2,Var3> > const& v2
        ) {
        std::cout<<"inner_product: single thread + CUDA"<<std::endl;
        assert(v1.size() == v2.size());
        std::size_t size_v = v1.size();
        
        typename inner_product_result_type<vector_polynomial<polynomial<Coeff,OrderSpecification,Var0,Var1,Var2,Var3> > >::type res;
        typename inner_product_result_type<vector_polynomial<polynomial<Coeff,OrderSpecification,Var0,Var1,Var2,Var3> > >::type poly;

        std::size_t split = static_cast<std::size_t>(VLI_SPLIT_PARAM*v1.size());
        vli::detail::inner_product_vector_nvidia(vli_size_tag<Coeff::size,OrderSpecification::value, Var0, Var1, Var2, Var3>(),
                                                 split, &v1[0](0,0)[0], &v2[0](0,0)[0]);

        for(std::size_t i=split ; i < size_v ; ++i)
            res += v1[i]*v2[i];
        
        gpu::cu_check_error(cudaMemcpy((void*)&poly(0,0),(void*)get_polynomial(vli_size_tag<Coeff::size,OrderSpecification::value, Var0, Var1, Var2, Var3>()),
                            2*Coeff::size*2*(OrderSpecification::value+1)*2*(OrderSpecification::value+1)*sizeof(long),cudaMemcpyDeviceToHost),__LINE__);// this thing synchronizes 
        res += poly;

        return res;

        }

        template <class Coeff, class OrderSpecification, class Var0, class Var1, class Var2, class Var3>
        inline typename inner_product_result_type<vector_polynomial<polynomial<Coeff,OrderSpecification,Var0,Var1,Var2,Var3> > >::type
        inner_product_gpu_omp(
             vector_polynomial<polynomial<Coeff,OrderSpecification,Var0,Var1,Var2,Var3> > const& v1,
             vector_polynomial<polynomial<Coeff,OrderSpecification,Var0,Var1,Var2,Var3> > const& v2
        ) {
        std::cout<<"inner_product: multi thread + CUDA"<<std::endl;
        assert(v1.size() == v2.size());
        std::size_t size_v = v1.size();
        
        std::vector<typename inner_product_result_type<vector_polynomial<polynomial<Coeff,OrderSpecification,Var0,Var1,Var2,Var3> > >::type > res(omp_get_max_threads());
        typename inner_product_result_type<vector_polynomial<polynomial<Coeff,OrderSpecification,Var0,Var1,Var2,Var3> > >::type poly;

        std::size_t split = static_cast<std::size_t>(VLI_SPLIT_PARAM*v1.size());

        vli::detail::inner_product_vector_nvidia(vli_size_tag<Coeff::size,OrderSpecification::value, Var0, Var1, Var2, Var3>(),
                                                 split, &v1[0](0,0)[0], &v2[0](0,0)[0]);

        #pragma omp parallel for schedule(dynamic)
        for(std::size_t i=split ; i < size_v ; ++i)
            res[omp_get_thread_num()] += v1[i]*v2[i];
      
        for(int i=1; i < omp_get_max_threads(); ++i)
            res[0]+=res[i];

//        gpu::cu_check_error(cudaMemcpy((void*)&poly(0,0),(void*)get_polynomial(vli_size_tag<Coeff::size,OrderSpecification::value, Var0, Var1, Var2, Var3>()),
//                           2*Coeff::size*2*(OrderSpecification::value+1)*sizeof(long),cudaMemcpyDeviceToHost),__LINE__);// this thing synchronizes 
        

//        gpu::cu_check_error(cudaMemcpy((void*)&poly(0,0),(void*)get_polynomial(vli_size_tag<Coeff::size,OrderSpecification::value, Var0, Var1, Var2, Var3>()),
//                            2*Coeff::size*2*(OrderSpecification::value+1)*2*(OrderSpecification::value+1)*sizeof(long),cudaMemcpyDeviceToHost),__LINE__);// this thing synchronizes 

//            gpu::cu_check_error(cudaMemcpy((void*)&poly(0,0),(void*)get_polynomial(vli_size_tag<Coeff::size,OrderSpecification::value, Var0, Var1, Var2, Var3>()),
//                              2*Coeff::size*2*(OrderSpecification::value+1)*2*(OrderSpecification::value+1)*2*(OrderSpecification::value+1)*sizeof(long),cudaMemcpyDeviceToHost),__LINE__);// this thing synchronizes 

            gpu::cu_check_error(cudaMemcpy((void*)&poly(0,0),(void*)get_polynomial(vli_size_tag<Coeff::size,OrderSpecification::value, Var0, Var1, Var2, Var3>()),
                             2*Coeff::size*2*(OrderSpecification::value+1)*2*(OrderSpecification::value+1)*2*(OrderSpecification::value+1)*2*(OrderSpecification::value+1)*sizeof(long),cudaMemcpyDeviceToHost),__LINE__);// this thing synchronizes 

        res[0] += poly;

        return res[0];

        }

    } // end namespace detail
} // end namespace vli

#endif //INNER_PRODUCT_GPU_BOOSTER_HPP
