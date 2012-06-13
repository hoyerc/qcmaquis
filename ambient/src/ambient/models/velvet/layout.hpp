#include "ambient/utils/ceil.h"
//#define LAYOUT_ACCESS_CHECK

#define VECTOR_RESERVATION 1.5

namespace ambient { namespace models { namespace velvet {

    // {{{ layout model

    inline layout::~layout(){
        if(this->mesh_dim != 1 || this->mesh_lda != 1){
            size_t size = this->mesh_dim.x*this->mesh_lda;
            for(size_t i = 0; i < size; i++) delete entries[i];
        }else
            delete entries[0];
    }

    inline layout::layout(size_t t_size, dim2 b_size, dim2 size)
    : spec(t_size*b_size.x*b_size.y), mem_dim(b_size), dim(size), placement(NULL), grid_dim(0,0), master(0), mesh_dim(0,0), mesh_lda(0)
    {
        this->set_dim(size);
    }

    inline const memspec& layout::get_spec() const {
        return this->spec;
    }

    inline void layout::embed(void* memory, size_t x, size_t y, size_t bound){
        this->get(x,y).set_memory(memory, bound);
    }

    inline layout::entry& layout::get(size_t x, size_t y){
#ifdef LAYOUT_ACCESS_CHECK
        if(y >= this->get_grid_dim().y || x >= this->get_grid_dim().x)
        printf("%ld: Trying to access %ld x %ld of %ld x %ld\n", this->sid, x, y, this->get_grid_dim().x, this->get_grid_dim().y);
#endif
        return *this->entries[x*mesh_lda+y];
    }

    inline void layout::mesh(){
        this->grid_dim = dim2(__a_ceil(this->dim.x / this->mem_dim.x), 
                              __a_ceil(this->dim.y / this->mem_dim.y));
        dim2 dim = this->grid_dim;
        if(this->mesh_dim.x >= dim.x && this->mesh_dim.y >= dim.y) return;

        if(this->mesh_lda < dim.y){ // reserve
            std::vector<entry*> tmp;
            tmp.reserve(std::max(dim.x,this->mesh_dim.x)*dim.y*VECTOR_RESERVATION);
            for(int j=0; j < this->mesh_dim.x; ++j){
                tmp.insert(tmp.end(),&this->entries[this->mesh_lda*j],&this->entries[this->mesh_lda*j + this->mesh_dim.y]);
                for(size_t y = this->mesh_dim.y; y < dim.y*VECTOR_RESERVATION; y++)
                    tmp.push_back(new layout::entry());
            }
            std::swap(this->entries,tmp);
            this->mesh_lda = dim.y*VECTOR_RESERVATION;
        }

        if(dim.x > mesh_dim.x){
            size_t appendix = (dim.x-this->mesh_dim.x)*this->mesh_lda;
            for(size_t i = 0; i < appendix; i++)
                entries.push_back(new layout::entry());
        }

        this->mesh_dim.x = std::max(dim.x, mesh_dim.x);
        this->mesh_dim.y = std::max(dim.y, mesh_dim.y);
    }

    inline size_t layout::id(){
        return this->sid;
    }

    inline size_t layout::get_master(){
        return this->master;
    }

    inline void layout::set_dim(dim2 dim){
        this->dim = dim;
        this->mesh();
    }

    inline dim2 layout::get_dim() const {
        return this->dim;
    }

    inline dim2 layout::get_mem_dim() const { 
        return this->mem_dim;
    }

    inline dim2 layout::get_grid_dim() const {
        return this->grid_dim;
    }

    // }}}

    // {{{ layout::entry + layout::marker //

    inline bool layout::entry::trylock(){
        if(this->locked == true) return false;
        this->locked = true;
        return true;
    }

    inline void layout::entry::unlock(){
        this->locked = false;
    }

    inline layout::entry::~entry(){
        free(this->header);
    }

    inline layout::entry::entry()
    : header(NULL), data(NULL), request(false), locked(false)
    {
    }

    inline void layout::entry::swap(entry& e){
        this->data = e.data;
        this->header = e.header;
        e.header = NULL;
    }

    inline void layout::entry::set_memory(void* memory, size_t bound){
        this->header = memory;
        this->data = (void*)((size_t)memory + bound);
    }

    inline void* layout::entry::get_memory(){
        return this->header;
    }

    inline bool layout::entry::valid(){
        return (this->data != NULL);
    }

    inline bool layout::entry::occupied(){
        if(!this->valid()) return true;
        return false;
    }

    inline bool layout::entry::requested(){
        return this->request;
    }

    inline std::list<controllers::velvet::cfunctor*>& layout::entry::get_assignments(){
        return this->assignments;
    }

    inline std::list<size_t>& layout::entry::get_path(){
        return this->path;
    }

    inline layout::marker::marker(){
        this->active = true;
        this->xmarker = 0;
        this->ymarker = 0;
    }

    inline bool layout::marker::marked(size_t x, size_t y){
        if(!this->active) return false;
        if(x >= this->xmarker || y >= this->ymarker) return true;
        return false;
    }

    inline void layout::marker::mark(size_t x, size_t y){
        this->active = true;
        this->xmarker = x;
        this->ymarker = y;
    }

    inline void layout::marker::clear(){
        this->active = false;
    }

    // }}}

} } }
