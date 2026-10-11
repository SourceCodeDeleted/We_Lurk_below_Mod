//-----------------------------------------------------------------------------
// gge_main.h
//
// Placeholder for the graphics engine's main header: only what the reconstructed files need so
// far. gge_get_device and gge_get_device_context are original names; they live in gge_main.cpp
// (their asserts say so) and are inlined into every caller, so they are written inline here.
// d3d_error and d3d_check are names chosen here.

#ifndef _GGE_MAIN_H_
#define _GGE_MAIN_H_

#include <d3d11.h>

extern ID3D11Device * d3d_device;
extern ID3D11DeviceContext * d3d_device_context;

inline ID3D11Device * gge_get_device()
{
    assert_exp( d3d_device );
    return d3d_device;
}

inline ID3D11DeviceContext * gge_get_device_context()
{
    assert_exp( d3d_device_context );
    return d3d_device_context;
}

// 0x40A950: when result is not S_OK, writes "DirectX 11 error." with the function, file and line
// to the debug log and returns true
bool d3d_error( HRESULT result, const char * file, const char * function, int line );

// one graphics module's start and stop functions; struct and table names are chosen here
struct gge_module
{
    void ( * init )();
    void ( * free )();
};
#define num_gge_modules     5
extern gge_module gge_module_tbl[ num_gge_modules ];

bool create_device_resources();
void gge_shutdown();
void gge_menu_render( dword clear_color );

#define d3d_check( result ) \
    do { if ( d3d_error( result, __FILE__, __FUNCSIG__, __LINE__ ) ) __debugbreak(); } while ( 0 )

#endif
