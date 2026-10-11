// Native test of src/gge_gs3.cpp against the real game data: sprite 0 of every .gs3 file is decoded
// through the reconstructed code and its pixels compared (by CRC-32) with tools/gs3_to_png.py's
// independent decoder. Usage: test_gs3 <game dir> <manifest from gs3_to_png.py --manifest>
#include "main.h"
#include "ec_fileio.h"
#include "gge_gs3.h"
#include "g_strings.h"
#include "fake_platform.h"
#include <fstream>
#include <sstream>

string_definition_data * string_definitions = NULL;
int num_string_definitions = 0;

static int failures = 0;
#define CHECK( cond ) do { if ( !( cond ) ) { printf( "FAILED line %d: %s\n", __LINE__, #cond ); failures++; } } while ( 0 )

static unsigned int crc32_of( const void * data, size_t size )
{
    static unsigned int table[ 256 ];
    if ( !table[ 1 ] )
        for ( unsigned int i = 0; i < 256; i++ ) { unsigned int c = i; for ( int k = 0; k < 8; k++ ) c = ( c & 1 ) ? 0xEDB88320u ^ ( c >> 1 ) : c >> 1; table[ i ] = c; }
    unsigned int crc = 0xFFFFFFFFu;
    const unsigned char * p = (const unsigned char *)data;
    for ( size_t i = 0; i < size; i++ ) crc = table[ ( crc ^ p[ i ] ) & 0xFF ] ^ ( crc >> 8 );
    return crc ^ 0xFFFFFFFFu;
}

int main( int argc, char ** argv )
{
    if ( argc < 3 ) { printf( "usage: test_gs3 <game dir> <manifest>\n" ); return 2; }
    assert_system_init();
    memory_system_init();
    debug_log_start();
    strcpy( game_path, argv[ 1 ] );
    CHECK( packfile_load() );
    gge_gs3_init();

    std::vector<dword> pixels( 4096 * 4096 ), other( 4096 * 4096 );
    std::ifstream manifest( argv[ 2 ] );
    std::string name;
    int bits, width, height, files = 0, wrong_size = 0, wrong_pixels = 0, player_files = 0, monster_files = 0;
    unsigned int crc;
    while ( manifest >> name >> bits >> width >> height >> std::hex >> crc >> std::dec )
    {
        files++;
        dword id = gge_gs3_new_sprites( (char *)name.c_str() );
        if ( id == invalid_id ) { wrong_size++; continue; }

        int w = 0, h = 0;
        gge_gs3_get_sprite_size( id, 0, &w, &h );
        if ( w != width || h != height ) { wrong_size++; gge_gs3_free_sprites( id ); continue; }

        memset( pixels.data(), 0, (size_t)w * h * 4 );
        gge_gs3_buffer_data buffer = { pixels.data(), w, h, 0, 0, w, h };
        gge_gs3_decompress_into_buffer( id, 0, 0, 0, 1, &buffer );
        if ( crc32_of( pixels.data(), (size_t)w * h * 4 ) != crc ) { wrong_pixels++; printf( "pixels differ: %s\n", name.c_str() ); }

        // the fixed-width decoders must give the same picture
        if ( bits == 8 && w == player_texture_width && h <= player_texture_height && name.compare( 0, 7, "player\\" ) == 0 )
        {
            memset( other.data(), 0, (size_t)w * h * 4 );
            gge_gs3_decompress_player_graphics( id, other.data() );
            if ( memcmp( other.data(), pixels.data(), (size_t)w * h * 4 ) != 0 ) wrong_pixels++;
            player_files++;
        }
        if ( bits == 8 && w == monster_texture_width && ( h == 256 || h == 128 ) && name.find( "monsters\\" ) != std::string::npos )
        {
            memset( other.data(), 0, (size_t)w * h * 4 );
            gge_gs3_decompress_monster_graphics( id, other.data() );
            if ( memcmp( other.data(), pixels.data(), (size_t)w * h * 4 ) != 0 ) wrong_pixels++;
            monster_files++;
        }

        gge_gs3_free_sprites( id );
    }
    CHECK( files >= 700 );
    CHECK( wrong_size == 0 );
    CHECK( wrong_pixels == 0 );
    CHECK( player_files > 0 && monster_files > 0 );

    // clipping and the shared buffer, on the first UI sheet (320 x 160)
    char sheet[] = "ui_gfx\\ancient_tree.gs3";
    dword id = gge_gs3_new_sprites( sheet );
    gge_gs3_buffer_data full = { pixels.data(), 320, 160, 0, 0, 320, 160 };
    memset( pixels.data(), 0, 320 * 160 * 4 );
    gge_gs3_decompress_into_buffer( id, 0, 0, 0, 1, &full );
    gge_gs3_buffer_data clipped = { other.data(), 320, 160, 10, 5, 100, 50 };
    memset( other.data(), 0, 320 * 160 * 4 );
    gge_gs3_decompress_into_buffer( id, 0, 0, 0, 1, &clipped );
    bool clip_ok = true;
    for ( int y = 0; y < 160; y++ ) for ( int x = 0; x < 320; x++ )
    {
        bool inside = x >= 10 && x < 100 && y >= 5 && y < 50;
        if ( other[ y * 320 + x ] != ( inside ? pixels[ y * 320 + x ] : 0 ) ) clip_ok = false;
    }
    CHECK( clip_ok );

    dword * shared = gge_gs3_decompress( id, 0, true );
    CHECK( shared != NULL && memcmp( shared, pixels.data(), 320 * 160 * 4 ) == 0 );
    gge_gs3_free_sprites( id );
    CHECK( gge_gs3_new_sprites( sheet ) == id );                // the slot is reused
    gge_gs3_free();                                             // frees the sheet left open and the shared buffer
    CHECK( gge_gs3_decompress( id, 0, true ) == NULL );
    packfile_close();

    printf( "gs3: %d files (%d player, %d monster sheets through the fixed-width decoders); %s\n", files, player_files, monster_files,
        failures ? "FAILED" : "all checks passed" );
    return failures != 0;
}
