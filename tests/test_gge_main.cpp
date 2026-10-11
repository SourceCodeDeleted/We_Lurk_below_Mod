// Native test of src/gge_main.cpp (device half): device creation, the module table, a menu frame
// and shutdown, with an in-memory Direct3D device. Usage: test_gge_main <game dir>
#define FAKE_D3D_REAL_DEVICE_GLOBALS
#include "main.h"
#include "ec_fileio.h"
#include "ec_config.h"
#include "gge_gs3.h"
#include "gge_textures.h"
#include "gge_materials.h"
#include "gge_material_types.h"
#include "gge_models.h"
#include "g_strings.h"
#include "fake_platform.h"
#include "fake_d3d.h"

string_definition_data * string_definitions = NULL;
int num_string_definitions = 0;

static int render_surfaces_freed = 0;
void sub_40C5D0() { render_surfaces_freed++; }

static int failures = 0;
#define CHECK( cond ) do { if ( !( cond ) ) { printf( "FAILED line %d: %s\n", __LINE__, #cond ); failures++; } } while ( 0 )

int main( int argc, char ** argv )
{
    if ( argc < 2 ) { printf( "usage: test_gge_main <game dir>\n" ); return 2; }
    assert_system_init();
    memory_system_init();
    debug_log_start();
    strcpy( game_path, argv[ 1 ] );
    CHECK( packfile_load() );

    window_width = 1600;
    window_height = 900;
    game_globals.unknown_2044 = 2;              // two views: the depth buffer is one view wide

    CHECK( create_device_resources() );
    CHECK( d3d_device == &the_device && d3d_device_context == &the_context && the_swap_chain != NULL );
    const DXGI_SWAP_CHAIN_DESC & sc = the_swap_chain->desc;
    CHECK( sc.BufferDesc.Width == 1600 && sc.BufferDesc.Height == 900 && sc.BufferDesc.RefreshRate.Numerator == 60 );
    CHECK( sc.BufferDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM && sc.BufferCount == 1 && sc.Windowed && sc.OutputWindow == (HWND)0x1234 );
    CHECK( the_device.target_views == 1 && the_device.depth_views == 1 );
    CHECK( the_device.depth_textures.size() == 1 && the_device.depth_textures[ 0 ]->desc.Width == 800 && the_device.depth_textures[ 0 ]->desc.Format == DXGI_FORMAT_D16_UNORM );
    CHECK( the_device.depth_states[ 0 ]->desc.DepthEnable && the_device.depth_states[ 0 ]->desc.DepthFunc == D3D11_COMPARISON_LESS_EQUAL && !the_device.depth_states[ 0 ]->desc.StencilEnable );
    CHECK( the_device.rasters[ 0 ]->desc.CullMode == D3D11_CULL_NONE && the_device.rasters[ 0 ]->desc.FillMode == D3D11_FILL_SOLID && the_device.rasters[ 0 ]->desc.DepthClipEnable );

    // start the graphics modules in table order
    for ( int i = 0; i < num_gge_modules; i++ ) if ( gge_module_tbl[ i ].init ) gge_module_tbl[ i ].init();
    CHECK( the_device.layouts.size() == 14 );
    CHECK( view_port.width == 800.0f && window_port.width == 1600.0f && view_port.height == 900.0f );

    // a menu frame with one textured quad
    char sheet[] = "ui_gfx\\ancient_tree.gs3";
    dword texture = invalid_id;
    gge_texture_safe_new( texture, sheet );
    dword material = gge_material_new( material_type_ui, texture, 0 );
    dword quad = gge_model_standard_indexed_new( material, 4, 6 );
    float vertices[ 4 * 7 ] = { 0 };
    gge_model_add_standard_indexed_vertices( quad, 4, vertices, 6 );

    config.vsync = true;
    gge_menu_render( 0x00336699 );
    CHECK( the_context.clear_color[ 0 ] == 0x33 / 255.0f && the_context.clear_color[ 1 ] == 0x66 / 255.0f && the_context.clear_color[ 2 ] == 0x99 / 255.0f && the_context.clear_color[ 3 ] == 1.0f );
    CHECK( the_context.depth_clears == 1 && the_context.topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    CHECK( the_context.viewport.Width == 1600.0f && the_context.viewport.Height == 900.0f && the_context.viewport.MaxDepth == 1.0f );
    CHECK( the_context.raster == the_device.rasters[ 0 ] && the_context.depth_state == the_device.depth_states[ 0 ] );
    CHECK( the_context.draws.size() == 1 && the_context.draws[ 0 ].count == 6 );
    CHECK( the_swap_chain->presents.size() == 1 && the_swap_chain->presents[ 0 ] == 1 );
    config.vsync = false;
    gge_menu_render( 0 );
    CHECK( the_swap_chain->presents.back() == 0 );

    // shutdown frees the modules in reverse order and releases every Direct3D object
    gge_model_free( quad );
    gge_material_free( material );
    gge_texture_safe_free( texture );
    gge_shutdown();
    CHECK( render_surfaces_freed == 1 && d3d_device == NULL && d3d_device_context == NULL );
    if ( live_objects != 0 ) printf( "live objects: %d\n", live_objects );
    CHECK( live_objects == 0 );                 // the back buffer texture from GetBuffer is released by the game
                                                // right after it makes the view
    packfile_close();
    printf( failures ? "%d check(s) FAILED\n" : "gge_main: all checks passed\n", failures );
    return failures != 0;
}
