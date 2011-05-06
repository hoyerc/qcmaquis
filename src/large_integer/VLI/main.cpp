
//#include "vli_config.h"
#include <iostream>

#define SIZE_BITS 256

#include <cuda.h>
#include <cublas.h>

#include "GpuManager.h"
#include "GpuManager.hpp"
#include "vli_number_cpu.hpp"
#include "vli_number_gpu.hpp"
#include "vli_vector_cpu.hpp"
#include "vli_vector_gpu.hpp"


#include "timings.h"

typedef int TYPE; 

int main (int argc, char * const argv[]) 
{

	
	gpu::gpu_manager* GPU;
	GPU->instance();
	TYPE FreqGPU = GPU->instance().GetDeviceProperties().clockRate;
	std::cout << " FreqGPU : " << FreqGPU << std::endl;

	vli::vli_cpu<TYPE> A(1);
	vli::vli_cpu<TYPE> B(1);	
	vli::vli_cpu<TYPE> C;


	vli::vli_gpu<TYPE> D(A);
	std::cout << D << std::endl;
	vli::vli_gpu<TYPE> E(B);
	std::cout << E << std::endl;
	vli::vli_gpu<TYPE> F(0);

	
	A+=B;	
   vli::detail::multiply_gpu(D.p(),E.p(),F.p(),1,vli::vli_gpu<TYPE>::size);
	
//	D*=E;

    F = D+E;    
	std::cout << " cpu "<< A << std::endl;
	std::cout << " gpu "<< F << std::endl;	

    std::cout<< E<<std::endl;    
/*	
	A = E*B;

	C = A;
	std::cout << A << std::endl;	

//	std::cout << B << std::endl;
*/
	vli::vli_vector<vli::vli_cpu<TYPE> > U(1000);
	vli::vli_vector<vli::vli_cpu<TYPE> > V(1000);
	vli::vli_vector<vli::vli_cpu<TYPE> > W(1000);

    U[4] = A;
    U[5] = B;
    W[0] = vli::vli_cpu<int>(1);

    vli::vli_vector_gpu<TYPE> X(U);
    vli::vli_vector_gpu<TYPE> Y(V);
    vli::vli_vector_gpu<TYPE> Z(W);

//    X[4] = A;
//    X[5] = B;


    Timer gpu_timer("GPU");
    Timer cpu_timer("CPU");

    gpu_timer.begin();
    for(unsigned int i=0; i< 100000; ++i)    
        Z = X + Z;
    gpu_timer.end();


    cpu_timer.begin();
    for(unsigned int i=0; i< 100000; ++i)    
        W = U + W;
    cpu_timer.end();
    

	GPU->instance().destructor();
	
    return 0;
}
