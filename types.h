

#include <stdint.h>
#include <float.h>

#define U64_MAX 0xFFFFFFFFFFFFFFFF
#define U32_MAX 0xFFFFFFFF
#define U16_MAX 0xFFFF
#define U8_MAX  0xFF

#define S32_MAX 0x7FFFFFFF



#if OS_WINDOWS
typedef wchar_t wchar;
#else
typedef wchar_t wchar;
#endif


typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#if 0
typedef int_fast8_t
typedef int_fast16_t
typedef int_fast32_t
typedef int_fast64_t
typedef int_least8_t
typedef int_least16_t
typedef int_least32_t
typedef int_least64_t
typedef intmax_t
typedef intintptr_t
#endif


typedef uint8_t byte;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#if 0
typedef uint_fast8_t
typedef uint_fast16_t
typedef uint_fast32_t
typedef uint_fast64_t
typedef uint_least8_t
typedef uint_least16_t
typedef uint_least32_t
typedef uint_least64_t
typedef uintmax_t
typedef uintintptr_t
#endif
