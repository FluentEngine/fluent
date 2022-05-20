#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "assert.h"

typedef int8_t   i8;
typedef uint8_t  u8;
typedef int16_t  i16;
typedef uint16_t u16;
typedef int32_t  i32;
typedef uint32_t u32;
typedef int64_t  i64;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;
typedef int      b32;

#ifdef FLUENT_DEBUG
#undef NDEBUG
#endif

#ifdef FLUENT_DEBUG
#define FT_ASSERT( x ) assert( x )
#else
#define FT_ASSERT( x )
#endif

#define ALLOC_STACK_ARRAY( T, NAME, COUNT )                                    \
	T* NAME = ( T* ) alloca( sizeof( T ) * COUNT )
