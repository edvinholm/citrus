






#define Zero(X) memset(&X, 0, sizeof(X))


#if DEBUG
#define app_dealloc_ignore_temporary_memory_test(Ptr) dealloc(Ptr, ALLOC_APP, true)
#else
#define app_dealloc_ignore_temporary_memory_test(Ptr) dealloc(Ptr, ALLOC_APP)
#endif






#define app_alloc(Size)      alloc(Size, ALLOC_APP)
#define game_alloc(Size)     alloc(Size, ALLOC_GAME)
#define gfx_alloc(Size)      alloc(Size, ALLOC_GFX)
#define ui_alloc(Size)       alloc(Size, ALLOC_UI)
#define server_alloc(Size)   alloc(Size, ALLOC_SERVER)
#define dev_alloc(Size)      alloc(Size, ALLOC_DEV)
#define platform_alloc(Size) alloc(Size, ALLOC_PLATFORM)

#define game_dealloc(Ptr)      dealloc(Ptr, ALLOC_GAME)
#define app_dealloc(Ptr)       dealloc(Ptr, ALLOC_APP)
#define gfx_dealloc(Size)      dealloc(Size, ALLOC_GFX)
#define ui_dealloc(Size)       dealloc(Size, ALLOC_UI)
#define server_dealloc(Size)   dealloc(Size, ALLOC_SERVER)
#define dev_dealloc(Size)      dealloc(Size, ALLOC_DEV)
#define platform_dealloc(Size) dealloc(Size, ALLOC_PLATFORM)



