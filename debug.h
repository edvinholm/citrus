
#define CONCAT(A, B) A##B

#if DEBUG && OS_WINDOWS

int DEBUG_timed_block_depth = 0;


#define TIMED_BLOCK_(Ident, BlockIdent, BlockName, Limit)       \
    char *BlockIdent = ##BlockName;                                     \
    DEBUG_timed_block_depth++;                                          \
    s64 Ident = platform_performance_counter();                         \
                                                                        \
    defer({                                                             \
            s64 End = platform_performance_counter();                   \
            DEBUG_timed_block_depth--;                                  \
            float dur = (End - Ident)/(platform_performance_counter_frequency() / 1000.0f); \
            if(dur < Limit) return;                             \
                                                                        \
            int d = DEBUG_timed_block_depth+1;                            \
            while(--d) printf("    ");                               \
            printf("[PROF] %s: \t\t\t%.2fms", BlockIdent, dur); \
            if(Limit > 0) {                                             \
                printf(" \t [L]");                     \
            }                                                           \
            printf("\n");                                               \
        });

#define TIMED_BLOCK_WITH_LIMIT(Name, Limit)                                   \
    TIMED_BLOCK_(CONCAT(block_performance_counter_, __COUNTER__), CONCAT(performance_counter_block_, __COUNTER__), Name, Limit);

#define TIMED_FUNCTION_WITH_LIMIT(Limit) TIMED_BLOCK_WITH_LIMIT(##__FUNCTION__, Limit)

#define TIMED_BLOCK(Name) \
    TIMED_BLOCK_WITH_LIMIT(Name, 0)

#define TIMED_FUNCTION \
    TIMED_BLOCK(##__FUNCTION__)



#else
#define TIMED_BLOCK_WITH_LIMIT(Name, Limit)
#define TIMED_FUNCTION_WITH_LIMIT(Limit)
#define TIMED_BLOCK(Name)
#define TIMED_FUNCTION
#endif


#define Print_Fail Debug_Print("%s(%d): error: FAILURE In function %s", __FILE__, __LINE__, __FUNCTION__)
#define Fail_Msg(Message_Format, ...) { Print_Fail; Debug_Print(": "); Debug_Print(Message_Format, __VA_ARGS__); Debug_Print("\n"); return false; }

// IMPORTANT: NOTE: Some of Fail actually still happens in release mode...
#define Fail {         \
    Print_Fail;        \
    Debug_Print("\n"); \
                       \
    report_fail_location((char *)__FUNCTION__, (u64)__LINE__);  \
                       \
    return false;      \
}



#include <assert.h>

#if OS_ANDROID
#include <unistd.h>


inline
void __android_assert(bool value, char *expr, const char *file, int line) {
    if(!(value)){
        printf("Assert failed at %s:%d (%s).\n", file, line, expr);
        __android_log_assert(expr, "CATLA", "Assert failed at %s:%d (%s).\n", file, line, expr);
        sleep(5);
        abort();
    }
}

#endif



void release_assert(bool value, char *expr, const char *file, const char *function, int line,
                              const char *key1 = NULL, u64 value1 = 0,
                              const char *key2 = NULL, u64 value2 = 0,
                              const char *key3 = NULL, u64 value3 = 0,
                              const char *key4 = NULL, u64 value4 = 0,
                              const char *key5 = NULL, u64 value5 = 0,
                              const char *key6 = NULL, u64 value6 = 0,
                              const char *key7 = NULL, u64 value7 = 0,
                              const char *key8 = NULL, u64 value8 = 0) {
    if(!(value)){
        
#if DEBUG
        printf("Assert failed at %s:%d (%s).\n", file, line, expr);
#endif
        
#if OS_ANDROID
        __android_log_assert(expr, "CATLA", "Assertion '%s' failed in %s (line %d).\n", expr, function, line);
        sleep(5);
        abort();
#else
        assert(value);
#endif
    }
}



// RELEASE ASSERT //

#define Release_Assert(x)           release_assert(x, #x, __FILE__, __FUNCTION__, __LINE__)
#define Release_Assert_Args(x, ...) release_assert(x, #x, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)


// DEBUG ASSERT AND BREAK //
#if DEBUG
// If DEBUG:

#if OS_ANDROID
#define Assert(x) __android_assert(x, #x, __FILE__, __LINE__)
#else
#define Assert(x) assert(x)
#endif

#if OS_WINDOWS
#include <intrin.h>
#define Break() __debugbreak()
#else
#define Break()
#endif

#define Break_If(x) if(x) { Break(); }


#else /////////--------------
// If not DEBUG:

#define Break_If(x)
#define Assert(x)

#endif



#if DEBUG
#define Debug_Print(...) printf(__VA_ARGS__)
#else
#define Debug_Print(...)
#endif


