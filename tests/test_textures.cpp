// Native test of src/gge_textures.cpp with an in-memory Direct3D device: textures are created from
// real .gs3 files in the pack and their uploaded pixels compared with the sprite decoder's output.
// Usage: test_textures <game dir>
#include "main.h"
#include "ec_fileio.h"
#include "gge_main.h"
#include "gge_gs3.h"
#include "gge_textures.h"
#include "g_strings.h"
#include "fake_platform.h"
#include "fake_d3d.h"
#include "gge_materials.h"

material_type_data material_type_tbl[ num_material_types ];

string_definition_data * string_definitions = NULL;
int num_string_definitions = 0;

static int failures = 0;
#define CHECK( cond ) do { if ( !( cond ) ) { printf( "FAILED line %d: %s\n", __LINE__, #cond ); failures++; } } while ( 0 )

// the texture object behind a texture id: its resource view pointer is all the API exposes, so the
// test keeps the creation order instead
static bool texture_matches( fake_texture * t, const std::vector<dword> & expected, int width, int height )
{
    if ( t->width != width || t->height != height ) return false;
    for ( int y = 0; y < height; y++ )
        for ( int x = 0; x < width; x++ )
            if ( t->pixels[ (size_t)y * t->pitch + x ] != expected[ (size_t)y * width + x ] ) return false;
    return true;
}

int main( int argc, char ** argv )
{
    if ( argc < 2 ) { printf( "usage: test_textures <game dir>\n" ); return 2; }
    assert_system_init();
    memory_system_init();
    debug_log_start();
    strcpy( game_path, argv[ 1 ] );
    CHECK( packfile_load() );
    gge_gs3_init();
    gge_textures_init();

    // reference pixels straight from the sprite decoder
    char sheet[] = "ui_gfx\\ancient_tree.gs3";
    dword sprite_id = gge_gs3_new_sprites( sheet );
    int width, height;
    gge_gs3_get_sprite_size( sprite_id, 0, &width, &height );
    dword * decoded = gge_gs3_decompress( sprite_id, 0, true );
    std::vector<dword> expected( decoded, decoded + (size_t)width * height );
    gge_gs3_free_sprites( sprite_id );

    fake_device & recorder = the_device;
#define created textures

    dword a = invalid_id;
    gge_texture_safe_new( a, sheet );
    CHECK( a == 0 && recorder.created.size() == 1 );
    CHECK( texture_matches( recorder.created[ 0 ], expected, width, height ) );
    CHECK( recorder.created[ 0 ]->pixels[ width ] == 0xDEADBEEF );         // nothing written past the row width
    CHECK( gge_texture_get_texture_rv( a ) != NULL );

    dword b = gge_texture_add_empty_from_gs3file( sheet );
    CHECK( b == 1 && recorder.created[ 1 ]->width == width && recorder.created[ 1 ]->pixels[ 0 ] == 0xDEADBEEF );
    gge_texture_update_gs3file( b, sheet );
    CHECK( texture_matches( recorder.created[ 1 ], expected, width, height ) );

    // raw data, then an update with different data
    std::vector<dword> raw( 16 * 8 );
    for ( size_t i = 0; i < raw.size(); i++ ) raw[ i ] = 0x01000000u * ( i & 0xFF ) + i;
    dword c = invalid_id;
    gge_texture_safe_new_from_raw( c, raw.data(), 16, 8 );
    CHECK( c == 2 && texture_matches( recorder.created[ 2 ], raw, 16, 8 ) );
    for ( dword & v : raw ) v ^= 0x00FFFFFF;
    gge_texture_update( c, raw.data(), 16, 8 );
    CHECK( texture_matches( recorder.created[ 2 ], raw, 16, 8 ) );
    std::vector<dword> too_big( 32 * 8 );
    gge_texture_update( c, too_big.data(), 32, 8 );                         // asserts: larger than the texture
    CHECK( texture_matches( recorder.created[ 2 ], raw, 16, 8 ) );

    // a sheet combined with itself is the sheet
    gge_texture_update_combined_gs3files( b, sheet, sheet );
    CHECK( texture_matches( recorder.created[ 1 ], expected, width, height ) );

    // freeing: slots are reused, everything is released
    gge_texture_safe_free( a );
    CHECK( a == invalid_id && gge_texture_add_empty_from_gs3file( sheet ) == 0 );
    CHECK( gge_texture_get_texture_rv( 200 ) == NULL );                     // asserts: not active
    gge_textures_free_all();
    CHECK( live_objects == 0 && d3d_errors == 0 );
    gge_textures_init();                                                    // only legal when nothing is left

    gge_gs3_free();
    packfile_close();
    printf( failures ? "%d check(s) FAILED\n" : "textures: all checks passed\n", failures );
    return failures != 0;
}
