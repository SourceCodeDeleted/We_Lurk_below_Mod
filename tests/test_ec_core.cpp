// Native test of the reconstructed engine core (ec_assert, ec_strings, ec_memory, ec_debug_log).
// Windows calls are replaced by the small fakes below; everything else is the code in src/.
// Build and run with tools/run_tests.sh.
#include "main.h"
#include "g_strings.h"
#include "fake_platform.h"

string_definition_data * string_definitions = NULL;
int num_string_definitions = 0;

static int failures = 0;
#define CHECK( cond ) do { if ( !( cond ) ) { printf( "FAILED line %d: %s\n", __LINE__, #cond ); failures++; } } while ( 0 )

static void fails_once( int * p ) { assert_return( p ); }

int main()
{
    assert_system_init();
    memory_system_init();
    debug_log_start();

    // safestring basics
    safestring a, b;
    safestring_create( &a, L"Hello" );
    CHECK( a.length == 5 && wcscmp( a.text, L"Hello" ) == 0 );
    safestring_concat_charstr( &a, ", world" );
    safestring_concat_char( &a, L'!' );
    CHECK( a.length == 13 && wcscmp( a.text, L"Hello, world!" ) == 0 );
    safestring_copy( &b, &a );
    CHECK( safestring_compare( &a, &b, true ) );
    safestring_create( &b, L"hello, WORLD!" );
    CHECK( safestring_compare( &a, &b, true ) );
    CHECK( !safestring_compare( &a, &b, false ) );
    safestring_truncate( &a, 5 );
    CHECK( a.length == 5 && a.text[ 5 ] == 0 );
    safestring_concat_int( &a, -42 );
    safestring_concat_float2( &a, 1.5f );
    safestring_concat_float1( &a, 2.25f );
    CHECK( wcscmp( a.text, L"Hello-421.502.2" ) == 0 || wcscmp( a.text, L"Hello-421.502.3" ) == 0 );
    safestring_create( &a, L"Press " );
    safestring_concat_key( &a, 32, true );
    CHECK( wcscmp( a.text, L"Press Space'" ) == 0 );
    char narrow[ 64 ];
    safestring_to_char( &a, narrow );
    CHECK( strcmp( narrow, "Press Space'" ) == 0 );
    safestring_create_from_id( &a, invalid_id );
    CHECK( a.length == 0 );

    // clamping at max_string_size - 1
    std::vector<wchar_t> big( 3000, L'x' );
    big.back() = 0;
    safestring_create( &a, big.data() );
    CHECK( a.length == max_string_size - 1 && a.text[ max_string_size - 1 ] == 0 );
    safestring_concat_str( &a, L"more" );
    CHECK( a.length == max_string_size - 1 );

    // string table lookups
    static string_definition_data table[ 2 ];
    safestring_create( &table[ 1 ].string, L"It Lurks Below" );
    string_definitions = table;
    num_string_definitions = 2;
    safestring_create_from_id( &a, 1 );
    safestring_concat_from_id( &a, 1 );
    CHECK( a.length == 28 );

    // tracked allocator
    void * p = drb_malloc( 100, "test.cpp", 1, "first" );
    void * q = drb_malloc( 200, "test.cpp", 2, NULL );
    CHECK( p && q && p != q );
    drb_free( p );
    memory_system_close();
    CHECK( fake_message_boxes == 2 );      // still allocated: q and the debug log buffer
    drb_free( q );

    // asserts: logged once per file and line, function returns early
    fails_once( NULL );
    fails_once( NULL );
    debug_log_format_line( L"value %i", 7 );
    debug_log_save( false );
    std::string written = fake_file_text( L"debug_log.txt" );
    size_t first = written.find( "ASSERT( p )" );
    CHECK( first != std::string::npos );
    CHECK( written.find( "ASSERT( p )", first + 1 ) == std::string::npos );
    CHECK( written.find( "Function: " ) != std::string::npos );
    CHECK( written.find( "File: " ) != std::string::npos );
    CHECK( written.find( "Line: " ) != std::string::npos );
    CHECK( written.find( "value 7\r\n" ) != std::string::npos );

    printf( failures ? "%d check(s) FAILED\n" : "all checks passed\n", failures );
    return failures != 0;
}
