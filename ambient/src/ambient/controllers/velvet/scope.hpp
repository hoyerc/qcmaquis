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

#ifndef AMBIENT_INTERFACE_SCOPE
#define AMBIENT_INTERFACE_SCOPE

namespace ambient { 

    using ambient::controllers::velvet::controller;

    template<scope_t T>
    class scope {};

    template<>
    class scope<base> : public controller::scope {
    public:
        scope(){
            this->round = ambient::channel.wk_dim();
            this->state = ambient::rank() ? ambient::remote : ambient::local;
            this->sector = 0;
            this->scores.resize(round, 0);
        }
        virtual bool tunable() const { 
            return true;
        }
        virtual void score(int c, size_t v) const {
            this->scores[c] += v;
        }
        virtual void select(int c) const {
            this->stakeholders.push_back(c);
        }
        virtual void toss(){
            int max = 0;
            if(stakeholders.empty()){
                for(int i = 0; i < round; i++)
                if(scores[i] >= max){
                    max = scores[i];
                    this->sector = i;
                }
            }else{
                for(int i = 0; i < stakeholders.size(); i++){
                    int k = stakeholders[i];
                    if(scores[k] >= max){
                        max = scores[k];
                        this->sector = k;
                    }
                }
                stakeholders.clear();
            }
            std::fill(scores.begin(), scores.end(), 0);
            this->state = (this->sector == ambient::rank()) ? 
                          ambient::local : ambient::remote;
        }
        mutable std::vector<int> stakeholders;
        mutable std::vector<int> scores;
        int round;
    };

    template<>
    class scope<single> : public controller::scope {
    public:
        static int compact_factor; 
        static void compact(size_t n){ 
            if(n <= ambient::channel.wk_dim()) return; 
            compact_factor = (int)(n / ambient::channel.wk_dim()); // iterations before switch 
        } 
        scope(int value = 0) : index(value), iterator(value) {
            this->factor = compact_factor; compact_factor = 1;
            if(ambient::controller.context != ambient::controller.context_base) dry = true;
            else{ dry = false; ambient::controller.set_context(this); }
            this->round = ambient::channel.wk_dim();
            this->shift();
        }
        void shift(){
            this->sector = (++this->iterator %= this->round*this->factor)/this->factor;
            this->state = (this->sector == ambient::rank()) ? ambient::local : ambient::remote;
        }
        void shift_back(){ 
            this->sector = (--this->iterator)/this->factor; 
            this->state = (this->sector == ambient::rank()) ? ambient::local : ambient::remote;
            if(this->sector < 0) printf("Error: shift_back to negative in scope!\n\n\n"); 
        } 
        scope& operator++ (){
            this->shift();
            this->index++;
            return *this;
        }
        scope& operator-- (){
            this->shift_back();
            this->index--;
            return *this;
        }
        operator size_t (){
            return index;
        }
        bool operator < (size_t lim){
            return index < lim;
        }
       ~scope(){
            if(!dry) ambient::controller.pop_context();
        }
        virtual bool tunable() const {
            return false; 
        }
        size_t index;
        bool dry;
        int iterator;
        int factor;
        int round;
    };

    template<>
    class scope<dedicated> : public controller::scope {
    public:
        scope(){
            ambient::controller.set_context(this);
            this->sector = ambient::rank.dedicated();
            this->state = (this->sector == ambient::rank()) ? ambient::local : ambient::remote;
        }
       ~scope(){
            ambient::controller.pop_context();
        }
        virtual bool tunable() const {
            return false; 
        }
    };

    template<>
    class scope<shared> : public controller::scope {
    public:
        scope(){
            ambient::controller.set_context(this);
            this->state = ambient::common;
        }
       ~scope(){
            ambient::controller.pop_context();
        }
        virtual bool tunable() const { 
            return false; 
        }
    };
}

#endif
