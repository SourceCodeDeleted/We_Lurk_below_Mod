// Native test of src/ec_config.cpp: defaults, the saved file's text, and loading an edited old-version file.
#include "main.h"
#include "ec_fileio.h"
#include "ec_config.h"
#include "g_strings.h"
#include "fake_platform.h"

string_definition_data * string_definitions = NULL;
int num_string_definitions = 0;

static int failures = 0;
#define CHECK( cond ) do { if ( !( cond ) ) { printf( "FAILED line %d: %s\n", __LINE__, #cond ); failures++; } } while ( 0 )

static const wchar_t * path = L"C:\\Users\\test\\AppData\\Local\\Graybeard Games\\It Lurks Below\\ilb_config.txt";

static bool has_line( const std::string & text, const char * line )
{
    return text.find( std::string( line ) + "\r\n" ) != std::string::npos;
}

int main()
{
    assert_system_init();
    memory_system_init();
    debug_log_start();

    // no file yet: defaults are applied and written out
    config_load();
    CHECK( config.version == 4 && config.window_width == 1920 && config.vsync && config.camera_dist == 30.0f );
    CHECK( config.keys[ config_key_jump - first_config_key ] == ' ' );
    CHECK( fake_last_ui_width == 1920 );

    std::string text = fake_file_text( path );
    int lines = 0;
    for ( char c : text ) if ( c == '\n' ) lines++;
    CHECK( lines == num_config_strings && num_config_strings == 81 );
    CHECK( text.compare( 0, 11, "version 4\r\n" ) == 0 );
    CHECK( has_line( text, "fullscreen 0" ) && has_line( text, "window_width 1920" ) );
    CHECK( has_line( text, "camera_dist 30.00" ) && has_line( text, "map_transparency 0.70" ) );
    CHECK( has_line( text, "key_move_left 97" ) && has_line( text, "key_shift 1073742049" ) && has_line( text, "key_quest_log 107" ) );

    // an edited version-1 file: values are clamped, the old binding is cleared, the file is rewritten
    std::string edited = "version 1\r\nwindow_width 800\r\nwindow_height 600\r\nmusic_volume 5\r\ngamma 0.01\r\n"
        "quality_lighting 9\r\nFULLSCREEN 1\r\nkey_crafting_alt 99\r\nkey_jump 120\r\nnot_a_setting 3\r\n";
    fake_files[ path ].assign( edited.begin(), edited.end() );
    config_load();
    CHECK( config.version == 4 );
    CHECK( config.window_width == 1024 && config.window_height == 720 );
    CHECK( config.music_volume == 1.0f && config.gamma == 0.1f );
    CHECK( config.quality_lighting == 2 );
    CHECK( config.fullscreen );                                             // names compare without case
    CHECK( config.keys[ config_key_crafting_alt - first_config_key ] == 0 );
    CHECK( config.keys[ config_key_jump - first_config_key ] == 120 );
    CHECK( config.camera_dist == 30.0f );                                   // version < 3 resets it
    CHECK( fake_last_ui_width == 1024 );
    text = fake_file_text( path );
    CHECK( text.compare( 0, 11, "version 4\r\n" ) == 0 && has_line( text, "window_width 1024" ) && has_line( text, "key_jump 120" ) );

    // a current-version file is read but not rewritten
    std::string current = "version 4\r\ncamera_dist 99\r\n";
    fake_files[ path ].assign( current.begin(), current.end() );
    config_load();
    CHECK( config.camera_dist == 41.5f );
    CHECK( fake_file_text( path ) == current );

    printf( failures ? "%d check(s) FAILED\n" : "config: all checks passed\n", failures );
    return failures != 0;
}
