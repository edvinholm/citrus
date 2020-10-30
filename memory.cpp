



bool equal(u8 *cs1, u8 *cs2);

template<typename T, Allocator_ID A>
void clear(Array<T, A> *arr);

struct String;






inline
Memory_Block_Header first_memory_block(size_t page_size)
{
    Memory_Block_Header first_block = {0};
    first_block.prev_size = 0;
    first_block.size = page_size - sizeof(Memory_Block_Header);
    first_block.alive = false;
    first_block.last  = true;
    return first_block;
}


Memory_Page *add_memory_page(size_t size, Allocator *allocator)
{
    Release_Assert(allocator->num_pages < ARRLEN(allocator->pages));
    
    Memory_Page page = {0};
    page.start = (u8 *)malloc(size);
    page.size = size;
    Release_Assert(page.start);

    Memory_Block_Header first_block = first_memory_block(page.size);

    Memory_Block_Header *first = (Memory_Block_Header*)page.start;
    
    *first = first_block;

    allocator->pages[allocator->num_pages++] = page;
    
    return allocator->pages + allocator->num_pages - 1;
}


inline
u8 *allocate_in_page(size_t size, Memory_Page *page, Allocator *allocator)
{
    Release_Assert(size > 0);

    size_t size_with_header = sizeof(Memory_Block_Header) + size;
    if(size_with_header > page->size) return NULL;

    Memory_Block_Header *my_block = NULL;

    Memory_Block_Header *block = (Memory_Block_Header *)page->start;

    while(true)
    {
        if(!block->alive && block->size >= size)
        {
            my_block = block;
        
            size_t ds = block->size - size;
            if(ds > sizeof(Memory_Block_Header))
            {
                my_block->size = size;
            
                Memory_Block_Header *rest = (Memory_Block_Header *)(((u8 *)block) + sizeof(Memory_Block_Header) + size);
                rest->prev_size = my_block->size;
                rest->size = ds - sizeof(Memory_Block_Header);
                rest->alive = false;

                rest->last = block->last;
                my_block->last = false;

                if (!rest->last)
                {
                    Memory_Block_Header *next = (Memory_Block_Header *)(((u8 *)rest) + sizeof(Memory_Block_Header) + rest->size);
                    next->prev_size = rest->size;
                }
            }
        
            // So if ds <= sizeof(Memory_Block_Header), our block will have the old block's size. So it will potentially be bigger than we asked for.

            my_block->alive = true;
            break;
        }

        if(block->last) break;

        Release_Assert(block->size > 0);
        block = (Memory_Block_Header *)((u8 *)block + sizeof(Memory_Block_Header) + block->size);
    }

    if (my_block)
    {
#if DEBUG
#if false
        u8 *block_start = (u8 *)my_block + sizeof(Memory_Block_Header);
        memset(block_start, 0xAA, my_block->size);
#endif

        allocator->block_size_sum += my_block->size;
#endif

        return ((u8 *)my_block) + sizeof(Memory_Block_Header);
    }
    
    return NULL;
}

u8 *allocate(size_t size, Allocator *allocator)
{
#if DEBUG
    allocator->num_allocs++;
#endif
    
    if (size == 0) return NULL;
    
    for(int p = allocator->num_pages-1; p >= 0; p--)
    {
        u8 *ptr = allocate_in_page(size, allocator->pages + p, allocator);
        if(ptr) return ptr;
    }
    
    size_t size_with_header = sizeof(Memory_Block_Header) + size;

    Memory_Page *new_page = add_memory_page(max(size_with_header, MEMORY_DEFAULT_PAGE_SIZE), allocator);
    u8 *ptr = allocate_in_page(size, new_page, allocator);
    Release_Assert(ptr);

    return ptr;
}

void deallocate(void *ptr, Allocator *allocator)
{
#if DEBUG
    allocator->num_deallocs++;
#endif

    auto *block = (Memory_Block_Header *)((u8 *)ptr - sizeof(Memory_Block_Header));
    
    Assert(block->alive);
    block->alive = false;

#if DEBUG
    Assert(allocator->block_size_sum >= block->size);
    allocator->block_size_sum -= block->size;
#endif

    // Find dead blocks backwards
    while(block->prev_size > 0)
    {
        auto *prev = (Memory_Block_Header *)((u8 *)block - block->prev_size - sizeof(Memory_Block_Header));
        if(prev->alive) break;
        
        block = prev;
    }
    
    // Merge dead blocks forward
    while(true)
    {
        if(block->last) break;
        
        auto *next = (Memory_Block_Header *)((u8 *)block + sizeof(Memory_Block_Header) + block->size);
        if(next->alive) {
            next->prev_size = block->size;
            break;
        }

        Assert(next->size > 0);

        block->size += sizeof(Memory_Block_Header) + next->size;

        block->last = next->last;
    }

#if DEBUG && false
    u8 *block_start = (u8 *)block + sizeof(Memory_Block_Header);
    memset(block_start, 0xDD, block->size);
#endif
}

u64 platform_milliseconds();


void init_allocator(Allocator *allocator)
{
    add_memory_page(MEMORY_DEFAULT_PAGE_SIZE, allocator);
}

void reset_allocator(Allocator *allocator)
{
    // Free all pages but one.
    for(int p = allocator->num_pages - 1; p >= 1; p--)
        free(allocator->pages[p].start);
    allocator->num_pages = 1;

    // Make first page be one dead block.
    auto *first_page = allocator->pages;
    auto *block = (Memory_Block_Header *)first_page->start;
    *block = first_memory_block(first_page->size);

#if DEBUG
    allocator->block_size_sum = 0;
#endif
}

inline
void reset_allocator(Allocator_ID allocator)
{
    reset_allocator(allocators[allocator]);
}














inline
void reset_temporary_memory()
{
    for(int i = 0; i < temporary_memory.pages.n; i++)
    {
        if(i >= MAX_NUM_TEMPORARY_MEMORY_PAGES_AFTER_RESET)
        {
            app_dealloc_ignore_temporary_memory_test(temporary_memory.pages[i].start);
            continue;
        }
        
        temporary_memory.pages[i].used = 0;
    }

    temporary_memory.pages.n = min(temporary_memory.pages.n, MAX_NUM_TEMPORARY_MEMORY_PAGES_AFTER_RESET);
}

#if DEBUG

int DEBUG_num_allocs = 0;

#endif

//@BadName maybe. Does not give NEXT multiple if remainder is 0.
u64 next_multiple(u64 x, u64 factor)
{
    u64 remainder = x % factor;
    if(remainder > 0)
        x += factor - remainder;
    return x;
}

u8 *tmp_alloc(size_t size)
{   
    Temporary_Memory_Page *page = NULL;
    for(int p = temporary_memory.pages.n - 1; p >= 0; p--)
    {
        Temporary_Memory_Page &pg = temporary_memory.pages[p];
        
        if(pg.used + size <= pg.size)
        {
            page = &pg;
            break;
        }
    }

    if(page == NULL)
    {
        size_t page_size = TMP_MEMORY_PAGE_SIZE;
        if(size > TMP_MEMORY_PAGE_SIZE)
            page_size = size;
        
        Temporary_Memory_Page new_page = {0};
        new_page.start = app_alloc(page_size);
        new_page.size = page_size;
        array_add(temporary_memory.pages, new_page);

        page = last_element_pointer(temporary_memory.pages);
    }
    
    Assert((page->used % 64) == 0); // Assert new block is 64-byte aligned.

    u8 *result = page->start + page->used;
    page->used += next_multiple(size, 64); // Make sure next block is 64-byte aligned.

    Release_Assert(result);
    return result;
}

u8 *tmp_realloc(u8 *old, size_t old_size, size_t new_size)
{
    Temporary_Memory_Page *page = NULL;
    for(int p = temporary_memory.pages.n - 1; p >= 0; p--)
    {
        Temporary_Memory_Page &pg = temporary_memory.pages[p];
        if(old >= pg.start && old < pg.start + pg.size)
            page = &pg;
    }
        
    Assert(page);

    if(old + old_size == page->start + page->used &&
       old + new_size <= page->start + page->size)
    {
        // We can just make this allocation bigger.
        page->used += new_size - old_size;

        return old;
    }

    return tmp_alloc(new_size);
}



inline
u8 *alloc(size_t size, Allocator_ID allocator)
{
    //void *result = allocate(size, default_allocator);

    Assert(ARRLEN(allocators) == ALLOC_NONE_OR_NUM);
    Assert(allocator != ALLOC_NONE_OR_NUM);
    Assert(allocator < ARRLEN(allocators));

#if DEBUG
    if(allocator != ALLOC_TMP)
        DEBUG_num_allocs++;
#endif

    switch(allocator)
    {
        case ALLOC_MALLOC: {
            auto *result = (u8 *)malloc(size);
            return result;
        } break;
            
        case ALLOC_TMP:    return tmp_alloc(size);

        default: {
            return (u8 *)malloc(size);
            
            //return allocate(size, allocators[allocator]);
        }
    }
}

template<typename T>
T *alloc_elements(int n, Allocator_ID allocator)
{
    return (T *)alloc(sizeof(T)*n, allocator);
}

inline
void dealloc(void *ptr, Allocator_ID allocator
#if DEBUG
    , bool DEBUG_ignore_temporary_memory_test/* = false*/
#endif
    )
{   
    Assert(ptr);

#if DEBUG
    if(!DEBUG_ignore_temporary_memory_test)
    {
        for(int p = temporary_memory.pages.n - 1; p >= 0; p--)
        {
            Temporary_Memory_Page &pg = temporary_memory.pages[p];
            Assert(!(ptr >= pg.start && ptr < pg.start + TMP_MEMORY_PAGE_SIZE));
        }
    }
#endif

#if DEBUG

    u8 first_byte_before = *(u8 *)ptr;

    *(u8 *)ptr = 'F';

#if DEBUG_MEMORY_SLOW
    if(!now_doing_allocs_deallocs_debug_stuff)
    {
        now_doing_allocs_deallocs_debug_stuff = true;
        register_dealloc(ptr, file, line);
        now_doing_allocs_deallocs_debug_stuff = false;
    }
#endif
    
    DEBUG_num_allocs--;
    
#endif

    switch(allocator)
    {
        case ALLOC_MALLOC: free(ptr); break;
        case ALLOC_TMP:    Assert(false); break;

        default: {
            free(ptr);
//            deallocate(ptr, allocators[allocator]);
        } break;
    }
    
}

void dealloc_if_legal(void *ptr, Allocator_ID allocator)
{
    if(allocator == ALLOC_TMP) return;
    dealloc(ptr, allocator);
}


template<typename U>
bool realloc_needed(U current_capacity, U required_capacity, U *_new_capacity = NULL, U min_capacity = 1)
{
    U new_capacity = max(min_capacity, current_capacity);
    while(new_capacity < required_capacity)
        new_capacity *= 2;
    
    if(new_capacity != current_capacity)
    {
        if(_new_capacity)
            *_new_capacity = new_capacity;
        return true;
    }
    return false;
}

// NOTE: May change *allocation and *capacity
// NOTE: U should be an integer type
template<typename T, typename U>
void ensure_capacity(T **allocation, U *capacity, U required_capacity, Allocator_ID allocator, U min_capacity = 1, bool copy_data = false)
{
    U new_capacity = max(min_capacity, *capacity);
    while(new_capacity < required_capacity)
        new_capacity *= 2;
    
    if(new_capacity != *capacity)
    {
        T *new_allocation = (T *)alloc(new_capacity * sizeof(T), allocator);
        
        if(copy_data) {
            memcpy(new_allocation, *allocation, *capacity);
        }
        
        if(*allocation)
            dealloc(*allocation, allocator);

        *allocation = new_allocation;
        *capacity   = new_capacity;
    }
}


inline
u32 reversed_byte_order_32(u32 integer)
{
    return ((integer >> 24) & 0xff) | ((integer << 8) & 0xff0000) | ((integer >> 8) & 0xff00) | ((integer << 24) & 0xff000000);
}

inline
u16 reversed_byte_order_16(u32 integer)
{
    return ((integer >>8) | (integer << 8));
}


int machine_is_big_endian = -1; // 0 == false, 1 == true, -1 == uninitialized.

u32 big_endian_32(u32 int_with_machine_endianness){
    return platform_big_endian_32(int_with_machine_endianness);
}
u16 big_endian_16(u16 int_with_machine_endianness){
    return platform_big_endian_16(int_with_machine_endianness);
}

u32 machine_endian_from_big_32(u32 big_endian_int){
    return platform_machine_endian_from_big_32(big_endian_int);
} 
u16 machine_endian_from_big_16(u16 big_endian_int){
    return platform_machine_endian_from_big_16(big_endian_int);
}


u32 machine_endian_from_little_32(u32 i){
    Assert(machine_is_big_endian != -1);
    
    if(machine_is_big_endian)
        return ((i >> 24) & 0xff) | ((i << 8) & 0xff0000) | ((i >> 8) & 0xff00) | ((i << 24) & 0xff000000);
    else
        return i;
} 
u16 machine_endian_from_little_16(u16 i){
    Assert(machine_is_big_endian != -1);

    if(machine_is_big_endian)
        return (i >> 8) | (i << 8);
    else
        return i;
}



template<typename T>
void clear_and_set_to_copy_of(T *dest, T &to_copy, Allocator_ID allocator)
{
    clear(dest, allocator);
    *dest = copy_of(&to_copy, allocator);
}


u64 next_power_of(u64 x, u64 power)
{
    auto mod = x % power;
    if(mod > 0)
        x += (power - mod);
    return x;
}

void ensure_buffer_set_capacity(u64 required_capacity, u64 *capacity,
                                void ***buffers, size_t *element_sizes,
                                u64 num_buffers, Allocator_ID allocator,
                                u64 min_capacity = 1)
{
    Assert(num_buffers <= 8);
    u8 *new_buffers[8] = {0};
    
    u64 old_capacity = *capacity;

    if(!realloc_needed(*capacity, required_capacity, capacity, min_capacity)) return;
    
    size_t total_size = 0;

    for(u64 i = 0; i < num_buffers; i++) {
        size_t element_size = element_sizes[i];
        size_t buffer_size  = next_power_of(element_size * (*capacity), 64);

        new_buffers[i] = (u8 *)total_size; // NOTE Temporarily store offset for each buffer.
        total_size += buffer_size;
    }

    u8 *memory = alloc(total_size, allocator);

    for(u64 i = 0; i < num_buffers; i++) {
        new_buffers[i] += (u64)memory;
        memcpy(new_buffers[i], *(buffers[i]), element_sizes[i] * old_capacity);
    }

    if(old_capacity != 0) {
        Assert(*(buffers[0]) != NULL);
        dealloc(*(buffers[0]), allocator);
    }
    else {
        Assert(*(buffers[0]) == NULL);
    }
    
    for(u64 i = 0; i < num_buffers; i++) {
        *(buffers[i]) = new_buffers[i];
    }
}




void init_memory()
{
    for(int a = 0; a < ARRLEN(allocators); a++)
    {
        Allocator *alc = allocators[a];
        if(!alc) continue;
        
        init_allocator(alc);
    }
}
