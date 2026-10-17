// libm.h - the part of musl's internal src/internal/libm.h that sin.c, cos.c, atan.c, atan2.c and their
// helpers here use (written for this project; the definitions follow musl's, see COPYRIGHT). No system
// headers: the files are built inside the game, which has none (port/ilb_math.c includes them all).
#pragma once

typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned long long uint64_t;
typedef long long int64_t;
typedef double double_t;            // SSE2 arithmetic on every target: no wider evaluation

#ifndef FLT_EVAL_METHOD
#define FLT_EVAL_METHOD 0
#endif
#define DBL_EPSILON 2.22044604925031308085e-16

#define hidden static
#define predict_true( x )   __builtin_expect( !!( x ), 1 )
#define predict_false( x )  __builtin_expect( x, 0 )

static inline void fp_force_eval( double x )  { volatile double y; y = x; }
static inline void fp_force_evalf( float x )  { volatile float y; y = x; }
#define FORCE_EVAL( x ) do { if ( sizeof( x ) == sizeof( float ) ) fp_force_evalf( x ); else fp_force_eval( x ); } while ( 0 )

#define asuint64( f )   ( ( union { double _f; uint64_t _i; } ){ f } )._i
#define asdouble( i )   ( ( union { uint64_t _i; double _f; } ){ i } )._f
#define EXTRACT_WORDS( hi, lo, d ) do { uint64_t __u = asuint64( d ); ( hi ) = __u >> 32; ( lo ) = (uint32_t)__u; } while ( 0 )
#define GET_HIGH_WORD( hi, d )     do { ( hi ) = asuint64( d ) >> 32; } while ( 0 )
#define GET_LOW_WORD( lo, d )      do { ( lo ) = (uint32_t)asuint64( d ); } while ( 0 )
#define INSERT_WORDS( d, hi, lo )  do { ( d ) = asdouble( ( (uint64_t)( hi ) << 32 ) | (uint32_t)( lo ) ); } while ( 0 )
#define SET_HIGH_WORD( d, hi )     INSERT_WORDS( d, hi, (uint32_t)asuint64( d ) )
#define SET_LOW_WORD( d, lo )      INSERT_WORDS( d, asuint64( d ) >> 32, lo )

// exact operations, the same in every C library
#define fabs( x )   __builtin_fabs( x )
#define floor( x )  __builtin_floor( x )
#define isnan( x )  __builtin_isnan( x )

// musl's scalbn (src/math/scalbn.c), kept here so that nothing comes from the system's library
static double scalbn( double x, int n )
{
    union { double f; uint64_t i; } u;
    double_t y = x;
    if ( n > 1023 )
    {
        y *= 0x1p1023; n -= 1023;
        if ( n > 1023 ) { y *= 0x1p1023; n -= 1023; if ( n > 1023 ) n = 1023; }
    }
    else if ( n < -1022 )
    {
        y *= 0x1p-1022 * 0x1p53; n += 1022 - 53;
        if ( n < -1022 ) { y *= 0x1p-1022 * 0x1p53; n += 1022 - 53; if ( n < -1022 ) n = -1022; }
    }
    u.i = (uint64_t)( 0x3ff + n ) << 52;
    x = y * u.f;
    return x;
}

hidden int    __rem_pio2_large( double *, double *, int, int, int );
hidden int    __rem_pio2( double, double * );
hidden double __sin( double, double, int );
hidden double __cos( double, double );
