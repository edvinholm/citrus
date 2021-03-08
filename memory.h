

struct Allocator {
    virtual void i_am_polymorphic() {
    }
};


const size_t LINEAR_ALLOCATOR_PAGE_SIZE = 0xF000;
const s64 MAX_NUM_LINEAR_ALLOCATOR_PAGES_AFTER_RESET = 4;

struct Linear_Allocator_Page
{
    u8 *start   = NULL;
    size_t used = 0;
    size_t size = 0;
};

struct Linear_Allocator: public Allocator
{
    Array<Linear_Allocator_Page, ALLOC_MALLOC> pages;
};
void clear(Linear_Allocator *allocator) {
    
    for(int i = 0; i < allocator->pages.n; i++){
        dealloc(allocator->pages[i].start, ALLOC_MALLOC);
    }
    
    clear(&allocator->pages);
}

Linear_Allocator tmp_allocator = Linear_Allocator();



struct Memory_Buffer
{
    u8 *data;
    s64 size;
};




const size_t MEMORY_DEFAULT_PAGE_SIZE = 4 * 1024 * 1024;

struct Memory_Block_Header
{
    size_t prev_size;
    size_t size;
    bool alive;
    bool last;
};

struct Memory_Page
{
    u8 *start;
    
    size_t size;
};


// @Jai @Stupid @Hack
struct Malloc_Allocator: public Allocator { };
Malloc_Allocator malloc_allocator = {};

Allocator *allocators[] = {
    &malloc_allocator, // ALLOC_MALLOC
    &tmp_allocator,    // ALLOC_TMP
};
static_assert(ARRLEN(allocators) == ALLOC_NONE_OR_NUM);
