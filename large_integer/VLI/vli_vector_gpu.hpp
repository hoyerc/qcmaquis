
#ifndef VLI_VECTOR_GPU_HPP 
#define VLI_VECTOR_GPU_HPP

#include <ostream>
#include <cassert>
#include "vli_vector_cpu.hpp"
#include "vli_number_gpu.hpp"
#include "detail/vli_vector_gpu_function_hooks.hpp"

namespace vli {

template <class BaseInt>
class vli_vector_gpu
{
    public:
    typedef BaseInt base_int_type;
    typedef std::size_t size_type;
    enum { vli_size = SIZE_BITS/(8*sizeof(BaseInt)) };

    class proxy
    {
        /**
          proxy objects to access elements of the vector
         */ 
        public:
            proxy(vli_vector_gpu& v, size_type i)
                :data_(v.p()+static_cast<std::ptrdiff_t>(i*vli_size))
            {
            }

            // TODO extend features like += etc.
            operator vli_gpu<BaseInt>() const
            {
                vli_gpu<BaseInt> vli;
                gpu::check_error( cudaMemcpy( vli.p(), data_, vli_size*sizeof(BaseInt) , cudaMemcpyDeviceToDevice), __LINE__);
                return vli;
            }

            proxy& operator = (vli_gpu<BaseInt> const& vli)
            {
                gpu::check_error( cudaMemcpy( data_, vli.p(), vli_size*sizeof(BaseInt) , cudaMemcpyDeviceToDevice), __LINE__);
                return *this;
            }


        private:
            BaseInt* data_;    
    };

    /**
      constructor
      constructs a vector of vli_gpu<BaseInt> of size
      */
    vli_vector_gpu(size_type size = 0)
        :size_(size)
    {
        gpu::check_error( cublasAlloc( size_*vli_size, sizeof(BaseInt), (void**)&data_ ), __LINE__);
        
        //TODO isn't there a better way to initalize the memory on the GPU?
        BaseInt dummy[vli_size];
        for(std::size_t i = 0; i < vli_size; ++i)
            dummy[i] = 0;
        for(std::size_t i = 0; i < size; ++i)
            gpu::check_error( cublasSetVector( vli_size, sizeof(BaseInt),&dummy, 1, p()+i*vli_size, 1), __LINE__);
    }

    /**
      copy-constructor
      */
    vli_vector_gpu(vli_vector_gpu const& v)
        :size_(v.size_)
    {
        gpu::check_error( cublasAlloc( size_*vli_size, sizeof(BaseInt), (void**)&data_ ), __LINE__);
        gpu::check_error( cudaMemcpy( data_, v.data_, size_*vli_size*sizeof(BaseInt) , cudaMemcpyDeviceToDevice), __LINE__);
    }

    /**
      copy-construct from CPU version
      */
    vli_vector_gpu(vli_vector<vli_cpu<BaseInt> > const& v_cpu)
        :size_(v_cpu.size())
    {
        BOOST_STATIC_ASSERT( vli_cpu<BaseInt>::size == static_cast<std::size_t>(vli_size) );

        gpu::check_error( cublasAlloc( size_*vli_size, sizeof(BaseInt), (void**)&data_ ), __LINE__);
        
        //TODO there's probably a better implementation
        for(typename vli_vector<BaseInt>::size_type i = 0; i < v_cpu.size(); ++i)
		    gpu::check_error(cublasSetVector(vli_cpu<BaseInt>::size, sizeof(BaseInt), &(v_cpu[i][0]), 1, data_+static_cast<std::ptrdiff_t>(i*vli_size), 1), __LINE__);
    }

    /**
      destructor
      */
    ~vli_vector_gpu()
    {
        cublasFree(data_);
    }

    /**
      swap
      */
    void swap(vli_vector_gpu& v)
    {
        using std::swap;
        swap(data_,v.data_);
        swap(size_,v.size_);
    }

    /**
      assignment
      */
    vli_vector_gpu& operator= (vli_vector_gpu v)
    {
        swap(v);
        return *this;
    }

    /**
      conversion to vli_vector on CPU
      */
    operator vli_vector<vli_cpu<BaseInt> >() const
    {
        BOOST_STATIC_ASSERT( vli_cpu<BaseInt>::size == static_cast<std::size_t>(vli_size) );

        vli_vector<vli_cpu<BaseInt> > v_cpu(size_);
        for(typename vli_vector<BaseInt>::size_type i = 0; i < size_; ++i)
            gpu::check_error(cublasGetVector(vli_size, sizeof(BaseInt), data_+static_cast<std::ptrdiff_t>(i*vli_size)  ,1, &(v_cpu[i][0]), 1), __LINE__);
        return v_cpu;
    }

    /**
      size of the vector (=dimension)
      */
    size_type size() const
    {
        return size_;
    }

    /**
      plus assignment
      */
    vli_vector_gpu& operator += (vli_vector_gpu const& v)
    {
        using detail::plus_assign;
        plus_assign(*this,v);
        return *this;
    }

    /**
      multiplication by a scalar
      */
    vli_vector_gpu& operator *= (vli_gpu<BaseInt> const& vli)
    {
        using detail::multiplies_assign;
        multiplies_assign(*this,vli);
        return *this;
    }

    inline BaseInt* p()
    {
        return data_;
    }

    inline BaseInt const* p() const
    {
        return data_;
    }

    /**
      element access
      */
    proxy operator [] (size_type i)
    {
        return proxy(*this, i);
    }

    /**
      print to ostream
      */
    void print(std::ostream& o) const
    {
        o<<static_cast<vli_vector<vli_cpu<BaseInt> > >(*this);
    }

    private:
       BaseInt* data_;  //< Device-Pointer to the vli_numbers on the GPU (memory structure: vli,vli,...)
       size_type size_; //< The dimension of the vector
};

/**
  vector addition
  */
template <class BaseInt>
const vli_vector_gpu<BaseInt> operator + (vli_vector_gpu<BaseInt> v_a, vli_vector_gpu<BaseInt> const& v_b)
{
    // TODO is it more efficient to do the + operation directly on the GPU
    // instead of mapping it to the += operation?
    assert(v_a.size() == v_b.size());
    v_a += v_b;
    return v_a;
}

/**
  inner product
  */
template <class BaseInt>
vli_gpu<BaseInt> inner_prod(vli_vector_gpu<BaseInt> const& v_a, vli_vector_gpu<BaseInt> const& v_b)
{
    assert( v_a.size() == v_b.size() );
    using detail::inner_prod_gpu;
    vli_gpu<BaseInt> vli;
    inner_prod_gpu(v_a.p(), v_b.p(), vli.p(),v_a.size(),vli_vector_gpu<BaseInt>::vli_size);
    return vli;
}

/**
  stream
  */
template <class BaseInt>
std::ostream& operator << (std::ostream& os, vli_vector_gpu<BaseInt> const& v)
{
    v.print(os);
    return os;
}

} //namespace vli

#endif // VLI_VECTOR_GPU_HPP
