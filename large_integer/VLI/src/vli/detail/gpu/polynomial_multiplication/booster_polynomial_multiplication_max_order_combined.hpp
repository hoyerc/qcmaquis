#define VLI__ExtendStride extend_stride<Var0, Order>::value // 2*order+1
#include<algorithm>

namespace vli {
    namespace detail {
// all this could be really simplified as the max_order version
    template <typename BaseInt, std::size_t Size, class OrderSpecification, class Var0, class Var1, class Var2, class Var3>
    struct booster;

    // 4 variables
    template <typename BaseInt, std::size_t Size, unsigned int Order, class Var0, class Var1, class Var2, class Var3>
    struct booster<BaseInt, Size, max_order_combined<Order>, Var0, Var1, Var2, Var3 >{
    inline static __device__ void polynomial_multiplication_max_order( const unsigned int * __restrict__ in1,
                                                                const unsigned int * __restrict__ in2,
                                                                const unsigned int element_count,
                                                                unsigned int* __restrict__ out,
                                                                unsigned int* __restrict__ workblock_count_by_warp,
                                                                single_coefficient_task* __restrict__ execution_plan) {
        const unsigned int local_thread_id = threadIdx.x;
        const unsigned int element_id = blockIdx.x;
        
        unsigned int c1[Size],c2[Size];
        unsigned int res[2*Size];

        unsigned int res1[2*Size];
        
        unsigned int iteration_count = workblock_count_by_warp[local_thread_id / 32];
        
        const unsigned int input_elem_offset = element_id *  vli::detail::max_order_combined_helpers::size<vli::detail::num_of_variables_helper<Var0,Var1,Var2,Var3 >::value+1, Order>::value * Size;
        
        for(unsigned int iteration_id = 0; iteration_id < iteration_count; ++iteration_id) {
            single_coefficient_task task = execution_plan[local_thread_id + (iteration_id * MulBlockSize<max_order_combined<Order>, Var0, Var1, Var2, Var3>::value)];
            const unsigned int step_count = task.step_count;
        
            if (step_count > 0) {
                #pragma unroll
                for(unsigned int i = 0; i < 2*Size; ++i)
                    res[i] = 0;
                
                const int output_degree_x = task.output_degree_x;
                const int output_degree_y = task.output_degree_y;
                const int output_degree_z = task.output_degree_z;
                const int output_degree_w = task.output_degree_w;

                for(int current_degree_x=max(0, output_degree_x-(int)Order); current_degree_x <= min(output_degree_x,(int)Order) ;++current_degree_x)
                    for(int current_degree_y=max(0, output_degree_x+output_degree_y-(int)Order-current_degree_x); current_degree_y <= min(output_degree_y,(int)Order-current_degree_x) ; ++current_degree_y)
                        for(int current_degree_z=max(0, output_degree_x+output_degree_y+output_degree_z-(int)Order-current_degree_x-current_degree_y); current_degree_z <= min(output_degree_z,(int)Order-current_degree_x - current_degree_y) ; ++current_degree_z)
                            for(int current_degree_w=max(0, output_degree_x+output_degree_y+output_degree_z+output_degree_w-(int)Order-current_degree_x-current_degree_y-current_degree_z); current_degree_w <= min(output_degree_w,(int)Order-current_degree_x - current_degree_y - current_degree_z) ; ++current_degree_w){
                
                             unsigned int in_polynomial_offset1 = ( //0____0'
                                                                   (current_degree_x*(2*(Order+1)+3-current_degree_x)*(2*(Order+1)*(Order+1)+6*(Order+1)+2 +current_degree_x*current_degree_x -2*(Order+1)*current_degree_x
                                                                   - 3*current_degree_x))/24
                                                                   +(current_degree_y*(current_degree_y*current_degree_y - 3*current_degree_y*((Order+1)+1-current_degree_x) + 3*((Order+1)-current_degree_x)*((Order+1)+2-current_degree_x)+2))/6
                                                                   +((Order+1)-current_degree_x-current_degree_y)*current_degree_z - (current_degree_z*current_degree_z-current_degree_z)/2 + current_degree_w
                                                                  ) * Size + input_elem_offset;
                             
                             unsigned int in_polynomial_offset2 = ( //*.* 
                                                                   ((output_degree_x-current_degree_x)*(2*(Order+1)+3-(output_degree_x-current_degree_x))*(2*(Order+1)*(Order+1)+6*(Order+1)+2
                                                                     + (output_degree_x-current_degree_x)*(output_degree_x-current_degree_x) -2*(Order+1)*(output_degree_x-current_degree_x)
                                                                     - 3*(output_degree_x-current_degree_x)))/24
                                                                   +((output_degree_y-current_degree_y)*((output_degree_y-current_degree_y)*(output_degree_y-current_degree_y) - 3*(output_degree_y-current_degree_y)*((Order+1)+1-(output_degree_x-current_degree_x))
                                                                     + 3*((Order+1)-(output_degree_x-current_degree_x))*((Order+1)+2-(output_degree_x-current_degree_x))+2))/6
                                                                   +((Order+1)-(output_degree_x-current_degree_x)-(output_degree_y-current_degree_y))*(output_degree_z-current_degree_z) 
                                                                     - ((output_degree_z-current_degree_z)*(output_degree_z-current_degree_z)-(output_degree_z-current_degree_z))/2 + (output_degree_w-current_degree_w)
                                                                  ) * Size + input_elem_offset;
                             
                             #pragma unroll
                             for(unsigned int i = 0; i < Size; ++i)
                                 c1[i] = in1[in_polynomial_offset1 + i];
                             
                             #pragma unroll
                             for(unsigned int i = 0; i < Size; ++i)
                                 c2[i] = in2[in_polynomial_offset2 + i];
                             
                             #pragma unroll
                             for(unsigned int i = 0; i < 2*Size; ++i)
                                 res1[i] = 0;
                             
                             multiplies<BaseInt, Size>(res, res1, c1, c2); // the multiplication using boost pp
                    } //end big loop

                unsigned int coefficient_id = (output_degree_x*(2*VLI__ExtendStride+3-output_degree_x)*(2*VLI__ExtendStride*VLI__ExtendStride+6*VLI__ExtendStride+2 +output_degree_x*output_degree_x -2*VLI__ExtendStride*output_degree_x
                                              - 3*output_degree_x))/24
                                              +(output_degree_y*(output_degree_y*output_degree_y - 3*output_degree_y*(VLI__ExtendStride+1-output_degree_x) + 3*(VLI__ExtendStride-output_degree_x)*(VLI__ExtendStride+2-output_degree_x)+2))/6
                                              +(VLI__ExtendStride-output_degree_x-output_degree_y)*output_degree_z - (output_degree_z*output_degree_z-output_degree_z)/2 + output_degree_w;
                
                unsigned int * out2 = out + (coefficient_id * element_count *2* Size) + element_id; // coefficient->int_degree->element_id
                #pragma unroll
                for(unsigned int i = 0; i < 2*Size; ++i) {
                        // This is a strongly compute-bound kernel,
                        // so it is fine to waste memory bandwidth by using non-coalesced writes in order to have less instructions,
                        //     less synchronization points, less shared memory used (and thus greater occupancy) and greater scalability.
                        *out2 = res[i];
                        out2 += element_count;
                }
            } // end step count
        } // end for it

        }; // end function
    }; //end struct

    // 3 variables
    template <typename BaseInt, std::size_t Size, unsigned int Order, class Var0, class Var1, class Var2>
    struct booster<BaseInt, Size, max_order_combined<Order>, Var0, Var1, Var2,vli::no_variable>{
    inline static __device__ void polynomial_multiplication_max_order( const unsigned int * __restrict__ in1,
                                                                const unsigned int * __restrict__ in2,
                                                                const unsigned int element_count,
                                                                unsigned int* __restrict__ out,
                                                                unsigned int* __restrict__ workblock_count_by_warp,
                                                                single_coefficient_task* __restrict__ execution_plan) {
        const unsigned int local_thread_id = threadIdx.x;
        const unsigned int element_id = blockIdx.x;
        
        unsigned int c1[Size],c2[Size];
        unsigned int res[2*Size];
        unsigned int res1[2*Size];
        
        unsigned int iteration_count = workblock_count_by_warp[local_thread_id / 32];
        
        const unsigned int input_elem_offset = element_id * vli::detail::max_order_combined_helpers::size<vli::detail::num_of_variables_helper<Var0,Var1,Var2, vli::no_variable >::value+1, Order>::value  * Size;
        
        for(unsigned int iteration_id = 0; iteration_id < iteration_count; ++iteration_id) {
            single_coefficient_task task = execution_plan[local_thread_id + (iteration_id * MulBlockSize<max_order_combined<Order>, Var0, Var1, Var2, vli::no_variable>::value)];
            const unsigned int step_count = task.step_count;
        
            if (step_count > 0) {
                #pragma unroll
                for(unsigned int i = 0; i < 2*Size; ++i)
                    res[i] = 0;
                
                const int output_degree_x = task.output_degree_x;
                const int output_degree_y = task.output_degree_y;
                const int output_degree_z = task.output_degree_z;

                for(int current_degree_x=max(0, output_degree_x-(int)Order); current_degree_x <= min(output_degree_x,(int)Order) ;++current_degree_x)
                    for(int current_degree_y=max(0, output_degree_x+output_degree_y-(int)Order-current_degree_x); current_degree_y <= min(output_degree_y,(int)Order-current_degree_x) ; ++current_degree_y)
                        for(int current_degree_z=max(0, output_degree_x+output_degree_y+output_degree_z-(int)Order-current_degree_x-current_degree_y); current_degree_z <= min(output_degree_z,(int)Order-current_degree_x - current_degree_y) ; ++current_degree_z){
                
                        unsigned int in_polynomial_offset1 = (
                                                              (current_degree_x*(current_degree_x*current_degree_x - 3*current_degree_x*((Order+1)+1)
                                                              + 3*(Order+1)*((Order+1)+2) +2))/6
                                                              + ((Order+1) - current_degree_x)*current_degree_y - (current_degree_y*current_degree_y-current_degree_y)/2 + current_degree_z
                                                             ) * Size + input_elem_offset;
      
                        
                        unsigned int in_polynomial_offset2 = ( 
                                                              ((output_degree_x-current_degree_x)*((output_degree_x-current_degree_x)*(output_degree_x-current_degree_x) - 3*(output_degree_x-current_degree_x)*((Order+1)+1)
                                                              + 3*(Order+1)*((Order+1)+2) +2))/6
                                                              + ((Order+1) - (output_degree_x-current_degree_x))*(output_degree_y-current_degree_y) - ((output_degree_y-current_degree_y)*(output_degree_y-current_degree_y)-(output_degree_y-current_degree_y))/2
                                                              + (output_degree_z-current_degree_z)
                                                             ) * Size + input_elem_offset;
                          
                        #pragma unroll
                        for(unsigned int i = 0; i < Size; ++i)
                            c1[i] = in1[in_polynomial_offset1 + i];
                    
                        #pragma unroll
                        for(unsigned int i = 0; i < Size; ++i)
                            c2[i] = in2[in_polynomial_offset2 + i];
                    
                        #pragma unroll
                        for(unsigned int i = 0; i < 2*Size; ++i)
                            res1[i] = 0;
                     
                        multiplies<BaseInt, Size>(res, res1, c1, c2); // the multiplication using boost pp
                }
                
                unsigned int coefficient_id =  (output_degree_x*(output_degree_x*output_degree_x - 3*output_degree_x*(VLI__ExtendStride+1)
                                               + 3*VLI__ExtendStride*(VLI__ExtendStride+2) +2))/6
                                               + (VLI__ExtendStride - output_degree_x)*output_degree_y - (output_degree_y*output_degree_y-output_degree_y)/2 + output_degree_z;
                
                unsigned int * out2 = out + (coefficient_id * element_count *2* Size) + element_id; // coefficient->int_degree->element_id
                #pragma unroll
                for(unsigned int i = 0; i < 2*Size; ++i) {
                        // This is a strongly compute-bound kernel,
                        // so it is fine to waste memory bandwidth by using non-coalesced writes in order to have less instructions,
                        //     less synchronization points, less shared memory used (and thus greater occupancy) and greater scalability.
                        *out2 = res[i];
                        out2 += element_count;
                }
            } // end step count
        } // end for it

        }; //end function
    }; //end struct

    // 2 variables
    template <typename BaseInt, std::size_t Size, unsigned int Order, class Var0, class Var1>
    struct booster<BaseInt, Size, max_order_combined<Order>, Var0, Var1, vli::no_variable, vli::no_variable>{
    inline static __device__ void polynomial_multiplication_max_order( const unsigned int * __restrict__ in1,
                                                                const unsigned int * __restrict__ in2,
                                                                const unsigned int element_count,
                                                                unsigned int* __restrict__ out,
                                                                unsigned int* __restrict__ workblock_count_by_warp,
                                                                single_coefficient_task* __restrict__ execution_plan) {
        const unsigned int local_thread_id = threadIdx.x;
        const unsigned int element_id = blockIdx.x;
        
        unsigned int c1[Size],c2[Size];
        unsigned int res[2*Size];
        unsigned int res1[2*Size];
        
        unsigned int iteration_count = workblock_count_by_warp[local_thread_id / 32];
        
        const unsigned int input_elem_offset = element_id * vli::detail::max_order_combined_helpers::size<vli::detail::num_of_variables_helper<Var0,Var1,vli::no_variable, vli::no_variable >::value+1, Order>::value  * Size;
        
        for(unsigned int iteration_id = 0; iteration_id < iteration_count; ++iteration_id) {
            single_coefficient_task task = execution_plan[local_thread_id + (iteration_id * MulBlockSize<max_order_combined<Order>, Var0, Var1, vli::no_variable, vli::no_variable>::value)];
            const unsigned int step_count = task.step_count;
        
            if (step_count > 0) {
                #pragma unroll
                for(unsigned int i = 0; i < 2*Size; ++i)
                    res[i] = 0;
                
                const int output_degree_x = task.output_degree_x;
                const int output_degree_y = task.output_degree_y;
                //note min and max are in numeric 
                for(int current_degree_x=max(0, output_degree_x-(int)Order); current_degree_x <= min(output_degree_x,(int)Order) ;++current_degree_x)
                    for(int current_degree_y=max(0, output_degree_x+output_degree_y-(int)Order-current_degree_x); current_degree_y <= min(output_degree_y,(int)Order-current_degree_x) ; ++current_degree_y){
                        unsigned int in_polynomial_offset1 = (  
                                                               (Order+1)*current_degree_x - (current_degree_x*current_degree_x-current_degree_x)/2 + current_degree_y
                                                             ) * Size + input_elem_offset;
                        
                        unsigned int in_polynomial_offset2 = ( 
                                                               (Order+1)*(output_degree_x-current_degree_x) - ((output_degree_x-current_degree_x)*(output_degree_x-current_degree_x)-(output_degree_x-current_degree_x))/2 + (output_degree_y-current_degree_y)
                                                             ) * Size + input_elem_offset;
                    
                        #pragma unroll
                        for(unsigned int i = 0; i < Size; ++i)
                            c1[i] = in1[in_polynomial_offset1 + i];
                    
                        #pragma unroll
                        for(unsigned int i = 0; i < Size; ++i)
                            c2[i] = in2[in_polynomial_offset2 + i];
                    
                        #pragma unroll
                        for(unsigned int i = 0; i < 2*Size; ++i)
                            res1[i] = 0;
                     
                        multiplies<BaseInt, Size>(res, res1, c1, c2); // the multiplication using boost pp
                    }                   

                unsigned int coefficient_id = VLI__ExtendStride*output_degree_x - (output_degree_x*output_degree_x-output_degree_x)/2 + output_degree_y;
                unsigned int * out2 = out + (coefficient_id * element_count *2* Size) + element_id; // coefficient->int_degree->element_id
                #pragma unroll
                for(unsigned int i = 0; i < 2*Size; ++i) {
                        // This is a strongly compute-bound kernel,
                        // so it is fine to waste memory bandwidth by using non-coalesced writes in order to have less instructions,
                        //     less synchronization points, less shared memory used (and thus greater occupancy) and greater scalability.
                        *out2 = res[i];
                        out2 += element_count;
                }
            } // end step count
        } // end for it

        }; //end function
    };// end struct

    // 1 variables
    template <typename BaseInt, std::size_t Size, unsigned int Order, class Var0>
    struct booster<BaseInt, Size, max_order_combined<Order>, Var0, vli::no_variable,vli::no_variable,vli::no_variable>{
    inline static __device__ void polynomial_multiplication_max_order( const unsigned int * __restrict__ in1,
                                                                const unsigned int * __restrict__ in2,
                                                                const unsigned int element_count,
                                                                unsigned int* __restrict__ out,
                                                                unsigned int* __restrict__ workblock_count_by_warp,
                                                                single_coefficient_task* __restrict__ execution_plan) {
        const unsigned int local_thread_id = threadIdx.x;
        const unsigned int element_id = blockIdx.x;
        
        unsigned int c1[Size],c2[Size];
        unsigned int res[2*Size];
        unsigned int res1[2*Size];
        
        unsigned int iteration_count = workblock_count_by_warp[local_thread_id / 32];
        
        const unsigned int input_elem_offset = element_id * stride<Var0,Order>::value * Size;
        
        for(unsigned int iteration_id = 0; iteration_id < iteration_count; ++iteration_id) {
            single_coefficient_task task = execution_plan[local_thread_id + (iteration_id * MulBlockSize<max_order_combined<Order>, Var0, vli::no_variable, vli::no_variable, vli::no_variable>::value)];
            const unsigned int step_count = task.step_count;
        
            if (step_count > 0) {
                #pragma unroll
                for(unsigned int i = 0; i < 2*Size; ++i)
                    res[i] = 0;
                
                const unsigned int output_degree_x = task.output_degree_x;
                
                unsigned int current_degree_x = output_degree_x > Order ? output_degree_x - Order : 0;
                
                for(unsigned int step_id = 0; step_id < step_count; ++step_id) {
                
                    unsigned int in_polynomial_offset1 = ( + current_degree_x
                                                         ) * Size + input_elem_offset;
                    
                    unsigned int in_polynomial_offset2 = ( + (output_degree_x - current_degree_x)
                                                         ) * Size + input_elem_offset;
                
                    #pragma unroll
                    for(unsigned int i = 0; i < Size; ++i)
                        c1[i] = in1[in_polynomial_offset1 + i];
                
                    #pragma unroll
                    for(unsigned int i = 0; i < Size; ++i)
                        c2[i] = in2[in_polynomial_offset2 + i];
                
                    #pragma unroll
                    for(unsigned int i = 0; i < 2*Size; ++i)
                        res1[i] = 0;
                 
                    multiplies<BaseInt, Size>(res, res1, c1, c2); // the multiplication using boost pp
                 
                    current_degree_x++;
                }
                
                unsigned int coefficient_id =  output_degree_x;
                
                unsigned int * out2 = out + (coefficient_id * element_count *2* Size) + element_id; // coefficient->int_degree->element_id
                #pragma unroll
                for(unsigned int i = 0; i < 2*Size; ++i) {
                        // This is a strongly compute-bound kernel,
                        // so it is fine to waste memory bandwidth by using non-coalesced writes in order to have less instructions,
                        //     less synchronization points, less shared memory used (and thus greater occupancy) and greater scalability.
                        *out2 = res[i];
                        out2 += element_count;
                }
            } // end step count
        } // end for it

        };  // end function
    }; // end struct


    }//end namesoace detail
}//end namespace vli
