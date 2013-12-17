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

#include "ambient/utils/io.hpp"
#include "ambient/utils/timings.hpp"
#include "ambient/utils/overseer.hpp"
#include "ambient/utils/service.hpp"

namespace ambient { namespace controllers { namespace ssm {

    inline controller::~controller(){ 
        if(!chains->empty()) printf("Ambient:: exiting with operations still in queue!\n");
        this->clear();
    }

    inline controller::controller(){
        this->stack_m.reserve(AMBIENT_STACK_RESERVE);
        this->stack_s.reserve(AMBIENT_STACK_RESERVE);
        this->chains = &this->stack_m;
        this->mirror = &this->stack_s;
        ambient::channel.init();
        if(ambient::rank()) ambient::rank.mute();
        this->context_base = new ambient::scope<base>();
        this->context = this->context_base;
        this->serial = (ambient::channel.dim() == 1) ? true : false;
        #ifdef AMBIENT_PARALLEL_MKL
        ambient::mkl_set_num_threads(1);
        #endif
        #ifdef AMBIENT_VERBOSE
            #ifdef AMBIENT_CILK
            ambient::cout << "ambient: initialized (using cilk)\n";
            #elif defined(AMBIENT_OMP)
            ambient::cout << "ambient: initialized (using openmp)\n";
            #else
            ambient::cout << "ambient: initialized (no threading)\n";
            #endif
            ambient::cout << "ambient: size of ambient bulk chunks: " << AMBIENT_BULK_CHUNK << "\n";
            ambient::cout << "ambient: maximum number of bulk chunks: " << AMBIENT_BULK_LIMIT << "\n";
            ambient::cout << "ambient: maximum sid value: " << AMBIENT_MAX_SID << "\n";
            ambient::cout << "ambient: number of database proc: " << AMBIENT_DB_PROCS << "\n";
            ambient::cout << "ambient: number of work proc: " << ambient::channel.wk_dim() << "\n";
            ambient::cout << "ambient: number of threads per proc: " << ambient::get_num_threads() << "\n";
            #ifdef AMBIENT_PARALLEL_MKL
            ambient::cout << "ambient: using MKL: threaded\n";
            #endif 
            ambient::cout << "\n";
        #endif
    }

    inline bool controller::tunable(){
        if(serial) return false;
        return context->tunable();
    }

    inline void controller::schedule(){
        const_cast<scope*>(context)->toss();
    }

    inline void controller::intend_read(history* o){
        revision* r = o->back(); if(r == NULL || ambient::model.common(r)) return;
        int candidate = ambient::model.remote(r) ? r->owner : (int)ambient::rank();
        context->score(candidate, r->spec.extent);
    }

    inline void controller::intend_write(history* o){
        revision* r = o->back(); if(r == NULL || ambient::model.common(r)) return;
        int candidate = ambient::model.remote(r) ? r->owner : (int)ambient::rank();
        context->select(candidate);
    }

    inline void controller::set_context(const scope* s){
        this->context = s; // no nesting
    }

    inline void controller::pop_context(){
        this->context = this->context_base;
    }

    inline bool controller::remote(){
        return (this->context->state == ambient::remote);
    }

    inline bool controller::local(){
        return (this->context->state == ambient::local);
    }

    inline bool controller::common(){
        return (this->context->state == ambient::common);
    }

    inline int controller::which(){
        return this->context->sector;
    }

    inline void controller::flush(){
        typedef typename std::vector<functor*>::const_iterator veci;
        #ifdef AMBIENT_COMPUTATIONAL_DATAFLOW
        printf("ambient::parallel graph dim: %d\n", chains->size());
        for(veci i = chains->begin(); i != chains->end(); ++i)
            printf("op%d[label=\"%s\"]\n", (*i)->id(), (*i)->name());
        #endif
        /*while(!chains->empty()){
            int i;
            const int N = chains->size();
            std::vector<functor*> prod[N];
            #pragma omp parallel for
            for(i = 0; i < N; i++){
                functor* task = (*chains)[i];
                if(task->ready()){
                    task->invoke();
                    prod[i].insert(prod[i].end(), task->deps.begin(), task->deps.end());
                }else prod[i].push_back(task);
            }
            for(i = 0; i < N; i++){
                mirror->insert(mirror->end(), prod[i].begin(), prod[i].end());
            }
            chains->clear();
            std::swap(chains,mirror);
        }*/
        AMBIENT_SMP_ENABLE
        while(!chains->empty()){
            for(veci i = chains->begin(); i != chains->end(); ++i){
                if((*i)->ready()){
                    functor* task = *i;
                    AMBIENT_THREAD task->invoke();
                    #ifdef AMBIENT_COMPUTATIONAL_DATAFLOW
                    for(int n = 0; n < task->deps.size(); ++n)
                        printf("op%d[label=\"%s\"]\nop%d -> op%d\n", task->deps[n]->id(), task->deps[n]->name(), 
                                                                     task->id(), task->deps[n]->id());
                    #endif
                    int size = task->deps.size();
                    for(int n = 0; n < size; n++) task->deps[n]->ready();
                    mirror->insert(mirror->end(), task->deps.begin(), task->deps.end());
                }else mirror->push_back(*i);
            }
            chains->clear();
            std::swap(chains,mirror);
        }
        AMBIENT_SMP_DISABLE
        ambient::model.clock++;
        #ifdef AMBIENT_TRACKING
        ambient::overseer::log::stop();
        #endif
        ambient::channel.barrier();
    }

    inline bool controller::empty(){
        return this->chains->empty();
    }

    inline void controller::clear(){
        this->garbage.clear();
    }

    inline bool controller::queue(functor* f){
        this->chains->push_back(f);
        return true;
    }

    inline bool controller::update(revision& r){
        if(r.assist.first != ambient::model.clock){
            r.assist.first = ambient::model.clock;
            return true;
        }
        return false;
    }

    inline void controller::sync(revision* r){
        if(serial) return;
        if(ambient::model.common(r)) return;
        if(ambient::model.feeds(r)) ambient::controllers::ssm::set<revision>::spawn(*r);
        else ambient::controllers::ssm::get<revision>::spawn(*r);
    }

    inline void controller::lsync(revision* r){
        if(ambient::model.common(r)) return;
        if(!ambient::model.feeds(r)) ambient::controllers::ssm::get<revision>::spawn(*r);
    }

    inline void controller::rsync(revision* r){
        if(ambient::model.common(r)) return;
        if(r->owner != which()) ambient::controllers::ssm::set<revision>::spawn(*r);
    }

    inline void controller::lsync(transformable* v){
        if(serial) return;
        ambient::controllers::ssm::set<transformable>::spawn(*v);
    }

    inline void controller::rsync(transformable* v){
        ambient::controllers::ssm::get<transformable>::spawn(*v);
    }

    template<typename T> void controller::collect(T* o){
        this->garbage.push_back(o);
    }

    inline void controller::squeeze(revision* r) const {
        #ifdef AMBIENT_MEMORY_SQUEEZE
        if(r->valid() && !r->referenced() && r->locked_once()){
            if(r->spec.region == ambient::rstandard){
                ambient::pool::free(r->data, r->spec);
                r->spec.region = ambient::rdelegated;
            }else if(r->spec.region == ambient::rbulked){
                ambient::memory::bulk::reuse(r->data);
                r->spec.region = ambient::rdelegated;
            }
        }
        #endif
    }

} } }

