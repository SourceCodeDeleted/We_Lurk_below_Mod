// Native test of src/ec_excel.cpp against the real game data: the strings table through the text
// reader, the items table through the binary reader.
// Usage: test_excel <dir containing ILB.gbp> <dir extracted by gbp_unpack.py>
#include "main.h"
#include "ec_fileio.h"
#include "ec_excel.h"
#include "g_strings.h"
#include "fake_platform.h"
#include <fstream>
#include <sstream>

string_definition_data * string_definitions = NULL;
int num_string_definitions = 0;

static excel_data string_definition_list[] =
{
    excel_data_string( "string_token", string_definition_data, string_token ),
    { "string", excel_type_safestring, offsetof( string_definition_data, string ), 0, NULL },
};
static excel_parse_table string_excel_data = excel_table_define( "xls\\strings.txt", "xls\\strings.gbe",
    string_definition_list, string_definitions, num_string_definitions );

struct item_record { char item_type[ 64 ]; byte rest[ 192 ]; };     // 256 bytes, see db/definitions.h
static item_record * item_definitions = NULL;
static int num_item_definitions = 0;
static excel_data item_definition_list[] = { excel_data_string( "item_type", item_record, item_type ) };
static excel_parse_table item_excel_data = excel_table_define( "xls\\items.txt", "xls\\items.gbe",
    item_definition_list, item_definitions, num_item_definitions );

static int failures = 0;
#define CHECK( cond ) do { if ( !( cond ) ) { printf( "FAILED line %d: %s\n", __LINE__, #cond ); failures++; } } while ( 0 )

static bool text_is( int id, const wchar_t * expected )
{
    return wcscmp( string_definitions[ id ].string.text, expected ) == 0;
}

int main( int argc, char ** argv )
{
    if ( argc < 3 ) { printf( "usage: test_excel <game dir> <extracted dir>\n" ); return 2; }

    assert_system_init();
    memory_system_init();
    debug_log_start();
    strcpy( game_path, argv[ 1 ] );
    CHECK( packfile_load() );

    read_data_excel_file( &string_excel_data );
    CHECK( string_definitions != NULL );

    // independent reading of the same text file: every row's token must sit at its id
    std::ifstream in( std::string( argv[ 2 ] ) + "/xls/strings.txt", std::ios::binary );
    std::stringstream all;
    all << in.rdbuf();
    std::string line;
    int rows = 0, wrong_token = 0, wrong_text = 0, highest = -1;
    std::getline( all, line );      // header
    while ( std::getline( all, line ) )
    {
        if ( !line.empty() && line.back() == '\r' ) line.pop_back();
        size_t t1 = line.find( '\t' ), t2 = line.find( '\t', t1 + 1 );
        if ( line.empty() || t1 == std::string::npos || t2 == std::string::npos ) continue;
        std::string token = line.substr( 0, t1 ), text = line.substr( t2 + 1 );
        int id = atoi( line.substr( t1 + 1, t2 - t1 - 1 ).c_str() );
        if ( id > highest ) highest = id;
        rows++;
        if ( id >= num_string_definitions || token != string_definitions[ id ].string_token ) { wrong_token++; continue; }
        if ( text.find( '"' ) == std::string::npos && text.size() > 1 )   // quoted and 1-character cells are special
        {
            char narrow[ 2048 ];
            safestring_to_char( &string_definitions[ id ].string, narrow );
            if ( text.substr( 0, 1023 ) != narrow ) wrong_text++;
        }
    }
    CHECK( rows > 3000 );
    CHECK( num_string_definitions == highest + 1 );
    CHECK( wrong_token == 0 );
    CHECK( wrong_text == 0 );
    CHECK( text_is( 0, L"It Lurks Below" ) );
    CHECK( text_is( 1, L"Version " ) );                 // """Version """ in the file
    CHECK( text_is( 3235, L"Community" ) );
    CHECK( strcmp( string_definitions[ 3235 ].string_token, "mm_community" ) == 0 );
    CHECK( *(int *)( (byte *)string_definitions + num_string_definitions * sizeof( string_definition_data ) ) == -1 );

    read_binary_excel_file( &item_excel_data );
    CHECK( num_item_definitions == 1024 );
    CHECK( strcmp( item_definitions[ 0 ].item_type, "gold" ) == 0 );
    CHECK( strcmp( item_definitions[ 356 ].item_type, "adamantine_pickaxe" ) == 0 );

    free_binary_excel_file( &item_excel_data );
    free_data_excel_file( &string_excel_data );
    packfile_close();

    printf( "excel: %d string rows, %d definitions, %d items; %s\n", rows, num_string_definitions, num_item_definitions,
        failures ? "FAILED" : "all checks passed" );
    return failures != 0;
}
