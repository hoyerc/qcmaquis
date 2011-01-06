#ifndef AMBIENT_H
#define AMBIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

class dim3
{
public:
    unsigned int x, y, z;
    dim3(unsigned int x = 1, unsigned int y = 1, unsigned int z = 1) : x(x), y(y), z(z) {}
    dim3& operator=(int value){
        x = y = z = value;
    }
    bool operator==(int value){
        return (x == value && y == value && z == value);
    }
};

class scheduler
{
private: 
    scheduler();                               // constructor is private
    scheduler(scheduler const&){};             // copy constructor is private
    scheduler& operator=(scheduler const&){};  // assignment operator is private
public:
    static scheduler* instance();

public:
    scheduler & operator>>(dim3 dim_distr);
    scheduler & operator,(dim3 dim); 
    void initialize(MPI_Comm comm = NULL);

    template<class packet>
    void build_packet_type();

    void finalize();

private:
    MPI_Comm comm;
    int size;
    int rank;

    enum { AMBIENT_MASTER,
           GROUP_MASTER,
           GROUP_SLAVE } mode;

    dim3 dim_distr; // work-item size of distribution blocks
    dim3 dim_cpu;   // work-item size of cpu core workload fractions
    dim3 dim_gpu;   // work-item size of gpgpu smp workload fractions
};

scheduler& operator>>(scheduler* instance, dim3 dim_distr);

#endif
