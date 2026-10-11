// Native test of the save-file functions in src/ec_fileio.cpp against an in-memory file system.
#include "main.h"
#include "ec_fileio.h"
#include "g_strings.h"
#include "fake_platform.h"

string_definition_data * string_definitions = NULL;
int num_string_definitions = 0;

static int failures = 0;
#define CHECK( cond ) do { if ( !( cond ) ) { printf( "FAILED line %d: %s\n", __LINE__, #cond ); failures++; } } while ( 0 )

int main()
{
    assert_system_init();
    memory_system_init();
    debug_log_start();

    const std::wstring dir = L"C:\\Users\\test\\AppData\\Local\\Graybeard Games\\It Lurks Below\\";
    char name[] = "save_1.dat", other[] = "save_1.bak", missing[] = "nothing.dat";
    byte payload[ 100 ];
    for ( int i = 0; i < 100; i++ ) payload[ i ] = (byte)( i * 7 );

    CHECK( !appdata_file_exist( name ) );
    CHECK( appdata_file_save( name, payload, 100 ) );
    CHECK( appdata_file_exist( name ) );
    CHECK( fake_files.count( dir + L"save_1.dat" ) == 1 );
    CHECK( fake_directories.size() >= 2 && fake_directories[ 0 ] == L"C:\\Users\\test\\AppData\\Local\\Graybeard Games\\"
        && fake_directories[ 1 ] == dir );

    dword size = 0;
    byte * loaded = appdata_file_load( name, &size );
    CHECK( loaded && size == 100 && memcmp( loaded, payload, 100 ) == 0 );
    drb_free( loaded );

    byte buffer[ 200 ];
    size = 0;
    CHECK( appdata_file_load_into_mem( name, buffer, 200, &size ) && size == 100 && memcmp( buffer, payload, 100 ) == 0 );
    CHECK( !appdata_file_load_into_mem( name, buffer, 50, &size ) );            // buffer too small
    memset( buffer, 0, 200 );
    CHECK( appdata_file_load_partial_into_mem( name, buffer, 200, 10 ) && memcmp( buffer, payload, 10 ) == 0 && buffer[ 10 ] == 0 );
    CHECK( !appdata_file_load_partial_into_mem( name, buffer, 200, 150 ) );     // file is shorter than asked
    CHECK( !appdata_file_load_partial_into_mem( name, buffer, 10, 20 ) );       // more than the buffer

    CHECK( appdata_file_rename( name, other ) );
    CHECK( !appdata_file_exist( name ) && appdata_file_exist( other ) );
    CHECK( appdata_file_load( missing, &size ) == NULL );
    CHECK( !appdata_file_save( name, NULL, 10 ) && !appdata_file_save( name, payload, 0 ) );

    printf( failures ? "%d check(s) FAILED\n" : "appdata: all checks passed\n", failures );
    return failures != 0;
}
