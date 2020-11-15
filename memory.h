


const size_t TMP_MEMORY_PAGE_SIZE = 0xF000;
const s64 MAX_NUM_TEMPORARY_MEMORY_PAGES_AFTER_RESET = 4;

struct Temporary_Memory_Page
{
    u8 *start;
    size_t used;
    size_t size;
};

struct Temporary_Memory
{
    Array<Temporary_Memory_Page, ALLOC_APP> pages;
};

Temporary_Memory temporary_memory = {0};






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


struct Allocator
{
    Memory_Page pages[512];
    int num_pages;

#if DEBUG
    size_t block_size_sum;
    int num_allocs;
    int num_deallocs;
#endif
};


Allocator app_allocator    = {0};
Allocator game_allocator   = {0};
Allocator gfx_allocator    = {0};
Allocator ui_allocator     = {0};
Allocator dev_allocator    = {0};
Allocator platform_allocator = {0};

Allocator *allocators[] = {
    NULL, // ALLOC_MALLOC
    NULL, // ALLOC_TMP
    &app_allocator,
    &game_allocator,
    &gfx_allocator,
    &ui_allocator,
    &dev_allocator,
    &platform_allocator
};
