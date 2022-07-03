#pragma once

#include <stdbool.h>
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

typedef void* ft_handle;

#ifdef FLUENT_DEBUG
#undef NDEBUG
#endif

#ifdef FLUENT_DEBUG
#define FT_ASSERT( x ) assert( x )
#else
#define FT_ASSERT( x ) ( void ) ( x )
#endif

#define FT_STATIC_ASSERT_IMPL( COND, MSG )                                     \
	typedef char static_assertion_##MSG[ ( !!( COND ) ) * 2 - 1 ]
#define FT_STATIC_ASSERT_LINE_IMPL( X, L )                                     \
	FT_STATIC_ASSERT_IMPL( X, at_line_##L )
#define FT_STATIC_ASSERT_LINE( X, L ) FT_STATIC_ASSERT_LINE_IMPL( X, L )
#define FT_STATIC_ASSERT( X )         FT_STATIC_ASSERT_LINE( X, __LINE__ )

#define FT_DECLARE_FUNCTION_POINTER( ret, name, ... )                          \
	typedef ret ( *name##_fun )( __VA_ARGS__ );                                \
	extern name##_fun name##_impl

#define FT_ALLOC_STACK_ARRAY( T, NAME, COUNT )                                 \
	T* NAME = alloca( sizeof( T ) * COUNT )

#define FT_ALLOC_HEAP_ARRAY( T, NAME, COUNT )                                  \
	T* NAME = calloc( sizeof( T ), COUNT )

#define FT_UNUSED( X ) ( void ) ( X )

#if defined( __clang__ ) || defined( __GNUC__ )
#define FT_INLINE static __attribute__( ( always_inline ) ) inline
#elif defined( _MSC_VER )
#define FT_INLINE __forceinline
#else
#define FT_INLINE static inline
#endif

#ifdef __cplusplus
#define FT_API extern "C"
#else
#define FT_API
#endif

FT_INLINE void
ft_safe_free( void* p )
{
	if ( p )
	{
		free( p );
	}
}

#define FT_MAX( a, b ) ( ( a ) > ( b ) ) ? ( a ) : ( b )
