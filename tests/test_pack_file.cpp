// Native test of the reconstructed pack file code (src/ec_fileio.cpp) against the real ILB.gbp:
// every file is loaded through pack_file_load and compared byte for byte with the copy extracted by
// tools/gbp_unpack.py. Usage: test_pack_file <dir containing ILB.gbp> <dir extracted by gbp_unpack.py>
#include "main.h"
#include "ec_fileio.h"
#include "g_strings.h"
#include "fake_platform.h"
#include <filesystem>
#include <fstream>

string_definition_data * string_definitions = NULL;
int num_string_definitions = 0;

int main( int argc, char ** argv )
{
    if ( argc < 3 ) { printf( "usage: test_pack_file <game dir> <extracted dir>\n" ); return 2; }
    namespace fs = std::filesystem;

    assert_system_init();
    memory_system_init();
    debug_log_start();

    strcpy( game_path, argv[ 1 ] );
    if ( !packfile_load() ) { printf( "packfile_load failed\n" ); return 1; }

    int files = 0, mismatches = 0, missing = 0;
    unsigned long long bytes = 0;
    fs::path root( argv[ 2 ] );
    for ( const auto & entry : fs::recursive_directory_iterator( root ) )
    {
        if ( !entry.is_regular_file() ) continue;
        std::string name = fs::relative( entry.path(), root ).string();
        for ( char & c : name ) if ( c == '/' ) c = '\\';

        std::ifstream in( entry.path(), std::ios::binary );
        std::vector<char> expected( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );

        dword size = 0;
        byte * memory = pack_file_load( name.c_str(), &size );
        files++;
        if ( memory == NULL ) { missing++; printf( "not found: %s\n", name.c_str() ); continue; }
        if ( size != expected.size() || memcmp( memory, expected.data(), size ) != 0 )
        {
            mismatches++;
            printf( "MISMATCH: %s\n", name.c_str() );
        }
        bytes += size;
        pack_file_close( memory );
    }

    dword upper = pack_file_get_index( "XLS\\ITEMS.GBE" );   // other capitalisation: found by probing
    packfile_close();
    debug_log_save( true );
    memory_system_close();

    printf( "%d files, %llu bytes compared: %d mismatches, %d not found; upper-case lookup %s\n",
        files, bytes, mismatches, missing, upper == invalid_id ? "FAILED" : "ok" );
    return ( mismatches || missing || files == 0 || upper == invalid_id ) ? 1 : 0;
}
