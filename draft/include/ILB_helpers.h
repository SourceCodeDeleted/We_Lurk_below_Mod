// Hand-written helpers for the mechanical draft: things IDA's output uses that neither its type export nor
// its defs.h provide when compiling as C for this target. Included after ILB_defs.h.
#pragma once

#define nullptr 0

// IDA shows float<->int bit reinterpretation as COERCE_*
#define COERCE_INT( x )             ( *(int *)&( float ){ x } )
#define COERCE_UNSIGNED_INT( x )    ( *(unsigned int *)&( float ){ x } )
#define COERCE_FLOAT( x )           ( *(float *)&( unsigned int ){ x } )

#define qmemcpy( dest, src, size )  memcpy( dest, src, size )
void *memcpy( void *, const void *, size_t );
void *memset( void *, int, size_t );
size_t strlen( const char * );
size_t wcslen( const wchar_t * );
char *strcat( char *, const char * );
double sqrt( double ); double fabs( double ); double floor( double ); double ceil( double );
double atan2( double, double ); double pow( double, double ); double sin( double ); double cos( double );
float fminf( float, float ); float fmaxf( float, float );
static inline void *memset32( void *dest, unsigned int value, size_t count )
{
    unsigned int *p = (unsigned int *)dest;
    for ( size_t i = 0; i < count; i++ ) p[ i ] = value;
    return dest;
}

// ---- SSE values as IDA writes them: Microsoft's unions with named members, and the intrinsics as functions.
// Plain C implementations, so the draft needs no compiler header that would clash with those unions.
#define ILB_EACH( n )   for ( int i = 0; i < ( n ); i++ )
static inline __m128i _mm_cvtsi32_si128( int a )            { __m128i r = { 0 }; r.m128i_i32[ 0 ] = a; return r; }
static inline int _mm_cvtsi128_si32( __m128i a )            { return a.m128i_i32[ 0 ]; }
static inline __m128 _mm_cvtepi32_ps( __m128i a )           { __m128 r; ILB_EACH( 4 ) r.m128_f32[ i ] = (float)a.m128i_i32[ i ]; return r; }
static inline __m128 _mm_cvtpd_ps( __m128d a )              { __m128 r = { 0 }; ILB_EACH( 2 ) r.m128_f32[ i ] = (float)a.m128d_f64[ i ]; return r; }
static inline __m128 _mm_mul_ps( __m128 a, __m128 b )       { ILB_EACH( 4 ) a.m128_f32[ i ] *= b.m128_f32[ i ]; return a; }
static inline __m128 _mm_add_ps( __m128 a, __m128 b )       { ILB_EACH( 4 ) a.m128_f32[ i ] += b.m128_f32[ i ]; return a; }
static inline __m128 _mm_unpacklo_ps( __m128 a, __m128 b )
{
    __m128 r = { { a.m128_f32[ 0 ], b.m128_f32[ 0 ], a.m128_f32[ 1 ], b.m128_f32[ 1 ] } };
    return r;
}
static inline __m128 _mm_unpackhi_ps( __m128 a, __m128 b )
{
    __m128 r = { { a.m128_f32[ 2 ], b.m128_f32[ 2 ], a.m128_f32[ 3 ], b.m128_f32[ 3 ] } };
    return r;
}
static inline __m128 _mm_shuffle_ps( __m128 a, __m128 b, unsigned int imm )
{
    __m128 r = { { a.m128_f32[ imm & 3 ], a.m128_f32[ ( imm >> 2 ) & 3 ], b.m128_f32[ ( imm >> 4 ) & 3 ], b.m128_f32[ ( imm >> 6 ) & 3 ] } };
    return r;
}
static inline __m128i _mm_shuffle_epi32( __m128i a, unsigned int imm )
{
    __m128i r;
    ILB_EACH( 4 ) r.m128i_i32[ i ] = a.m128i_i32[ ( imm >> ( i * 2 ) ) & 3 ];
    return r;
}
static inline __m128i _mm_add_epi32( __m128i a, __m128i b )     { ILB_EACH( 4 ) a.m128i_u32[ i ] += b.m128i_u32[ i ]; return a; }
static inline __m128i _mm_mullo_epi32( __m128i a, __m128i b )   { ILB_EACH( 4 ) a.m128i_u32[ i ] *= b.m128i_u32[ i ]; return a; }
static inline __m128i _mm_sub_epi8( __m128i a, __m128i b )      { ILB_EACH( 16 ) a.m128i_u8[ i ] -= b.m128i_u8[ i ]; return a; }
static inline __m128i _mm_and_si128( __m128i a, __m128i b )     { ILB_EACH( 2 ) a.m128i_u64[ i ] &= b.m128i_u64[ i ]; return a; }
static inline __m128i _mm_or_si128( __m128i a, __m128i b )      { ILB_EACH( 2 ) a.m128i_u64[ i ] |= b.m128i_u64[ i ]; return a; }
static inline __m128i _mm_cmpgt_epi32( __m128i a, __m128i b )   { ILB_EACH( 4 ) a.m128i_i32[ i ] = a.m128i_i32[ i ] > b.m128i_i32[ i ] ? -1 : 0; return a; }
static inline __m128i _mm_srli_si128( __m128i a, unsigned int bytes )
{
    __m128i r = { 0 };
    for ( unsigned int i = 0; i + bytes < 16; i++ ) r.m128i_u8[ i ] = a.m128i_u8[ i + bytes ];
    return r;
}
static inline __m128i _mm_srai_epi32( __m128i a, unsigned int count )
{
    if ( count > 31 ) count = 31;
    ILB_EACH( 4 ) a.m128i_i32[ i ] >>= count;
    return a;
}
static inline __m128i _mm_srli_epi32( __m128i a, unsigned int count )
{
    ILB_EACH( 4 ) a.m128i_u32[ i ] = count > 31 ? 0 : a.m128i_u32[ i ] >> count;
    return a;
}
static inline __m128i _mm_slli_epi32( __m128i a, unsigned int count )
{
    ILB_EACH( 4 ) a.m128i_u32[ i ] = count > 31 ? 0 : a.m128i_u32[ i ] << count;
    return a;
}
// the variable-count forms take the count from the low 64 bits of a register
static inline unsigned int ILB_shift_count( __m128i c )         { return c.m128i_u64[ 0 ] > 32 ? 32 : (unsigned int)c.m128i_u64[ 0 ]; }
static inline __m128i _mm_sra_epi32( __m128i a, __m128i c )     { return _mm_srai_epi32( a, ILB_shift_count( c ) ); }
static inline __m128i _mm_srl_epi32( __m128i a, __m128i c )     { return _mm_srli_epi32( a, ILB_shift_count( c ) ); }
static inline __m128i _mm_sll_epi32( __m128i a, __m128i c )     { return _mm_slli_epi32( a, ILB_shift_count( c ) ); }
static inline __m128i _mm_unpacklo_epi8( __m128i a, __m128i b )
{
    __m128i r;
    ILB_EACH( 8 ) { r.m128i_u8[ i * 2 ] = a.m128i_u8[ i ]; r.m128i_u8[ i * 2 + 1 ] = b.m128i_u8[ i ]; }
    return r;
}
static inline __m128i _mm_unpacklo_epi16( __m128i a, __m128i b )
{
    __m128i r;
    ILB_EACH( 4 ) { r.m128i_u16[ i * 2 ] = a.m128i_u16[ i ]; r.m128i_u16[ i * 2 + 1 ] = b.m128i_u16[ i ]; }
    return r;
}

#define ILB_M128_BITS( x )      ( { __m128 ILB_tmp = ( x ); *(_OWORD *)&ILB_tmp; } )
// a float passed in a register that IDA typed as "box *" / as a double
#define COERCE_BOX_( x )        ( (box *)COERCE_UNSIGNED_INT( x ) )
#define COERCE_DOUBLE( x )      ( *(double *)&( unsigned long long ){ x } )
#define NAN                     __builtin_nanf( "" )
#define HKEY_CURRENT_USER       ( (HKEY)0x80000001 )
// the n-th 32-bit part of a wider value, as an lvalue
#define DWORD1( x )             ( *( (unsigned int *)&( x ) + 1 ) )
#define DWORD2( x )             ( *( (unsigned int *)&( x ) + 2 ) )
#define DWORD3( x )             ( *( (unsigned int *)&( x ) + 3 ) )
static inline unsigned int __ROL4__( unsigned int value, int count ) { count &= 31; return count ? ( value << count ) | ( value >> ( 32 - count ) ) : value; }
static inline unsigned int __ROR4__( unsigned int value, int count ) { count &= 31; return count ? ( value >> count ) | ( value << ( 32 - count ) ) : value; }
char *strcpy( char *, const char * );
wchar_t *wcscpy( wchar_t *, const wchar_t * );
int wcscmp( const wchar_t *, const wchar_t * );
double fmin( double, double ); double fmax( double, double );

// IDA's defs.h only implements the flag helpers for C++
#undef __OFSUB__
#undef __OFADD__
#undef __CFADD__
#undef __CFSUB__
#define __OFSUB__( x, y )   ( { __typeof__( x ) ILB_r; __builtin_sub_overflow( ( x ), ( __typeof__( x ) )( y ), &ILB_r ); } )
#define __OFADD__( x, y )   ( { __typeof__( x ) ILB_r; __builtin_add_overflow( ( x ), ( __typeof__( x ) )( y ), &ILB_r ); } )
#define __CFADD__( x, y )   ( (__typeof__( x ))( ( x ) + ( y ) ) < ( x ) )
#define __CFSUB__( x, y )   ( ( x ) < ( __typeof__( x ) )( y ) )

typedef HKEY *PHKEY;
#define COERCE_UNIT_( x )           ( (struct unit *)COERCE_UNSIGNED_INT( x ) )
#define COERCE_UNSIGNED_INT64( x )  ( *(unsigned long long *)&( double ){ x } )

// bit copies between 8-byte structs and 64-bit integers, and between the vector register types
#define ILB_FROM_INT64( type, x )   ( *(type *)&( unsigned long long ){ x } )
#define ILB_AS_INT64( x )           _Generic( ( x ), vector2d: *(__int64 *)&( x ), default: (__int64)_Generic( ( x ), vector2d: 0, default: ( x ) ) )
static inline __m128 ILB_m128_from_m128i( __m128i a )   { return a.ilb_ps; }
static inline __m128 ILB_m128_from_m128( __m128 a )     { return a; }
static inline __m128 ILB_m128_from_bits( _OWORD a )     { return (__m128)a; }
#define ILB_TO_M128( x )            _Generic( ( x ), __m128i: ILB_m128_from_m128i, __m128: ILB_m128_from_m128, default: ILB_m128_from_bits )( x )

// trace build (draft/glue/trace.c): the Direct3D device creation goes through a logging wrapper
#ifdef ILB_TRACE
#define D3D11CreateDeviceAndSwapChain ILB_trace_D3D11CreateDeviceAndSwapChain
#endif

// the rendering layer's calls go to the OpenGL backend (see the rule in tools/draft_fix.py)
#include "gge_gl_backend.h"
