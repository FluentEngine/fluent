#pragma once

#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#ifndef _WIN32
#include <alloca.h>
#else
#include <malloc.h>
#endif

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
typedef void*    ft_handle;

#ifdef FLUENT_DEBUG
#undef NDEBUG
#endif

#ifdef FLUENT_DEBUG
#define FT_ASSERT( x ) assert( x )
#else
#define FT_ASSERT( x ) ( void ) ( x )
#endif

#define DECLARE_FUNCTION_POINTER( ret, name, ... )                             \
	typedef ret ( *name##_fun )( __VA_ARGS__ );                                \
	extern name##_fun name##_impl

#define ALLOC_STACK_ARRAY( T, NAME, COUNT )                                    \
	T* NAME = alloca( sizeof( T ) * COUNT )

#define ALLOC_HEAP_ARRAY( T, NAME, COUNT )                                     \
	T* NAME = calloc( sizeof( T ), COUNT )

#define FT_UNUSED( X ) ( void ) ( X )
