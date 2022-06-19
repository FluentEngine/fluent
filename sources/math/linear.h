#pragma once

#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include "base/base.h"

FT_INLINE float
radians( float degree )
{
	return degree * ( ( float ) M_PI / 180.0f );
}

#define LINMATH_H_DEFINE_VEC( n )                                              \
	typedef float float##n[ n ];                                               \
	FT_INLINE void float##n##_add( float##n       r,                           \
	                               float##n const a,                           \
	                               float##n const b )                          \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = a[ i ] + b[ i ];                    \
	}                                                                          \
	FT_INLINE void float##n##_sub( float##n       r,                           \
	                               float##n const a,                           \
	                               float##n const b )                          \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = a[ i ] - b[ i ];                    \
	}                                                                          \
	FT_INLINE void float##n##_scale( float##n       r,                         \
	                                 float##n const v,                         \
	                                 float const    s )                        \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = v[ i ] * s;                         \
	}                                                                          \
	FT_INLINE void float##n##_inv_scale( float##n       r,                     \
	                                     float##n const v,                     \
	                                     float const    s )                    \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = v[ i ] / s;                         \
	}                                                                          \
	FT_INLINE float float##n##_mul_inner( float##n const a, float##n const b ) \
	{                                                                          \
		float p = 0.f;                                                         \
		int   i;                                                               \
		for ( i = 0; i < n; ++i ) p += b[ i ] * a[ i ];                        \
		return p;                                                              \
	}                                                                          \
	FT_INLINE float float##n##_len( float##n const v )                         \
	{                                                                          \
		return sqrtf( float##n##_mul_inner( v, v ) );                          \
	}                                                                          \
	FT_INLINE void float##n##_norm( float##n r, float##n const v )             \
	{                                                                          \
		float k = 1.f / float##n##_len( v );                                   \
		float##n##_scale( r, v, k );                                           \
	}                                                                          \
	FT_INLINE void float##n##_min( float##n       r,                           \
	                               float##n const a,                           \
	                               float##n const b )                          \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = a[ i ] < b[ i ] ? a[ i ] : b[ i ];  \
	}                                                                          \
	FT_INLINE void float##n##_max( float##n       r,                           \
	                               float##n const a,                           \
	                               float##n const b )                          \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = a[ i ] > b[ i ] ? a[ i ] : b[ i ];  \
	}                                                                          \
	FT_INLINE void float##n##_dup( float##n r, float##n const src )            \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = src[ i ];                           \
	}                                                                          \
	FT_INLINE void float##n##_lerp( float##n       r,                          \
	                                float##n const a,                          \
	                                float##n const b,                          \
	                                float          t )                         \
	{                                                                          \
		for ( int i = 0; i < n; i++ )                                          \
			r[ i ] = ( a[ i ] * ( 1.0f - t ) ) + ( b[ i ] * t );               \
	}                                                                          \
	typedef int32_t int##n[ n ];                                               \
	FT_INLINE void int##n##_add( int##n r, int##n const a, int##n const b )    \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = a[ i ] + b[ i ];                    \
	}                                                                          \
	FT_INLINE void int##n##_sub( int##n r, int##n const a, int##n const b )    \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = a[ i ] - b[ i ];                    \
	}                                                                          \
	FT_INLINE void int##n##_scale( int##n r, int##n const v, int const s )     \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = v[ i ] * s;                         \
	}                                                                          \
	FT_INLINE void int##n##_inv_scale( int##n r, int##n const v, int const s ) \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = v[ i ] / s;                         \
	}                                                                          \
	FT_INLINE int32_t int##n##_mul_inner( int##n const a, int##n const b )     \
	{                                                                          \
		int32_t p = 0;                                                         \
		int     i;                                                             \
		for ( i = 0; i < n; ++i ) p += b[ i ] * a[ i ];                        \
		return p;                                                              \
	}                                                                          \
	FT_INLINE int32_t int##n##_len( int##n const v )                           \
	{                                                                          \
		return sqrt( int##n##_mul_inner( v, v ) );                             \
	}                                                                          \
	FT_INLINE void int##n##_norm( int##n r, int##n const v )                   \
	{                                                                          \
		int k = 1 / int##n##_len( v );                                         \
		int##n##_scale( r, v, k );                                             \
	}                                                                          \
	FT_INLINE void int##n##_min( int##n r, int##n const a, int##n const b )    \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = a[ i ] < b[ i ] ? a[ i ] : b[ i ];  \
	}                                                                          \
	FT_INLINE void int##n##_max( int##n r, int##n const a, int##n const b )    \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = a[ i ] > b[ i ] ? a[ i ] : b[ i ];  \
	}                                                                          \
	FT_INLINE void int##n##_dup( int##n r, int##n const src )                  \
	{                                                                          \
		int i;                                                                 \
		for ( i = 0; i < n; ++i ) r[ i ] = src[ i ];                           \
	}                                                                          \
	FT_INLINE void int##n##_lerp( int##n       r,                              \
	                              int##n const a,                              \
	                              int##n const b,                              \
	                              int32_t      t )                             \
	{                                                                          \
		for ( int i = 0; i < n; i++ )                                          \
			r[ i ] = ( a[ i ] * ( 1.0f - t ) ) + ( b[ i ] * t );               \
	}

LINMATH_H_DEFINE_VEC( 2 )
LINMATH_H_DEFINE_VEC( 3 )
LINMATH_H_DEFINE_VEC( 4 )

FT_INLINE void
float3_mul_cross( float3 r, float3 const a, float3 const b )
{
	r[ 0 ] = a[ 1 ] * b[ 2 ] - a[ 2 ] * b[ 1 ];
	r[ 1 ] = a[ 2 ] * b[ 0 ] - a[ 0 ] * b[ 2 ];
	r[ 2 ] = a[ 0 ] * b[ 1 ] - a[ 1 ] * b[ 0 ];
}

FT_INLINE void
float3_reflect( float3 r, float3 const v, float3 const n )
{
	float p = 2.f * float3_mul_inner( v, n );
	int   i;
	for ( i = 0; i < 3; ++i ) r[ i ] = v[ i ] - p * n[ i ];
}

FT_INLINE void
float4_mul_cross( float4 r, float4 const a, float4 const b )
{
	r[ 0 ] = a[ 1 ] * b[ 2 ] - a[ 2 ] * b[ 1 ];
	r[ 1 ] = a[ 2 ] * b[ 0 ] - a[ 0 ] * b[ 2 ];
	r[ 2 ] = a[ 0 ] * b[ 1 ] - a[ 1 ] * b[ 0 ];
	r[ 3 ] = 1.f;
}

FT_INLINE void
float4_reflect( float4 r, float4 const v, float4 const n )
{
	float p = 2.f * float4_mul_inner( v, n );
	int   i;
	for ( i = 0; i < 4; ++i ) r[ i ] = v[ i ] - p * n[ i ];
}

typedef float4 float4x4[ 4 ];
FT_INLINE void
float4x4_identity( float4x4 M )
{
	int i, j;
	for ( i = 0; i < 4; ++i )
		for ( j = 0; j < 4; ++j ) M[ i ][ j ] = i == j ? 1.f : 0.f;
}
FT_INLINE void
float4x4_dup( float4x4 M, float4x4 const N )
{
	int i;
	for ( i = 0; i < 4; ++i ) float4_dup( M[ i ], N[ i ] );
}
FT_INLINE void
float4x4_row( float4 r, float4x4 const M, int i )
{
	int k;
	for ( k = 0; k < 4; ++k ) r[ k ] = M[ k ][ i ];
}
FT_INLINE void
float4x4_col( float4 r, float4x4 const M, int i )
{
	int k;
	for ( k = 0; k < 4; ++k ) r[ k ] = M[ i ][ k ];
}
FT_INLINE void
float4x4_transpose( float4x4 M, float4x4 const N )
{
	// Note: if M and N are the same, the user has to
	// explicitly make a copy of M and set it to N.
	int i, j;
	for ( j = 0; j < 4; ++j )
		for ( i = 0; i < 4; ++i ) M[ i ][ j ] = N[ j ][ i ];
}
FT_INLINE void
float4x4_add( float4x4 M, float4x4 const a, float4x4 const b )
{
	int i;
	for ( i = 0; i < 4; ++i ) float4_add( M[ i ], a[ i ], b[ i ] );
}
FT_INLINE void
float4x4_sub( float4x4 M, float4x4 const a, float4x4 const b )
{
	int i;
	for ( i = 0; i < 4; ++i ) float4_sub( M[ i ], a[ i ], b[ i ] );
}
FT_INLINE void
float4x4_scale( float4x4 M, float4x4 const a, float k )
{
	int i;
	for ( i = 0; i < 4; ++i ) float4_scale( M[ i ], a[ i ], k );
}
FT_INLINE void
float4x4_scale_aniso( float4x4 M, float4x4 const a, float x, float y, float z )
{
	float4_scale( M[ 0 ], a[ 0 ], x );
	float4_scale( M[ 1 ], a[ 1 ], y );
	float4_scale( M[ 2 ], a[ 2 ], z );
	float4_dup( M[ 3 ], a[ 3 ] );
}
FT_INLINE void
float4x4_mul( float4x4 M, float4x4 const a, float4x4 const b )
{
	float4x4 temp;
	int      k, r, c;
	for ( c = 0; c < 4; ++c )
		for ( r = 0; r < 4; ++r )
		{
			temp[ c ][ r ] = 0.f;
			for ( k = 0; k < 4; ++k )
				temp[ c ][ r ] += a[ k ][ r ] * b[ c ][ k ];
		}
	float4x4_dup( M, temp );
}
FT_INLINE void
float4x4_mul_float4( float4 r, float4x4 const M, float4 const v )
{
	int i, j;
	for ( j = 0; j < 4; ++j )
	{
		r[ j ] = 0.f;
		for ( i = 0; i < 4; ++i ) r[ j ] += M[ i ][ j ] * v[ i ];
	}
}
FT_INLINE void
float4x4_translate( float4x4 T, float x, float y, float z )
{
	float4x4_identity( T );
	T[ 3 ][ 0 ] = x;
	T[ 3 ][ 1 ] = y;
	T[ 3 ][ 2 ] = z;
}
FT_INLINE void
float4x4_translate_in_place( float4x4 M, float x, float y, float z )
{
	float4 t = { x, y, z, 0 };
	float4 r;
	int    i;
	for ( i = 0; i < 4; ++i )
	{
		float4x4_row( r, M, i );
		M[ 3 ][ i ] += float4_mul_inner( r, t );
	}
}
FT_INLINE void
float4x4_from_float3_mul_outer( float4x4 M, float3 const a, float3 const b )
{
	int i, j;
	for ( i = 0; i < 4; ++i )
		for ( j = 0; j < 4; ++j )
			M[ i ][ j ] = i < 3 && j < 3 ? a[ i ] * b[ j ] : 0.f;
}
FT_INLINE void
float4x4_rotate( float4x4       R,
                 float4x4 const M,
                 float          x,
                 float          y,
                 float          z,
                 float          angle )
{
	float  s = sinf( angle );
	float  c = cosf( angle );
	float3 u = { x, y, z };

	if ( float3_len( u ) > 1e-4 )
	{
		float3_norm( u, u );
		float4x4 T;
		float4x4_from_float3_mul_outer( T, u, u );

		float4x4 S = { { 0, u[ 2 ], -u[ 1 ], 0 },
		               { -u[ 2 ], 0, u[ 0 ], 0 },
		               { u[ 1 ], -u[ 0 ], 0, 0 },
		               { 0, 0, 0, 0 } };
		float4x4_scale( S, S, s );

		float4x4 C;
		float4x4_identity( C );
		float4x4_sub( C, C, T );

		float4x4_scale( C, C, c );

		float4x4_add( T, T, C );
		float4x4_add( T, T, S );

		T[ 3 ][ 3 ] = 1.f;
		float4x4_mul( R, M, T );
	}
	else
	{
		float4x4_dup( R, M );
	}
}
FT_INLINE void
float4x4_rotate_X( float4x4 Q, float4x4 const M, float angle )
{
	float    s = sinf( angle );
	float    c = cosf( angle );
	float4x4 R = { { 1.f, 0.f, 0.f, 0.f },
	               { 0.f, c, s, 0.f },
	               { 0.f, -s, c, 0.f },
	               { 0.f, 0.f, 0.f, 1.f } };
	float4x4_mul( Q, M, R );
}
FT_INLINE void
float4x4_rotate_Y( float4x4 Q, float4x4 const M, float angle )
{
	float    s = sinf( angle );
	float    c = cosf( angle );
	float4x4 R = { { c, 0.f, -s, 0.f },
	               { 0.f, 1.f, 0.f, 0.f },
	               { s, 0.f, c, 0.f },
	               { 0.f, 0.f, 0.f, 1.f } };
	float4x4_mul( Q, M, R );
}
FT_INLINE void
float4x4_rotate_Z( float4x4 Q, float4x4 const M, float angle )
{
	float    s = sinf( angle );
	float    c = cosf( angle );
	float4x4 R = { { c, s, 0.f, 0.f },
	               { -s, c, 0.f, 0.f },
	               { 0.f, 0.f, 1.f, 0.f },
	               { 0.f, 0.f, 0.f, 1.f } };
	float4x4_mul( Q, M, R );
}
FT_INLINE void
float4x4_invert( float4x4 T, float4x4 const M )
{
	float s[ 6 ];
	float c[ 6 ];
	s[ 0 ] = M[ 0 ][ 0 ] * M[ 1 ][ 1 ] - M[ 1 ][ 0 ] * M[ 0 ][ 1 ];
	s[ 1 ] = M[ 0 ][ 0 ] * M[ 1 ][ 2 ] - M[ 1 ][ 0 ] * M[ 0 ][ 2 ];
	s[ 2 ] = M[ 0 ][ 0 ] * M[ 1 ][ 3 ] - M[ 1 ][ 0 ] * M[ 0 ][ 3 ];
	s[ 3 ] = M[ 0 ][ 1 ] * M[ 1 ][ 2 ] - M[ 1 ][ 1 ] * M[ 0 ][ 2 ];
	s[ 4 ] = M[ 0 ][ 1 ] * M[ 1 ][ 3 ] - M[ 1 ][ 1 ] * M[ 0 ][ 3 ];
	s[ 5 ] = M[ 0 ][ 2 ] * M[ 1 ][ 3 ] - M[ 1 ][ 2 ] * M[ 0 ][ 3 ];

	c[ 0 ] = M[ 2 ][ 0 ] * M[ 3 ][ 1 ] - M[ 3 ][ 0 ] * M[ 2 ][ 1 ];
	c[ 1 ] = M[ 2 ][ 0 ] * M[ 3 ][ 2 ] - M[ 3 ][ 0 ] * M[ 2 ][ 2 ];
	c[ 2 ] = M[ 2 ][ 0 ] * M[ 3 ][ 3 ] - M[ 3 ][ 0 ] * M[ 2 ][ 3 ];
	c[ 3 ] = M[ 2 ][ 1 ] * M[ 3 ][ 2 ] - M[ 3 ][ 1 ] * M[ 2 ][ 2 ];
	c[ 4 ] = M[ 2 ][ 1 ] * M[ 3 ][ 3 ] - M[ 3 ][ 1 ] * M[ 2 ][ 3 ];
	c[ 5 ] = M[ 2 ][ 2 ] * M[ 3 ][ 3 ] - M[ 3 ][ 2 ] * M[ 2 ][ 3 ];

	/* Assumes it is invertible */
	float idet = 1.0f / ( s[ 0 ] * c[ 5 ] - s[ 1 ] * c[ 4 ] + s[ 2 ] * c[ 3 ] +
	                      s[ 3 ] * c[ 2 ] - s[ 4 ] * c[ 1 ] + s[ 5 ] * c[ 0 ] );

	T[ 0 ][ 0 ] =
	    ( M[ 1 ][ 1 ] * c[ 5 ] - M[ 1 ][ 2 ] * c[ 4 ] + M[ 1 ][ 3 ] * c[ 3 ] ) *
	    idet;
	T[ 0 ][ 1 ] = ( -M[ 0 ][ 1 ] * c[ 5 ] + M[ 0 ][ 2 ] * c[ 4 ] -
	                M[ 0 ][ 3 ] * c[ 3 ] ) *
	              idet;
	T[ 0 ][ 2 ] =
	    ( M[ 3 ][ 1 ] * s[ 5 ] - M[ 3 ][ 2 ] * s[ 4 ] + M[ 3 ][ 3 ] * s[ 3 ] ) *
	    idet;
	T[ 0 ][ 3 ] = ( -M[ 2 ][ 1 ] * s[ 5 ] + M[ 2 ][ 2 ] * s[ 4 ] -
	                M[ 2 ][ 3 ] * s[ 3 ] ) *
	              idet;

	T[ 1 ][ 0 ] = ( -M[ 1 ][ 0 ] * c[ 5 ] + M[ 1 ][ 2 ] * c[ 2 ] -
	                M[ 1 ][ 3 ] * c[ 1 ] ) *
	              idet;
	T[ 1 ][ 1 ] =
	    ( M[ 0 ][ 0 ] * c[ 5 ] - M[ 0 ][ 2 ] * c[ 2 ] + M[ 0 ][ 3 ] * c[ 1 ] ) *
	    idet;
	T[ 1 ][ 2 ] = ( -M[ 3 ][ 0 ] * s[ 5 ] + M[ 3 ][ 2 ] * s[ 2 ] -
	                M[ 3 ][ 3 ] * s[ 1 ] ) *
	              idet;
	T[ 1 ][ 3 ] =
	    ( M[ 2 ][ 0 ] * s[ 5 ] - M[ 2 ][ 2 ] * s[ 2 ] + M[ 2 ][ 3 ] * s[ 1 ] ) *
	    idet;

	T[ 2 ][ 0 ] =
	    ( M[ 1 ][ 0 ] * c[ 4 ] - M[ 1 ][ 1 ] * c[ 2 ] + M[ 1 ][ 3 ] * c[ 0 ] ) *
	    idet;
	T[ 2 ][ 1 ] = ( -M[ 0 ][ 0 ] * c[ 4 ] + M[ 0 ][ 1 ] * c[ 2 ] -
	                M[ 0 ][ 3 ] * c[ 0 ] ) *
	              idet;
	T[ 2 ][ 2 ] =
	    ( M[ 3 ][ 0 ] * s[ 4 ] - M[ 3 ][ 1 ] * s[ 2 ] + M[ 3 ][ 3 ] * s[ 0 ] ) *
	    idet;
	T[ 2 ][ 3 ] = ( -M[ 2 ][ 0 ] * s[ 4 ] + M[ 2 ][ 1 ] * s[ 2 ] -
	                M[ 2 ][ 3 ] * s[ 0 ] ) *
	              idet;

	T[ 3 ][ 0 ] = ( -M[ 1 ][ 0 ] * c[ 3 ] + M[ 1 ][ 1 ] * c[ 1 ] -
	                M[ 1 ][ 2 ] * c[ 0 ] ) *
	              idet;
	T[ 3 ][ 1 ] =
	    ( M[ 0 ][ 0 ] * c[ 3 ] - M[ 0 ][ 1 ] * c[ 1 ] + M[ 0 ][ 2 ] * c[ 0 ] ) *
	    idet;
	T[ 3 ][ 2 ] = ( -M[ 3 ][ 0 ] * s[ 3 ] + M[ 3 ][ 1 ] * s[ 1 ] -
	                M[ 3 ][ 2 ] * s[ 0 ] ) *
	              idet;
	T[ 3 ][ 3 ] =
	    ( M[ 2 ][ 0 ] * s[ 3 ] - M[ 2 ][ 1 ] * s[ 1 ] + M[ 2 ][ 2 ] * s[ 0 ] ) *
	    idet;
}
FT_INLINE void
float4x4_orthonormalize( float4x4 R, float4x4 const M )
{
	float4x4_dup( R, M );
	float  s = 1.f;
	float3 h;

	float3_norm( R[ 2 ], R[ 2 ] );

	s = float3_mul_inner( R[ 1 ], R[ 2 ] );
	float3_scale( h, R[ 2 ], s );
	float3_sub( R[ 1 ], R[ 1 ], h );
	float3_norm( R[ 1 ], R[ 1 ] );

	s = float3_mul_inner( R[ 0 ], R[ 2 ] );
	float3_scale( h, R[ 2 ], s );
	float3_sub( R[ 0 ], R[ 0 ], h );

	s = float3_mul_inner( R[ 0 ], R[ 1 ] );
	float3_scale( h, R[ 1 ], s );
	float3_sub( R[ 0 ], R[ 0 ], h );
	float3_norm( R[ 0 ], R[ 0 ] );
}

FT_INLINE void
float4x4_frustum( float4x4 M,
                  float    l,
                  float    r,
                  float    b,
                  float    t,
                  float    n,
                  float    f )
{
	M[ 0 ][ 0 ] = 2.f * n / ( r - l );
	M[ 0 ][ 1 ] = M[ 0 ][ 2 ] = M[ 0 ][ 3 ] = 0.f;

	M[ 1 ][ 1 ] = 2.f * n / ( t - b );
	M[ 1 ][ 0 ] = M[ 1 ][ 2 ] = M[ 1 ][ 3 ] = 0.f;

	M[ 2 ][ 0 ] = ( r + l ) / ( r - l );
	M[ 2 ][ 1 ] = ( t + b ) / ( t - b );
	M[ 2 ][ 2 ] = -( f + n ) / ( f - n );
	M[ 2 ][ 3 ] = -1.f;

	M[ 3 ][ 2 ] = -2.f * ( f * n ) / ( f - n );
	M[ 3 ][ 0 ] = M[ 3 ][ 1 ] = M[ 3 ][ 3 ] = 0.f;
}
FT_INLINE void
float4x4_ortho( float4x4 M,
                float    l,
                float    r,
                float    b,
                float    t,
                float    n,
                float    f )
{
	M[ 0 ][ 0 ] = 2.f / ( r - l );
	M[ 0 ][ 1 ] = M[ 0 ][ 2 ] = M[ 0 ][ 3 ] = 0.f;

	M[ 1 ][ 1 ] = 2.f / ( t - b );
	M[ 1 ][ 0 ] = M[ 1 ][ 2 ] = M[ 1 ][ 3 ] = 0.f;

	M[ 2 ][ 2 ] = -2.f / ( f - n );
	M[ 2 ][ 0 ] = M[ 2 ][ 1 ] = M[ 2 ][ 3 ] = 0.f;

	M[ 3 ][ 0 ] = -( r + l ) / ( r - l );
	M[ 3 ][ 1 ] = -( t + b ) / ( t - b );
	M[ 3 ][ 2 ] = -( f + n ) / ( f - n );
	M[ 3 ][ 3 ] = 1.f;
}
FT_INLINE void
float4x4_perspective( float4x4 m, float y_fov, float aspect, float n, float f )
{
	/* NOTE: Degrees are an unhandy unit to work with.
	 * linmath.h uses radians for everything! */
	float const a = 1.f / tanf( y_fov / 2.f );

	m[ 0 ][ 0 ] = a / aspect;
	m[ 0 ][ 1 ] = 0.f;
	m[ 0 ][ 2 ] = 0.f;
	m[ 0 ][ 3 ] = 0.f;

	m[ 1 ][ 0 ] = 0.f;
	m[ 1 ][ 1 ] = a;
	m[ 1 ][ 2 ] = 0.f;
	m[ 1 ][ 3 ] = 0.f;

	m[ 2 ][ 0 ] = 0.f;
	m[ 2 ][ 1 ] = 0.f;
	m[ 2 ][ 2 ] = -( ( f + n ) / ( f - n ) );
	m[ 2 ][ 3 ] = -1.f;

	m[ 3 ][ 0 ] = 0.f;
	m[ 3 ][ 1 ] = 0.f;
	m[ 3 ][ 2 ] = -( ( 2.f * f * n ) / ( f - n ) );
	m[ 3 ][ 3 ] = 0.f;
}
FT_INLINE void
float4x4_look_at( float4x4     m,
                  float3 const eye,
                  float3 const center,
                  float3 const up )
{
	/* Adapted from Android's OpenGL Matrix.java.                        */
	/* See the OpenGL GLUT documentation for gluLookAt for a description */
	/* of the algorithm. We implement it in a straightforward way:       */

	/* TODO: The negation of of can be spared by swapping the order of
	 *       operands in the following cross products in the right way. */
	float3 f;
	float3_sub( f, center, eye );
	float3_norm( f, f );

	float3 s;
	float3_mul_cross( s, f, up );
	float3_norm( s, s );

	float3 t;
	float3_mul_cross( t, s, f );

	m[ 0 ][ 0 ] = s[ 0 ];
	m[ 0 ][ 1 ] = t[ 0 ];
	m[ 0 ][ 2 ] = -f[ 0 ];
	m[ 0 ][ 3 ] = 0.f;

	m[ 1 ][ 0 ] = s[ 1 ];
	m[ 1 ][ 1 ] = t[ 1 ];
	m[ 1 ][ 2 ] = -f[ 1 ];
	m[ 1 ][ 3 ] = 0.f;

	m[ 2 ][ 0 ] = s[ 2 ];
	m[ 2 ][ 1 ] = t[ 2 ];
	m[ 2 ][ 2 ] = -f[ 2 ];
	m[ 2 ][ 3 ] = 0.f;

	m[ 3 ][ 0 ] = 0.f;
	m[ 3 ][ 1 ] = 0.f;
	m[ 3 ][ 2 ] = 0.f;
	m[ 3 ][ 3 ] = 1.f;

	float4x4_translate_in_place( m, -eye[ 0 ], -eye[ 1 ], -eye[ 2 ] );
}

typedef float quat[ 4 ];
#define quat_add       float4_add
#define quat_sub       float4_sub
#define quat_norm      float4_norm
#define quat_scale     float4_scale
#define quat_mul_inner float4_mul_inner

FT_INLINE void
quat_identity( quat q )
{
	q[ 0 ] = q[ 1 ] = q[ 2 ] = 0.f;
	q[ 3 ]                   = 1.f;
}
FT_INLINE void
quat_mul( quat r, quat const p, quat const q )
{
	float3 w;
	float3_mul_cross( r, p, q );
	float3_scale( w, p, q[ 3 ] );
	float3_add( r, r, w );
	float3_scale( w, q, p[ 3 ] );
	float3_add( r, r, w );
	r[ 3 ] = p[ 3 ] * q[ 3 ] - float3_mul_inner( p, q );
}
FT_INLINE void
quat_conj( quat r, quat const q )
{
	int i;
	for ( i = 0; i < 3; ++i ) r[ i ] = -q[ i ];
	r[ 3 ] = q[ 3 ];
}
FT_INLINE void
quat_rotate( quat r, float angle, float3 const axis )
{
	float3 axis_norm;
	float3_norm( axis_norm, axis );
	float s = sinf( angle / 2 );
	float c = cosf( angle / 2 );
	float3_scale( r, axis_norm, s );
	r[ 3 ] = c;
}
FT_INLINE void
quat_mul_float3( float3 r, quat const q, float3 const v )
{
	/*
	 * Method by Fabian 'ryg' Giessen (of Farbrausch)
	t = 2 * cross(q.xyz, v)
	v' = v + q.w * t + cross(q.xyz, t)
	 */
	float3 t;
	float3 q_xyz = { q[ 0 ], q[ 1 ], q[ 2 ] };
	float3 u     = { q[ 0 ], q[ 1 ], q[ 2 ] };

	float3_mul_cross( t, q_xyz, v );
	float3_scale( t, t, 2 );

	float3_mul_cross( u, q_xyz, t );
	float3_scale( t, t, q[ 3 ] );

	float3_add( r, v, t );
	float3_add( r, r, u );
}
FT_INLINE void
float4x4_from_quat( float4x4 M, quat const q )
{
	float a  = q[ 3 ];
	float b  = q[ 0 ];
	float c  = q[ 1 ];
	float d  = q[ 2 ];
	float a2 = a * a;
	float b2 = b * b;
	float c2 = c * c;
	float d2 = d * d;

	M[ 0 ][ 0 ] = a2 + b2 - c2 - d2;
	M[ 0 ][ 1 ] = 2.f * ( b * c + a * d );
	M[ 0 ][ 2 ] = 2.f * ( b * d - a * c );
	M[ 0 ][ 3 ] = 0.f;

	M[ 1 ][ 0 ] = 2 * ( b * c - a * d );
	M[ 1 ][ 1 ] = a2 - b2 + c2 - d2;
	M[ 1 ][ 2 ] = 2.f * ( c * d + a * b );
	M[ 1 ][ 3 ] = 0.f;

	M[ 2 ][ 0 ] = 2.f * ( b * d + a * c );
	M[ 2 ][ 1 ] = 2.f * ( c * d - a * b );
	M[ 2 ][ 2 ] = a2 - b2 - c2 + d2;
	M[ 2 ][ 3 ] = 0.f;

	M[ 3 ][ 0 ] = M[ 3 ][ 1 ] = M[ 3 ][ 2 ] = 0.f;
	M[ 3 ][ 3 ]                             = 1.f;
}

FT_INLINE void
float4x4o_mul_quat( float4x4 R, float4x4 const M, quat const q )
{
	/*  XXX: The way this is written only works for orthogonal matrices. */
	/* TODO: Take care of non-orthogonal case. */
	quat_mul_float3( R[ 0 ], q, M[ 0 ] );
	quat_mul_float3( R[ 1 ], q, M[ 1 ] );
	quat_mul_float3( R[ 2 ], q, M[ 2 ] );

	R[ 3 ][ 0 ] = R[ 3 ][ 1 ] = R[ 3 ][ 2 ] = 0.f;
	R[ 0 ][ 3 ]                             = M[ 0 ][ 3 ];
	R[ 1 ][ 3 ]                             = M[ 1 ][ 3 ];
	R[ 2 ][ 3 ]                             = M[ 2 ][ 3 ];
	R[ 3 ][ 3 ] = M[ 3 ][ 3 ]; // typically 1.0, but here we make it general
}
FT_INLINE void
quat_from_float4x4( quat q, float4x4 const M )
{
	float r = 0.f;
	int   i;

	int  perm[] = { 0, 1, 2, 0, 1 };
	int *p      = perm;

	for ( i = 0; i < 3; i++ )
	{
		float m = M[ i ][ i ];
		if ( m < r )
			continue;
		m = r;
		p = &perm[ i ];
	}

	r = sqrtf( 1.f + M[ p[ 0 ] ][ p[ 0 ] ] - M[ p[ 1 ] ][ p[ 1 ] ] -
	           M[ p[ 2 ] ][ p[ 2 ] ] );

	if ( r < 1e-6 )
	{
		q[ 0 ] = 1.f;
		q[ 1 ] = q[ 2 ] = q[ 3 ] = 0.f;
		return;
	}

	q[ 0 ] = r / 2.f;
	q[ 1 ] = ( M[ p[ 0 ] ][ p[ 1 ] ] - M[ p[ 1 ] ][ p[ 0 ] ] ) / ( 2.f * r );
	q[ 2 ] = ( M[ p[ 2 ] ][ p[ 0 ] ] - M[ p[ 0 ] ][ p[ 2 ] ] ) / ( 2.f * r );
	q[ 3 ] = ( M[ p[ 2 ] ][ p[ 1 ] ] - M[ p[ 1 ] ][ p[ 2 ] ] ) / ( 2.f * r );
}

FT_INLINE void
float4x4_arcball( float4x4       R,
                  float4x4 const M,
                  float2 const   _a,
                  float2 const   _b,
                  float          s )
{
	float2 a;
	memcpy( a, _a, sizeof( a ) );
	float2 b;
	memcpy( b, _b, sizeof( b ) );

	float z_a = 0.;
	float z_b = 0.;

	if ( float2_len( a ) < 1. )
	{
		z_a = sqrtf( 1.0f - float2_mul_inner( a, a ) );
	}
	else
	{
		float2_norm( a, a );
	}

	if ( float2_len( b ) < 1. )
	{
		z_b = sqrtf( 1.0f - float2_mul_inner( b, b ) );
	}
	else
	{
		float2_norm( b, b );
	}

	float3 a_ = { a[ 0 ], a[ 1 ], z_a };
	float3 b_ = { b[ 0 ], b[ 1 ], z_b };

	float3 c_;
	float3_mul_cross( c_, a_, b_ );

	float const angle = acos( float3_mul_inner( a_, b_ ) ) * s;
	float4x4_rotate( R, M, c_[ 0 ], c_[ 1 ], c_[ 2 ], angle );
}

FT_INLINE void
slerp( quat qm, const quat qa, const quat qb, double t )
{
	// Calculate angle between them.
	double cos_half_theta = qa[ 3 ] * qb[ 3 ] + qa[ 0 ] * qb[ 0 ] +
	                        qa[ 1 ] * qb[ 1 ] + qa[ 2 ] * qb[ 2 ];

	// if qa=qb or qa=-qb then theta = 0 and we can return qa
	if ( abs( cos_half_theta ) >= 1.0 )
	{
		qm[ 3 ] = qa[ 3 ];
		qm[ 0 ] = qa[ 0 ];
		qm[ 1 ] = qa[ 1 ];
		qm[ 2 ] = qa[ 2 ];

		return;
	}

	// Calculate temporary values.
	double half_theta     = acos( cos_half_theta );
	double sin_half_theta = sqrt( 1.0 - cos_half_theta * cos_half_theta );

	// if theta = 180 degrees then result is not fully defined
	// we could rotate around any axis normal to qa or qb
	if ( fabs( sin_half_theta ) < 0.001 )
	{ // fabs is floating point absolute
		qm[ 3 ] = ( qa[ 3 ] * 0.5 + qb[ 3 ] * 0.5 );
		qm[ 0 ] = ( qa[ 0 ] * 0.5 + qb[ 0 ] * 0.5 );
		qm[ 1 ] = ( qa[ 1 ] * 0.5 + qb[ 1 ] * 0.5 );
		qm[ 2 ] = ( qa[ 2 ] * 0.5 + qb[ 2 ] * 0.5 );
		return;
	}

	double ratio_a = sin( ( 1 - t ) * half_theta ) / sin_half_theta;
	double ratio_b = sin( t * half_theta ) / sin_half_theta;
	// calculate quaternion.
	qm[ 3 ] = ( qa[ 3 ] * ratio_a + qb[ 3 ] * ratio_b );
	qm[ 0 ] = ( qa[ 0 ] * ratio_a + qb[ 0 ] * ratio_b );
	qm[ 1 ] = ( qa[ 1 ] * ratio_a + qb[ 1 ] * ratio_b );
	qm[ 2 ] = ( qa[ 2 ] * ratio_a + qb[ 2 ] * ratio_b );
}

FT_INLINE void
float4x4_compose( float4x4     r,
                  const float3 translation,
                  const quat   rotation,
                  const float3 scale )
{
	float tx = translation[ 0 ];
	float ty = translation[ 1 ];
	float tz = translation[ 2 ];
	float qx = rotation[ 0 ];
	float qy = rotation[ 1 ];
	float qz = rotation[ 2 ];
	float qw = rotation[ 3 ];
	float sx = scale[ 0 ];
	float sy = scale[ 1 ];
	float sz = scale[ 2 ];

	r[ 0 ][ 0 ] = ( 1 - 2 * qy * qy - 2 * qz * qz ) * sx;
	r[ 0 ][ 1 ] = ( 2 * qx * qy + 2 * qz * qw ) * sx;
	r[ 0 ][ 2 ] = ( 2 * qx * qz - 2 * qy * qw ) * sx;
	r[ 0 ][ 3 ] = 0.f;

	r[ 1 ][ 0 ] = ( 2 * qx * qy - 2 * qz * qw ) * sy;
	r[ 1 ][ 1 ] = ( 1 - 2 * qx * qx - 2 * qz * qz ) * sy;
	r[ 1 ][ 2 ] = ( 2 * qy * qz + 2 * qx * qw ) * sy;
	r[ 1 ][ 3 ] = 0.f;

	r[ 2 ][ 0 ] = ( 2 * qx * qz + 2 * qy * qw ) * sz;
	r[ 2 ][ 1 ] = ( 2 * qy * qz - 2 * qx * qw ) * sz;
	r[ 2 ][ 2 ] = ( 1 - 2 * qx * qx - 2 * qy * qy ) * sz;
	r[ 2 ][ 3 ] = 0.f;

	r[ 3 ][ 0 ] = tx;
	r[ 3 ][ 1 ] = ty;
	r[ 3 ][ 2 ] = tz;
	r[ 3 ][ 3 ] = 1.f;
}

FT_INLINE void
decompose_matrix( float3         translation,
                  quat           rotation,
                  float3         scale,
                  const float4x4 mat )
{
	// extract translation.
	memcpy( translation, mat[ 3 ], sizeof( float3 ) );

	// extract upper-left for determinant computation.
	const float a = mat[ 0 ][ 0 ];
	const float b = mat[ 0 ][ 1 ];
	const float c = mat[ 0 ][ 2 ];
	const float d = mat[ 1 ][ 0 ];
	const float e = mat[ 1 ][ 1 ];
	const float f = mat[ 1 ][ 2 ];
	const float g = mat[ 2 ][ 0 ];
	const float h = mat[ 2 ][ 1 ];
	const float i = mat[ 2 ][ 2 ];
	const float A = e * i - f * h;
	const float B = f * g - d * i;
	const float C = d * h - e * g;

	// extract scale.
	const float det    = a * A + b * B + c * C;
	float3      t0     = { a, b, c };
	float       scalex = float3_len( t0 );
	float3      t1     = { d, e, f };
	float       scaley = float3_len( t1 );
	float3      t2     = { g, h, i };
	float       scalez = float3_len( t2 );
	float3      s      = { scalex, scaley, scalez };

	if ( det < 0 )
	{
		float3_scale( scale, s, -1.0f );
	}
	else
	{
		float3_dup( scale, s );
	}

	// Remove scale from the matrix if it is not close to zero.
	float4x4 clone;
	float4x4_dup( clone, mat );
	if ( abs( det ) > 0.00001 )
	{
		float4_inv_scale( clone[ 0 ], clone[ 0 ], s[ 0 ] );
		float4_inv_scale( clone[ 1 ], clone[ 1 ], s[ 1 ] );
		float4_inv_scale( clone[ 2 ], clone[ 2 ], s[ 2 ] );

		// Extract rotation
		quat_from_float4x4( rotation, clone );
	}
	else
	{
		// Set to identity if close to zero
		quat_identity( rotation );
	}
}
