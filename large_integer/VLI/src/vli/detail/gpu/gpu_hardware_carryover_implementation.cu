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

#include <vector>
#include <algorithm>
#include <iostream>

#include <boost/mpl/assert.hpp>

#include "vli/detail/gpu/variables_gpu.h"
#include "vli/detail/kernels_gpu.h" // signature interface with cpu
#include "vli/detail/gpu/gpu_hardware_carryover_implementation.h" 
#include "vli/detail/gpu/gpu_mem_block.h"
#include "vli/detail/gpu/kernels_gpu_neg_asm.hpp"
#include "vli/detail/gpu/kernels_gpu_add_asm.hpp"
#include "vli/detail/gpu/kernels_gpu_mul_asm.hpp"
#include "vli/detail/gpu/vli_number_gpu_function_hooks.hpp"

namespace vli {
    namespace detail {

    template <typename BaseInt, std::size_t Size, unsigned int Order, class Var0, class Var1, class Var2, class Var3>
    __global__ void
    __launch_bounds__(MulBlockSize<Order, Var0, Var1, Var2, Var3>::value , 2)
    polynomial_mul_full(
    	const unsigned int * __restrict__ in1,
    	const unsigned int * __restrict__ in2,
        const unsigned int element_count,
        unsigned int* __restrict__ out,
        unsigned int* __restrict__ workblock_count_by_warp,
        single_coefficient_task* __restrict__ execution_plan)
    {
            BOOST_MPL_ASSERT_MSG( (2*stride<Var0,Order>::value*stride<Var1,Order>::value*stride<Var2,Order>::value*stride<Var3,Order>::value*Size*sizeof(BaseInt)) <  0xc000, NOT_ENOUGHT_SHARED_MEM_48KB_MAX, (Var0,Var1,Var2,Var3));
            
	    __shared__ unsigned int in_buffer1[stride<Var0,Order>::value * stride<Var1,Order>::value *  stride<Var2,Order>::value * stride<Var3,Order>::value * Size];
	    __shared__ unsigned int in_buffer2[stride<Var0,Order>::value * stride<Var1,Order>::value *  stride<Var2,Order>::value * stride<Var3,Order>::value * Size];
               
	    const unsigned int local_thread_id = threadIdx.x;
	    const unsigned int element_id = blockIdx.x;

	    // Copy both input polynomials into the shared memory
	    {
		    const unsigned int * in_shifted1 = in1 + (element_id * stride<Var0,Order>::value * stride<Var1,Order>::value *  stride<Var2,Order>::value * stride<Var3,Order>::value  * Size);
		    const unsigned int * in_shifted2 = in2 + (element_id * stride<Var0,Order>::value * stride<Var1,Order>::value *  stride<Var2,Order>::value * stride<Var3,Order>::value  * Size);
		    unsigned int index_id = local_thread_id;
                    #pragma unroll
		    for(unsigned int i = 0; i < ( stride<Var0,Order>::value * stride<Var1,Order>::value *  stride<Var2,Order>::value * stride<Var3,Order>::value * Size) / MulBlockSize<Order, Var0, Var1, Var2, Var3>::value; ++i) {
                            // pbs starts there
			    unsigned int coefficient_id = index_id / Size;
			    unsigned int degree_id = index_id % Size;
			    unsigned int current_degree_y = coefficient_id / (Order+1);
			    unsigned int current_degree_x = coefficient_id % (Order+1);
			    unsigned int local_index_id = current_degree_x + (current_degree_y * OrderPadded<Order>::value) + (degree_id * (OrderPadded<Order>::value *(Order+1)));
			    in_buffer1[local_index_id] = in_shifted1[index_id];
			    in_buffer2[local_index_id] = in_shifted2[index_id];
			    index_id += MulBlockSize<Order, Var0, Var1, Var2, Var3>::value ;
		    }

		    if (index_id < ((Order+1) * (Order+1) * Size)) {
			    unsigned int coefficient_id = index_id / Size;
			    unsigned int degree_id = index_id % Size;
			    unsigned int current_degree_y = coefficient_id / (Order+1);
			    unsigned int current_degree_x = coefficient_id % (Order+1);
			    unsigned int local_index_id = current_degree_x + (current_degree_y * OrderPadded<Order>::value) + (degree_id * (OrderPadded<Order>::value * (Order+1)));
			    in_buffer1[local_index_id] = in_shifted1[index_id];
			    in_buffer2[local_index_id] = in_shifted2[index_id];
		    }
		    __syncthreads();
	    }

	    unsigned int c1[Size],c2[Size];
	    unsigned int res[2*Size];
	    unsigned int res1[2*Size];

	    unsigned int iteration_count = workblock_count_by_warp[local_thread_id / 32];

	    for(unsigned int iteration_id = 0; iteration_id < iteration_count; ++iteration_id)
	    {
		    single_coefficient_task task = execution_plan[local_thread_id + (iteration_id * MulBlockSize<Order, Var0, Var1, Var2, Var3>::value)];
		    const unsigned int step_count = task.step_count;

		    if (step_count > 0)
		    {
			    const unsigned int output_degree_y = task.output_degree_y;
			    const unsigned int output_degree_x = task.output_degree_x;

                            #pragma unroll
			    for(unsigned int i = 0; i < 2*Size; ++i)
				    res[i] = 0;

			    const unsigned int start_degree_x_inclusive = output_degree_x > Order ? output_degree_x - Order : 0;
			    const unsigned int end_degree_x_inclusive = output_degree_x < (Order+1) ? output_degree_x : Order;
			    unsigned int current_degree_x = start_degree_x_inclusive;
			    unsigned int current_degree_y = output_degree_y > Order  ? output_degree_y - Order : 0;

			    for(unsigned int step_id = 0; step_id < step_count; ++step_id) {
				    unsigned int * in_polynomial1 = in_buffer1 + current_degree_x + (current_degree_y * OrderPadded<Order>::value);
				    unsigned int * in_polynomial2 = in_buffer2 + (output_degree_x - current_degree_x) + ((output_degree_y - current_degree_y) * OrderPadded<Order>::value);

                                    #pragma unroll
				    for(unsigned int i = 0; i < 2*Size; ++i)
					    res1[i] = 0;

                                    #pragma unroll
				    for(unsigned int degree1 = 0; degree1 < Size; ++degree1)
					    c1[degree1] = in_polynomial1[degree1 * (OrderPadded<Order>::value * (Order+1))];

                                    #pragma unroll
				    for(unsigned int degree2 = 0; degree2 < Size; ++degree2)
					    c2[degree2] = in_polynomial2[degree2  * (OrderPadded<Order>::value * (Order+1))];

                                    multiplies<BaseInt, Size>(res, res1, c1, c2);

				    // Calculate the next pair of input coefficients to be multiplied and added to the result
				    current_degree_x++;
				    if (current_degree_x > end_degree_x_inclusive) {
					    current_degree_x = start_degree_x_inclusive;
					    current_degree_y++;
				    }
			    }

			    unsigned int coefficient_id = output_degree_y * (Order*2+1) + output_degree_x;
			    unsigned int * out2 = out + (coefficient_id * element_count *2* Size) + element_id; // coefficient->int_degree->element_id
                            #pragma unroll
			    for(unsigned int i = 0; i < 2*Size; ++i) {
				    // This is a strongly compute-bound kernel,
				    // so it is fine to waste memory bandwidth by using non-coalesced writes in order to have less instructions,
				    //     less synchronization points, less shared memory used (and thus greater occupancy) and greater scalability.
				    *out2 = res[i];
				    out2 += element_count;
			    }
		    } // if (step_count > 0)
	    } //for(unsigned int iteration_id
    }

    template <typename BaseInt, std::size_t Size, unsigned int Order>
    __global__ void
    __launch_bounds__(SUM_BLOCK_SIZE, 2)
    polynomial_sum_intermediate_full(
		    const unsigned int * __restrict__ intermediate,
		    const unsigned int element_count,
		    unsigned int * __restrict__ out)
    {
	    __shared__ unsigned int buf[SUM_BLOCK_SIZE * 2*OrderPadded<Order>::value];

	    unsigned int local_thread_id = threadIdx.x;
	    unsigned int coefficient_id = blockIdx.x;

	    unsigned int * t1 = buf + (local_thread_id * 2*OrderPadded<Order>::value);
            #pragma unroll
	    for(unsigned int i = 0; i < 2*Size; ++i)
		    t1[i] = 0;

	    const unsigned int * in2 = intermediate + (coefficient_id * element_count *2*Size) + local_thread_id;
	    for(unsigned int element_id = local_thread_id; element_id < element_count; element_id += SUM_BLOCK_SIZE)
	    {
                 asm( "add.cc.u32   %0 , %0 , %1 ; \n\t" : "+r"(t1[0]):"r"(in2[0])); 
                 #pragma unroll
                 for(int i=1; i < 2*(Order+1); ++i)
                     asm( "addc.cc.u32  %0 , %0 , %1 ; \n\t" : "+r"(t1[i]):"r"(in2[i*element_count])); 

     	         in2 += SUM_BLOCK_SIZE;
	    }

            #pragma unroll
	    for(unsigned int stride = SUM_BLOCK_SIZE >> 1; stride > 0; stride >>= 1) {
		    __syncthreads();
		    if (local_thread_id < stride) {
			    unsigned int * t2 = buf + ((local_thread_id + stride) * 2 *OrderPadded<Order>::value);
                            add<BaseInt, 2*Size>(t1,t2);
		    }
	    }

	    if (local_thread_id == 0) {
  		    unsigned int * out2 = out+(coefficient_id*2*Size); 
                    #pragma unroll
		    for(unsigned int i=0; i<2*Size; ++i)
			    out2[i] = buf[i];
	    }
    }

    template <typename BaseInt, std::size_t Size, unsigned int Order, class Var0, class Var1, class Var2, class Var3>
    void inner_product_vector_nvidia(std::size_t VectorSize, BaseInt const* A, BaseInt const* B) {
	    gpu_memblock<BaseInt, Var0, Var1, Var2, Var3>* gm = gpu_memblock<BaseInt, Var0, Var1, Var2, Var3>::Instance(); // allocate memory for vector input, intermediate and output, singleton only one time 
            gm->resize(Size,Order,VectorSize); // we resize if the block is larger
	    gpu_hardware_carryover_implementation<BaseInt, Size, Order, Var0, Var1, Var2, Var3>* ghc = gpu_hardware_carryover_implementation<BaseInt, Size, Order, Var0, Var1, Var2, Var3>::Instance(); // calculate the different packet, singleton only one time 
	    cudaMemcpyAsync((void*)gm->V1Data_,(void*)A,VectorSize*stride<Var0,Order>::value*stride<Var1,Order>::value*stride<Var2,Order>::value*stride<Var3,Order>::value*Size*sizeof(BaseInt),cudaMemcpyHostToDevice);
	    cudaMemcpyAsync((void*)gm->V2Data_,(void*)B,VectorSize*stride<Var0,Order>::value*stride<Var1,Order>::value*stride<Var2,Order>::value*stride<Var3,Order>::value*Size*sizeof(BaseInt),cudaMemcpyHostToDevice);

	    {
                dim3 grid(VectorSize) ;
                dim3 threads(MulBlockSize<Order, Var0, Var1, Var2, Var3>::value);
                polynomial_mul_full<BaseInt, Size, Order, Var0, Var1, Var2, Var3><<<grid,threads>>>(gm->V1Data_, gm->V2Data_,VectorSize, gm->VinterData_,ghc->workblock_count_by_warp_,ghc->execution_plan_);
	    }

	    {
                dim3 grid(extend_stride<Var0, Order>::value*extend_stride<Var1, Order>::value*extend_stride<Var2, Order>::value*extend_stride<Var3, Order>::value);
                dim3 threads(SUM_BLOCK_SIZE);
                polynomial_sum_intermediate_full<BaseInt, Size, Order><<<grid,threads>>>(gm->VinterData_, VectorSize, gm->PoutData_);
	    }
    } 

    template <typename BaseInt, std::size_t Size, unsigned int Order, class Var0, class Var1, class Var2, class Var3>
    BaseInt* get_polynomial(){
	    gpu_memblock<BaseInt, Var0, Var1, Var2, Var3>* gm = gpu_memblock<BaseInt, Var0, Var1, Var2, Var3>::Instance(); // I just get the mem pointer
	    return gm->PoutData_;
    }

    //to do clean memory for gpu
   

#define VLI_IMPLEMENT_GPU_FUNCTIONS(TYPE, VLI_SIZE, POLY_ORDER, VAR) \
    void inner_product_vector_nvidia(vli_size_tag<VLI_SIZE, POLY_ORDER, EXPEND_VAR(VAR)>, std::size_t vector_size, TYPE const* A, TYPE const* B) \
    {inner_product_vector_nvidia<unsigned int, 2*VLI_SIZE, POLY_ORDER, EXPEND_VAR(VAR) >(vector_size, const_cast<unsigned int*>(reinterpret_cast<unsigned int const*>(A)), const_cast<unsigned int*>(reinterpret_cast<unsigned int const*>(B)));} \
    unsigned int* get_polynomial(vli_size_tag<VLI_SIZE, POLY_ORDER, EXPEND_VAR(VAR)>) /* cuda mem allocated on unsigned int (gpu_mem_block class), do not change the return type */ \
    {return get_polynomial<unsigned int, 2*VLI_SIZE, POLY_ORDER, EXPEND_VAR(VAR)>();}\

#define VLI_IMPLEMENT_GPU_FUNCTIONS_FOR(r, data, BASEINT_SIZE_ORDER_VAR_TUPLE) \
    VLI_IMPLEMENT_GPU_FUNCTIONS( BOOST_PP_TUPLE_ELEM(4,0,BASEINT_SIZE_ORDER_VAR_TUPLE), BOOST_PP_TUPLE_ELEM(4,1,BASEINT_SIZE_ORDER_VAR_TUPLE), BOOST_PP_TUPLE_ELEM(4,2,BASEINT_SIZE_ORDER_VAR_TUPLE), BOOST_PP_TUPLE_ELEM(4,3,BASEINT_SIZE_ORDER_VAR_TUPLE) )

    BOOST_PP_SEQ_FOR_EACH(VLI_IMPLEMENT_GPU_FUNCTIONS_FOR, _, VLI_COMPILE_BASEINT_SIZE_ORDER_VAR_TUPLE_SEQ)

#undef VLI_IMPLEMENT_GPU_FUNCTIONS_FOR
#undef VLI_IMPLEMENT_GPU_FUNCTIONS

    }
}
