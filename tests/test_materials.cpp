// Native test of src/gge_materials.cpp (set-up and bookkeeping) and the texture material lists,
// with an in-memory Direct3D device and the real shader files from the pack.
// Usage: test_materials <game dir>
#include "main.h"
#include "ec_fileio.h"
#include "gge_gs3.h"
#include "gge_textures.h"
#include "gge_materials.h"
#include "gge_material_types.h"
#include "gge_models.h"
#include "ec_config.h"
#include "g_strings.h"
#include "fake_platform.h"
#include "fake_d3d.h"

string_definition_data * string_definitions = NULL;
int num_string_definitions = 0;

static int failures = 0;
#define CHECK( cond ) do { if ( !( cond ) ) { printf( "FAILED line %d: %s\n", __LINE__, #cond ); failures++; } } while ( 0 )

// the chain of materials starting at `head`, as a string of ids
static std::string chain( dword head )
{
    std::string s;
    for ( dword id = head; id != invalid_id; id = gge_material_get_next_id( id ) ) s += std::to_string( id ) + " ";
    return s;
}

int main( int argc, char ** argv )
{
    if ( argc < 2 ) { printf( "usage: test_materials <game dir>\n" ); return 2; }
    assert_system_init();
    memory_system_init();
    debug_log_start();
    strcpy( game_path, argv[ 1 ] );
    CHECK( packfile_load() );
    gge_gs3_init();
    gge_textures_init();
    gge_materials_init();

    // set-up: every type's two shaders from the pack, two constant buffers, two blend states, three samplers
    CHECK( the_device.vertex_shader_sizes.size() == 14 && the_device.pixel_shader_sizes.size() == 14 && the_device.layouts.size() == 14 );
    CHECK( the_device.layouts[ material_type_plain ]->num_elements == 2 && the_device.layouts[ material_type_overlay ]->num_elements == 4 );
    CHECK( the_device.layouts[ material_type_ui ]->last_offset == 24 && the_device.layouts[ material_type_alpha_ui ]->last_offset == 16 );
    CHECK( d3d_errors == 0 );
    CHECK( the_device.buffers.size() == 2 && the_device.buffers[ 0 ]->desc.ByteWidth == 128 && the_device.buffers[ 1 ]->desc.ByteWidth == 32 );
    CHECK( the_device.buffers[ 0 ]->desc.BindFlags == D3D11_BIND_CONSTANT_BUFFER );
    CHECK( the_device.blends.size() == 2 && the_device.blends[ 0 ]->desc.RenderTarget[ 0 ].BlendEnable && !the_device.blends[ 1 ]->desc.RenderTarget[ 0 ].BlendEnable );
    CHECK( the_device.blends[ 0 ]->desc.RenderTarget[ 0 ].SrcBlend == D3D11_BLEND_SRC_ALPHA && the_device.blends[ 1 ]->desc.RenderTarget[ 0 ].RenderTargetWriteMask == 15 );
    CHECK( the_device.samplers.size() == 3 );
    CHECK( the_device.samplers[ 0 ]->desc.Filter == D3D11_FILTER_MIN_MAG_MIP_POINT && the_device.samplers[ 0 ]->desc.AddressU == D3D11_TEXTURE_ADDRESS_CLAMP );
    CHECK( the_device.samplers[ 1 ]->desc.AddressU == D3D11_TEXTURE_ADDRESS_WRAP && the_device.samplers[ 1 ]->desc.AddressV == D3D11_TEXTURE_ADDRESS_CLAMP );
    CHECK( the_device.samplers[ 2 ]->desc.Filter == D3D11_FILTER_MIN_MAG_MIP_LINEAR && the_device.samplers[ 2 ]->desc.MaxLOD > 1e30f );

    // materials on a texture are chained newest first
    char sheet[] = "ui_gfx\\ancient_tree.gs3";
    dword texture = gge_texture_add_empty_from_gs3file( sheet );
    dword m0 = gge_material_new( 3, texture, 5 );
    dword m1 = gge_material_new( 4, texture, 6 );
    dword m2 = gge_material_new( 5, texture, 7 );
    CHECK( m0 == 0 && m1 == 1 && m2 == 2 );
    CHECK( gge_material_get_vertex_size( m0 ) == 28 && gge_material_get_vertex_size( m1 ) == 24 && gge_material_get_rendering_layer( m2 ) == 7 );
    CHECK( chain( m2 ) == "2 1 0 " );

    // materials without a texture go on their own list
    dword n0 = gge_material_new( 0, invalid_id, 0 );
    dword n1 = gge_material_new( 0, invalid_id, 0 );
    CHECK( chain( n1 ) == "4 3 " && n0 == 3 );

    // removal from the middle, the head and the tail
    gge_material_free( m1 );
    CHECK( chain( m2 ) == "2 0 " );
    CHECK( gge_material_new( 9, texture, 1 ) == 1 );            // the slot is reused, and it becomes the head
    CHECK( chain( 1 ) == "1 2 0 " );
    gge_material_free( 1 );
    gge_material_free( m0 );
    CHECK( chain( m2 ) == "2 " );

    // a texture cannot be freed while a material uses it
    gge_texture_free( texture );                                // asserts, texture stays half-freed but counted
    gge_material_free( m2 );
    gge_material_free( n0 );
    gge_material_free( n1 );

    // rejects
    CHECK( gge_material_new( 14, texture, 0 ) == invalid_id );
    CHECK( gge_material_get_next_id( 200 ) == invalid_id );     // asserts: unused material

    // drawing: a sprite material with one quad, and the UI material that samples linearly
    gge_models_init();
    dword draw_texture = gge_texture_add_empty_from_gs3file( sheet );
    dword sprite = gge_material_new( material_type_sprite, draw_texture, 0 );
    dword quad = gge_model_standard_indexed_new( sprite, 4, 6 );
    float quad_vertices[ 4 * 7 ] = { 0 };
    gge_model_add_standard_indexed_vertices( quad, 4, quad_vertices, 6 );
    config.gamma = 1.5f;
    views[ 1 ].projection_matrix.m[ 0 ][ 0 ] = 7.0f;
    views[ 1 ].view_matrix.m[ 0 ][ 0 ] = 8.0f;
    screen_matrix.m[ 0 ][ 0 ] = 9.0f;

    gge_material_render( 1, sprite, 1 );
    CHECK( the_context.draws.size() == 1 && the_context.draws[ 0 ].indexed && the_context.draws[ 0 ].count == 6 && the_context.draws[ 0 ].stride == 28 );
    CHECK( the_context.layout == the_device.layouts[ material_type_sprite ] );
    CHECK( the_context.vertex_shader != NULL && the_context.pixel_shader != NULL );
    CHECK( the_context.vs_constants[ 0 ] == the_device.buffers[ 0 ] && the_context.ps_constants[ 1 ] == the_device.buffers[ 1 ] );
    CHECK( the_context.blend == the_device.blends[ 0 ] && the_context.blend_mask == 0xffffffff );      // sprites blend
    CHECK( the_context.sampler == the_device.samplers[ 0 ] && the_context.resource_view == gge_texture_get_texture_rv( draw_texture ) );
    const float * globals = (const float *)the_device.buffers[ 0 ]->bytes.data();
    CHECK( globals[ 0 ] == 7.0f && globals[ 16 ] == 8.0f );                                            // projection, then the view matrix
    const float * per_material = (const float *)the_device.buffers[ 1 ]->bytes.data();
    CHECK( per_material[ 0 ] == 0.25f && per_material[ 1 ] == 1.5f && per_material[ 4 ] == 1920.0f && per_material[ 5 ] == 1080.0f && per_material[ 6 ] == 1.0f );

    gge_material_render( 1, sprite, 0 );                        // screen space: the fixed matrix replaces the view matrix
    CHECK( globals[ 16 ] == 9.0f && the_context.draws.size() == 2 );

    dword ui = gge_material_new( material_type_bloom_blend, draw_texture, 0 );
    dword ui_quad = gge_model_standard_indexed_new( ui, 4, 6 );
    gge_material_render( 0, ui, 0 );                            // nothing to draw yet
    CHECK( the_context.draws.size() == 2 );
    gge_model_add_standard_indexed_vertices( ui_quad, 4, quad_vertices, 6 );
    gge_material_render( 0, ui, 0 );
    CHECK( the_context.draws.size() == 3 );
    CHECK( the_context.blend == the_device.blends[ 1 ] && the_context.sampler == the_device.samplers[ 2 ] );   // no blending, linear
    CHECK( the_context.layout == the_device.layouts[ material_type_bloom_blend ] );

    // layers: sprite is in layer 0 (drawn in pass 0), a second sprite in layer 3 (pass 1)
    dword late = gge_material_new( material_type_sprite, draw_texture, 3 );
    dword late_quad = gge_model_standard_indexed_new( late, 4, 6 );
    gge_model_add_standard_indexed_vertices( late_quad, 4, quad_vertices, 6 );
    size_t before = the_context.draws.size();
    gge_render_layers( 1, 1, 0 );                               // pass 0: layer 0 only -> sprite and ui
    CHECK( the_context.draws.size() == before + 2 );
    gge_render_layers( 1, 1, 1 );                               // pass 1: layer 3 -> late
    CHECK( the_context.draws.size() == before + 3 );
    gge_render_layers( 1, 0, 0 );                               // screen space: everything
    CHECK( the_context.draws.size() == before + 6 && globals[ 16 ] == 9.0f );

    // render target accounting
    dword surface = gge_texture_memory_rendering_surface_add( 100, 50 );
    CHECK( surface == 0 && gge_texture_memory_rendering_surface_add( 1, 1 ) == 1 );
    gge_texture_memory_rendering_surface_remove( surface );
    gge_texture_memory_rendering_surface_remove( 1 );
    gge_texture_memory_rendering_surface_remove( 1 );           // asserts: already free

    gge_models_free();
    gge_material_free( late );
    gge_material_free( ui );
    gge_material_free( sprite );
    gge_materials_free();
    gge_textures_free_all();
    gge_gs3_free();
    packfile_close();
    CHECK( live_objects == 0 );

    printf( failures ? "%d check(s) FAILED\n" : "materials: all checks passed\n", failures );
    return failures != 0;
}
