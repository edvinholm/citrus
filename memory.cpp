



bool equal(u8 *cs1, u8 *cs2);

template<typename T, Allocator_ID A>
void clear(Array<T, A> *arr);

struct String;





bool ensure_size(s64 required_size, Memory_Buffer *buffer)
{
    s64 new_size = max(128, buffer->size);
    
    while(new_size < required_size)
        new_size *= 2;

    if(new_size == buffer->size) return true;
    
    u8 *new_data = (u8 *)malloc(new_size);
    if(new_data == NULL) return false;
        
    if(buffer->size > 0) {
        Assert(buffer->data);
        memcpy(new_data, buffer->data, buffer->size);
        free(buffer->data);
    }
    else { Assert(buffer->data == NULL); }

    buffer->data = new_data;
    buffer->size = new_size;

    return true;
}


u64 platform_milliseconds();








inline
void reset_linear_allocator(Linear_Allocator *allocator)
{
    for(int i = 0; i < allocator->pages.n; i++)
    {
        if(i >= MAX_NUM_LINEAR_ALLOCATOR_PAGES_AFTER_RESET)
        {
            dealloc(allocator->pages[i].start, ALLOC_MALLOC);
            continue;
        }
        
        allocator->pages[i].used = 0;
    }

    allocator->pages.n = min(allocator->pages.n, MAX_NUM_LINEAR_ALLOCATOR_PAGES_AFTER_RESET);
}


inline
void reset_temporary_memory()
{
    reset_linear_allocator(&tmp_allocator);
}


//@BadName maybe. Does not give NEXT multiple if remainder is 0.
u64 next_multiple(u64 x, u64 factor)
{
    u64 remainder = x % factor;
    if(remainder > 0)
        x += factor - remainder;
    return x;
}

u8 *allocate(size_t size, Linear_Allocator *allocator)
{

    Linear_Allocator_Page *page = NULL;
    for(int p = tmp_allocator.pages.n - 1; p >= 0; p--)
    {
        Linear_Allocator_Page &pg = tmp_allocator.pages[p];
        
        if(pg.used + size <= pg.size)
        {
            page = &pg;
            break;
        }
    }

    if(page == NULL)
    {
        size_t page_size = LINEAR_ALLOCATOR_PAGE_SIZE;
        if(size > LINEAR_ALLOCATOR_PAGE_SIZE)
            page_size = size;
        
        Linear_Allocator_Page new_page = {0};
        new_page.start = alloc(page_size, ALLOC_MALLOC);
        new_page.size = page_size;
        array_add(tmp_allocator.pages, new_page);

        page = last_element_pointer(tmp_allocator.pages);
    }
    
    Assert((page->used % 64) == 0); // Assert new block is 64-byte aligned.

    u8 *result = page->start + page->used;
    page->used += next_multiple(size, 64); // Make sure next block is 64-byte aligned.

    Release_Assert(result);
    return result;
}


u8 *reallocate(u8 *old, size_t old_size, size_t new_size, Linear_Allocator *allocator)
{
    Linear_Allocator_Page *page = NULL;
    for(int p = tmp_allocator.pages.n - 1; p >= 0; p--)
    {
        Linear_Allocator_Page &pg = tmp_allocator.pages[p];
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

    auto *new_ = allocate(new_size, allocator);
    memcpy(new_, old, old_size);
    return new_;
}



inline
u8 *tmp_alloc(size_t size)
{
    return allocate(size, &tmp_allocator);
}

u8 *tmp_realloc(u8 *old, size_t old_size, size_t new_size)
{
    return reallocate(old, old_size, new_size, &tmp_allocator);
}


// @Cleanup: @ErrorProne: @Robustness If we pass the wrong number of arguments,
//                                    we might call the standard realloc.
inline
u8 *realloc(void *ptr, size_t old_size, size_t size, Allocator_ID allocator)
{
    //void *result = allocate(size, default_allocator);

    Assert(ARRLEN(allocators) == ALLOC_NONE_OR_NUM);
    Assert(allocator != ALLOC_NONE_OR_NUM);
    Assert(allocator < ARRLEN(allocators));

    switch(allocator)
    {
        case ALLOC_MALLOC: {
            auto *result = (u8 *)realloc(ptr, size);
            return result;
        } break;
            
        case ALLOC_TMP: return tmp_realloc((u8 *)ptr, old_size, size);

        default: Assert(false); return NULL;
    }
}


u8 *alloc(size_t size, Allocator *allocator)
{
    auto *linear = dynamic_cast<Linear_Allocator *>(allocator);
    if(linear != NULL) return allocate(size, linear);

    auto *malloc_ = dynamic_cast<Malloc_Allocator *>(allocator);
    if(malloc_ != NULL) return (u8 *)malloc(size);

    Assert(false);
    return NULL;
}


inline
u8 *alloc(size_t size, Allocator_ID allocator_id)
{
    //void *result = allocate(size, default_allocator);

    Assert(ARRLEN(allocators) == ALLOC_NONE_OR_NUM);
    Assert(allocator_id != ALLOC_NONE_OR_NUM);
    Assert(allocator_id < ARRLEN(allocators));

    Allocator *allocator = allocators[allocator_id];
    return alloc(size, allocator);
}

template<typename T>
T *alloc_elements(int n, Allocator_ID allocator)
{
    return (T *)alloc(sizeof(T)*n, allocator);
}

void dealloc(void *ptr, Allocator *allocator)
{    
    Assert(ptr);
    
    auto *linear = dynamic_cast<Linear_Allocator *>(allocator);
    if(linear != NULL)  { Assert(false); return; } // Not legal

    auto *malloc_ = dynamic_cast<Malloc_Allocator *>(allocator);
	if (malloc_ != NULL) { free(ptr); return; }

    Assert(false);
}

inline
void dealloc(void *ptr, Allocator_ID allocator_id)
{   
    return dealloc(ptr, allocators[allocator_id]);
}

void dealloc_if_legal(void *ptr, Allocator *allocator)
{
    auto *linear = dynamic_cast<Linear_Allocator *>(allocator);
    if(linear != NULL)  { return; } // Not legal

    dealloc(ptr, allocator);
}

void dealloc_if_legal(void *ptr, Allocator_ID allocator_id)
{
    return dealloc(ptr, allocators[allocator_id]);
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
                                u64 num_buffers, Allocator *allocator,
                                u64 min_capacity = 1)
{
    Assert(num_buffers <= 8);
    
    u64 old_capacity = *capacity;

    if(!realloc_needed(*capacity, required_capacity, capacity, min_capacity)) return;
    
    u8 *new_buffers[8] = {0};
    
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
        dealloc_if_legal(*(buffers[0]), allocator);
    }
    else {
        Assert(*(buffers[0]) == NULL);
    }
    
    for(u64 i = 0; i < num_buffers; i++) {
        *(buffers[i]) = new_buffers[i];
    }
}

inline
void dealloc_buffer_set(void *first_buffer, Allocator_ID allocator)
{
    dealloc(first_buffer, allocator);
}


