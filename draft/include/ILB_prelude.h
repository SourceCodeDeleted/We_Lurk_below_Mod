// Hand-written prelude for the mechanical draft: the handful of basic and Windows types that IDA's
// type export leaves undefined. Included before draft/build/ILB_draft_types.h.
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

typedef long HRESULT;
typedef long LSTATUS;
typedef int INT;
typedef int BOOL;
typedef unsigned int UINT;
typedef unsigned int DWORD;
typedef unsigned int REGSAM;
typedef const wchar_t *LPCWSTR;
typedef const wchar_t *LPCWCH;
typedef const char *LPCCH;
typedef const void *LPCVOID;
typedef DWORD *LPDWORD;
typedef BOOL *LPBOOL;
typedef void *HMODULE;
typedef void *HGDIOBJ;
typedef void *HDC;
typedef void *LPSECURITY_ATTRIBUTES;
typedef void *LPOVERLAPPED;
typedef void *LPTOP_LEVEL_EXCEPTION_FILTER;
typedef void *PWINDOWINFO;
typedef void *PSLIST_HEADER;
typedef void *_locale_t;
typedef long long __time64_t;
typedef struct type_info type_info;
typedef struct _EXCEPTION_POINTERS EXCEPTION_POINTERS;
typedef void *HBITMAP;
typedef float FLOAT;
typedef unsigned int uintptr_t;

// 16-byte values. IDA treats them as 128-bit integers (x = 0, x = (unsigned int)y), which this 32-bit target
// only has as _BitInt.
// No alignment is promised for any of the 16-byte types: the original code reads and writes them at whatever
// address its data happens to have (unaligned moves), and an optimising build would otherwise use the
// aligned instructions and fault. No structure of the game contains one, so no layout depends on this.
typedef unsigned _BitInt( 128 ) _OWORD __attribute__(( aligned( 1 ) ));

// SSE registers as IDA writes them: Microsoft's unions with named members. The ilb_ members are not
// Microsoft's; they make IDA's casts "(__m128)integer" valid C (GNU cast-to-union).
typedef union __attribute__(( packed, aligned( 1 ) )) __m128
{
    float m128_f32[ 4 ];
    unsigned __int64 m128_u64[ 2 ];
    __int8 m128_i8[ 16 ];
    __int16 m128_i16[ 8 ];
    __int32 m128_i32[ 4 ];
    __int64 m128_i64[ 2 ];
    unsigned __int8 m128_u8[ 16 ];
    unsigned __int16 m128_u16[ 8 ];
    unsigned __int32 m128_u32[ 4 ];
    int ilb_i32; unsigned int ilb_u32; long ilb_l32; unsigned long ilb_ul32; long long ilb_i64; unsigned long long ilb_u64; _OWORD ilb_u128;
} __m128;
typedef union __attribute__(( packed, aligned( 1 ) )) __m128i
{
    __int8 m128i_i8[ 16 ];
    __int16 m128i_i16[ 8 ];
    __int32 m128i_i32[ 4 ];
    __int64 m128i_i64[ 2 ];
    unsigned __int8 m128i_u8[ 16 ];
    unsigned __int16 m128i_u16[ 8 ];
    unsigned __int32 m128i_u32[ 4 ];
    unsigned __int64 m128i_u64[ 2 ];
    int ilb_i32; unsigned int ilb_u32; long ilb_l32; unsigned long ilb_ul32; long long ilb_i64; unsigned long long ilb_u64; _OWORD ilb_u128;
    __m128 ilb_ps;
} __m128i;
typedef union __attribute__(( packed, aligned( 1 ) )) __m128d
{
    double m128d_f64[ 2 ];
    int ilb_i32; unsigned int ilb_u32; long long ilb_i64; unsigned long long ilb_u64;
} __m128d;
