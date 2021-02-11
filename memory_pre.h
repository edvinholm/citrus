
enum Allocator_ID
{
    ALLOC_MALLOC,
    ALLOC_TMP,
    
    ALLOC_NONE_OR_NUM
};

u8 *alloc(size_t size, Allocator_ID allocator);


void dealloc(void *ptr, Allocator_ID allocator
#if DEBUG
    , bool DEBUG_ignore_temporary_memory_test = false
#endif
              );

