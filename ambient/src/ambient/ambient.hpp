/*
 * Ambient, License - Version 1.0 - May 3rd, 2012
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

#ifndef AMBIENT
#define AMBIENT
// {{{ system includes
#include <mpi.h>
#include <complex>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <limits>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <memory.h>
#include <stdarg.h>
#include <ctype.h>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <algorithm>
#include <execinfo.h>
// }}}

#define AMBIENT_VERBOSE

#if !defined(AMBIENT_CILK) && !defined(AMBIENT_OMP) && !defined(AMBIENT_SERIAL)
#if defined __INTEL_COMPILER
#define AMBIENT_CILK
#elif defined __GNUC__
#define AMBIENT_OMP
#endif
#endif

#ifdef AMBIENT_CILK
    #include <cilk/cilk.h>
    #include <cilk/cilk_api.h>
    #define AMBIENT_NUM_THREADS __cilkrts_get_total_workers()
    #define AMBIENT_THREAD_ID __cilkrts_get_worker_number()
    #define AMBIENT_THREAD cilk_spawn
    #define AMBIENT_SMP_ENABLE
    #define AMBIENT_SMP_DISABLE cilk_sync;
#elif defined(AMBIENT_OMP)
    #include <omp.h>
    #define AMBIENT_THREAD_ID omp_get_thread_num()
    #define AMBIENT_PRAGMA(a) _Pragma( #a )
    #define AMBIENT_THREAD AMBIENT_PRAGMA(omp task untied)
    #define AMBIENT_SMP_ENABLE AMBIENT_PRAGMA(omp parallel) { AMBIENT_PRAGMA(omp single nowait)
    #define AMBIENT_SMP_DISABLE }
    #define AMBIENT_NUM_THREADS [&]()->int{ int n; AMBIENT_SMP_ENABLE \
                                { n = omp_get_num_threads(); } \
                                AMBIENT_SMP_DISABLE return n; }()
#else // serial
    #define AMBIENT_NUM_THREADS 1
    #define AMBIENT_THREAD_ID   0
    #define AMBIENT_THREAD
    #define AMBIENT_SMP_ENABLE
    #define AMBIENT_SMP_DISABLE
#endif

#define AMBIENT_DB_PROCS   0
#define AMBIENT_BROADCAST -1

//#define AMBIENT_NUMERIC_EXPERIMENTAL
//#define AMBIENT_TRACE void* b[15]; backtrace_symbols_fd(b,backtrace(b,15),2);
//#define AMBIENT_CHECK_BOUNDARIES
//#define AMBIENT_LOOSE_FUTURE
//#define AMBIENT_PARALLEL_MKL // please define in compiler

#define AMBIENT_MEMORY_SQUEEZE
#define AMBIENT_REBALANCED_SVD

#define AMBIENT_INSTR_BULK_CHUNK      16777216 // 16 MB
#define AMBIENT_DATA_BULK_CHUNK       67108864 // 64 MB
#define AMBIENT_BULK_LIMIT            40

#define AMBIENT_MKL_NUM_THREADS       16
#define AMBIENT_MAX_SID               2097152 // Cray MPI

#define AMBIENT_STACK_RESERVE         65536
#define AMBIENT_COLLECTOR_STR_RESERVE 65536
#define AMBIENT_COLLECTOR_REV_RESERVE 65536
#define AMBIENT_COLLECTOR_RAW_RESERVE 1024
#define AMBIENT_SCOPE_SWITCH_FACTOR   20480
#define AMBIENT_FUTURE_SIZE           64
#define AMBIENT_IB                    2048
#define AMBIENT_IB_EXTENT             2048*2048*16

#define PAGE_SIZE 4096
#define ALIGNMENT 64

namespace ambient {
    inline int get_num_threads(){
        static int n = AMBIENT_NUM_THREADS; return n;
    }
    enum locality   { remote, local, common };
    enum scope_t    { base, single, shared, dedicated, threaded };
    enum region_t   { rbulked, rstandard, rdelegated };
}


#include "ambient/utils/dim2.h"
#include "ambient/utils/tree.hpp"
#include "ambient/utils/fence.hpp"
#include "ambient/utils/singleton.hpp"

#include "ambient/memory/pool.hpp"
#include "ambient/memory/new.hpp"
#include "ambient/memory/allocator.hpp"

#include "ambient/models/ssm/revision.h"
#include "ambient/models/ssm/history.h"
#include "ambient/models/ssm/transformable.h"
#include "ambient/models/ssm/model.h"

#include "ambient/channels/mpi/group.h"
#include "ambient/channels/mpi/multirank.h"
#include "ambient/channels/mpi/request.h"
#include "ambient/channels/mpi/collective.h"
#include "ambient/channels/mpi/channel.h"

#include "ambient/controllers/ssm/functor.h"
#include "ambient/controllers/ssm/get.h"
#include "ambient/controllers/ssm/set.h"
#include "ambient/controllers/ssm/collector.h"
#include "ambient/controllers/ssm/controller.h"

#include "ambient/utils/auxiliary.hpp"
#include "ambient/utils/io.hpp"

#include "ambient/models/ssm/revision.hpp"
#include "ambient/models/ssm/history.hpp"
#include "ambient/models/ssm/transformable.hpp"
#include "ambient/models/ssm/model.hpp"

#include "ambient/channels/mpi/group.hpp"
#include "ambient/channels/mpi/multirank.hpp"
#include "ambient/channels/mpi/request.hpp"
#include "ambient/channels/mpi/collective.hpp"
#include "ambient/channels/mpi/channel.hpp"

#include "ambient/controllers/ssm/get.hpp"
#include "ambient/controllers/ssm/set.hpp"
#include "ambient/controllers/ssm/scope.hpp"
#include "ambient/controllers/ssm/collector.hpp"
#include "ambient/controllers/ssm/controller.hpp"

#include "ambient/interface/typed.hpp"
#include "ambient/interface/kernel.hpp"
#include "ambient/interface/access.hpp"
#endif
