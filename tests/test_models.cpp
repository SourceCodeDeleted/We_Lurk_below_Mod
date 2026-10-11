// Native test of src/gge_models.cpp and the material model lists, with an in-memory Direct3D device.
// Usage: test_models <game dir>
#include "main.h"
#include "ec_fileio.h"
#include "gge_gs3.h"
#include "gge_textures.h"
#include "gge_materials.h"
#include "gge_models.h"
#include "g_strings.h"
#include "fake_platform.h"
#include "fake_d3d.h"

string_definition_data * string_definitions = NULL;
int num_string_definitions = 0;

// one stand-in material type: 16-byte vertices, copied the way the real types do it
static const char * shader_name() { return "shaders\\dx11\\plain"; }
static int vertex_size() { return 16; }
static void vertex_copy( ID3D11Buffer * vertexbuffer, int start, int count, void * list )
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    d3d_device_context->Map( vertexbuffer, 0, start != 0 ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD, 0, &mapped );
    memcpy( (char *)mapped.pData + start * 16, list, count * 16 );
    d3d_device_context->Unmap( vertexbuffer, 0 );
}
#define ROW { shader_name, NULL, NULL, NULL, vertex_size, vertex_copy }
material_type_data material_type_tbl[ num_material_types ] = { ROW, ROW, ROW, ROW, ROW, ROW, ROW, ROW, ROW, ROW, ROW, ROW, ROW, ROW };

static int failures = 0;
#define CHECK( cond ) do { if ( !( cond ) ) { printf( "FAILED line %d: %s\n", __LINE__, #cond ); failures++; } } while ( 0 )

int main( int argc, char ** argv )
{
    if ( argc < 2 ) { printf( "usage: test_models <game dir>\n" ); return 2; }
    assert_system_init();
    memory_system_init();
    debug_log_start();
    strcpy( game_path, argv[ 1 ] );
    CHECK( packfile_load() );
    gge_gs3_init();
    gge_textures_init();
    gge_materials_init();
    size_t buffers_before = the_device.buffers.size();
    gge_models_init();

    // the shared quad index buffer: 2730 quads, each ( b, b+2, b+3 ) ( b, b+3, b+1 )
    CHECK( the_device.buffers.size() == buffers_before + 1 );
    fake_buffer * shared = the_device.buffers.back();
    CHECK( shared->desc.ByteWidth == 0x8000 && shared->desc.BindFlags == D3D11_BIND_INDEX_BUFFER );
    const word * idx = (const word *)shared->bytes.data();
    CHECK( idx[ 0 ] == 0 && idx[ 1 ] == 2 && idx[ 2 ] == 3 && idx[ 3 ] == 0 && idx[ 4 ] == 3 && idx[ 5 ] == 1 );
    CHECK( idx[ 6 ] == 4 && idx[ 7 ] == 6 && idx[ 11 ] == 5 );
    CHECK( idx[ 16379 ] == 4 * 2729 + 1 && idx[ 16380 ] == 0xEEEE );     // last quad written, nothing after it

    dword material = gge_material_new( 0, invalid_id, 0 );

    // plain model: vertices append, a second batch does not discard the first
    dword plain = gge_model_new( material, 8 );
    fake_buffer * plain_vb = the_device.buffers.back();
    CHECK( plain == 0 && plain_vb->desc.ByteWidth == 8 * 16 && plain_vb->desc.BindFlags == D3D11_BIND_VERTEX_BUFFER );
    CHECK( !gge_model_should_render( plain ) );
    dword verts[ 8 * 4 ];
    for ( int i = 0; i < 32; i++ ) verts[ i ] = 1000 + i;
    gge_model_add_vertices( plain, 3, verts );
    gge_model_add_vertices( plain, 2, verts + 12 );
    CHECK( memcmp( plain_vb->bytes.data(), verts, 5 * 16 ) == 0 && plain_vb->bytes[ 5 * 16 ] == 0xEE && plain_vb->discards == 1 );
    CHECK( gge_model_do_vertices_fit( plain, 3 ) && !gge_model_do_vertices_fit( plain, 4 ) );
    gge_model_add_vertices( plain, 4, verts );                           // asserts: would overflow
    CHECK( gge_model_should_render( plain ) );
    gge_model_render( plain, 16 );
    CHECK( the_context.draws.size() == 1 && !the_context.draws[ 0 ].indexed && the_context.draws[ 0 ].count == 5 && the_context.draws[ 0 ].stride == 16 );

    // indexed model: its own index buffer
    dword indexed = gge_model_indexed_new( material, 4, 6 );
    fake_buffer * indexed_ib = the_device.buffers.back();
    CHECK( indexed == 1 && indexed_ib->desc.ByteWidth == 12 && indexed_ib->desc.BindFlags == D3D11_BIND_INDEX_BUFFER );
    word tri[ 6 ] = { 0, 1, 2, 2, 1, 3 };
    gge_model_add_indexed_vertices( indexed, 4, verts, 3, tri );
    gge_model_add_indexed_vertices( indexed, 0, verts, 3, tri + 3 );
    CHECK( memcmp( indexed_ib->bytes.data(), tri, 12 ) == 0 );
    gge_model_add_vertices( indexed, 1, verts );                         // asserts: wrong kind of model
    gge_model_render( indexed, 16 );
    CHECK( the_context.draws.back().indexed && the_context.draws.back().count == 6 && the_context.draws.back().indexbuffer == indexed_ib );

    // standard indexed model: draws with the shared quad indices
    dword quads = gge_model_standard_indexed_new( material, 8, 12 );
    CHECK( quads == 2 && !gge_model_should_render( quads ) );
    gge_model_add_standard_indexed_vertices( quads, 8, verts, 12 );
    gge_model_render( quads, 16 );
    CHECK( the_context.draws.back().indexed && the_context.draws.back().count == 12 && the_context.draws.back().indexbuffer == shared );
    gge_model_reset_vertices( quads );
    CHECK( !gge_model_should_render( quads ) );
    CHECK( gge_model_standard_indexed_new( material, 8, 0x4000 ) == invalid_id );    // too many indices

    // the material's model list: newest first, removable from the middle
    CHECK( gge_model_get_next_id( quads ) == indexed && gge_model_get_next_id( indexed ) == plain && gge_model_get_next_id( plain ) == invalid_id );
    gge_model_free( indexed );
    CHECK( gge_model_get_next_id( quads ) == plain );
    CHECK( gge_model_new( material, 2 ) == 1 );                          // slot reused, becomes the head
    CHECK( gge_model_get_next_id( 1 ) == quads );

    gge_models_free();                                                   // frees the three models and the shared buffer
    gge_material_free( material );
    gge_materials_free();
    gge_textures_free_all();
    gge_gs3_free();
    packfile_close();
    CHECK( live_objects == 0 );

    printf( failures ? "%d check(s) FAILED\n" : "models: all checks passed\n", failures );
    return failures != 0;
}
