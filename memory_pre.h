
enum Allocator_ID
{
    ALLOC_MALLOC,
    ALLOC_TMP,
    ALLOC_APP,
    ALLOC_GAME,
    ALLOC_GFX,
    ALLOC_UI,
    ALLOC_NETWORK,
    ALLOC_DEV,
    ALLOC_PLATFORM,

    ALLOC_RS,
    ALLOC_US,
    ALLOC_MS,
 
    ALLOC_NONE_OR_NUM
};

u8 *alloc(size_t size, Allocator_ID allocator);


void dealloc(void *ptr, Allocator_ID allocator
#if DEBUG
    , bool DEBUG_ignore_temporary_memory_test = false
#endif
              );




#if DEBUG
#define app_dealloc_ignore_temporary_memory_test(Ptr) dealloc(Ptr, ALLOC_APP, true)
#else
#define app_dealloc_ignore_temporary_memory_test(Ptr) dealloc(Ptr, ALLOC_APP)
#endif






#define app_alloc(Size)      alloc(Size, ALLOC_APP)
#define game_alloc(Size)     alloc(Size, ALLOC_GAME)
#define gfx_alloc(Size)      alloc(Size, ALLOC_GFX)
#define ui_alloc(Size)       alloc(Size, ALLOC_UI)
#define network_alloc(Size)  alloc(Size, ALLOC_NETWORK)
#define dev_alloc(Size)      alloc(Size, ALLOC_DEV)
#define platform_alloc(Size) alloc(Size, ALLOC_PLATFORM)

#define us_alloc(Size) alloc(Size, ALLOC_US)
#define rs_alloc(Size) alloc(Size, ALLOC_RS)
#define ms_alloc(Size) alloc(Size, ALLOC_MS)

#define game_dealloc(Ptr)      dealloc(Ptr, ALLOC_GAME)
#define app_dealloc(Ptr)       dealloc(Ptr, ALLOC_APP)
#define gfx_dealloc(Size)      dealloc(Size, ALLOC_GFX)
#define ui_dealloc(Size)       dealloc(Size, ALLOC_UI)
#define network_dealloc(Size)  dealloc(Size, ALLOC_NETWORK)
#define dev_dealloc(Size)      dealloc(Size, ALLOC_DEV)
#define platform_dealloc(Size) dealloc(Size, ALLOC_PLATFORM)

#define us_dealloc(Size) dealloc(Size, ALLOC_US)
#define rs_dealloc(Size) dealloc(Size, ALLOC_RS)
#define ms_dealloc(Size) dealloc(Size, ALLOC_MS)


