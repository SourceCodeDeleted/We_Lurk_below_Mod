//-----------------------------------------------------------------------------
// ec_config.cpp
// Reconstructed from ILB.exe: default keys 0x401440, get_next_word 0x401640, config_set_value
// 0x401700, config_decode 0x402120 (get_next_line inlined), config_load 0x4022F0,
// config_file_add_line 0x4025E0, bool line helper 0x4026A0, config_save 0x402710
// (init_config_file_memory and free_config_file_memory inlined).
// See ec_config.h for which names are original.

#include "main.h"
#include "ec_fileio.h"
#include "ec_config.h"
#include "g_game_globals.h"
#include "pending.h"
#include <SDL.h>
#include <math.h>

#define max_config_file_lines       128
#define max_config_word_size        64

config_data config;

static char config_names[ num_config_strings ][ max_config_word_size ] =
{
    "version", "fullscreen", "borderless", "vsync",
    "window_width", "window_height", "camera_dist", "gamma",
    "screen_shake", "quality_lighting", "bloom", "music_volume",
    "combat_volume", "sfx_volume", "show_helm", "auto_walk_up_one",
    "always_show_item_names", "default_style", "hp_bar", "map_mode",
    "map_transparency", "latest_news", "first_tips", "rain",
    "pickup_seeds", "gamepad", "xbox_input", "direction_dig",
    "cloud_saves", "verbose_loading", "damage_numbers", "key_move_left",
    "key_move_right", "key_move_up", "key_crouch", "key_jump",
    "key_interact", "key_recharge", "key_sleep", "key_shift",
    "key_ctrl", "key_hotbar_1", "key_hotbar_alt_1", "key_hotbar_2",
    "key_hotbar_alt_2", "key_hotbar_3", "key_hotbar_alt_3", "key_hotbar_4",
    "key_hotbar_alt_4", "key_hotbar_5", "key_hotbar_alt_5", "key_hotbar_6",
    "key_hotbar_alt_6", "key_hotbar_7", "key_hotbar_alt_7", "key_hotbar_8",
    "key_hotbar_alt_8", "key_hotbar_9", "key_hotbar_alt_9", "key_hotbar_10",
    "key_hotbar_alt_10", "key_hotbar_11", "key_hotbar_alt_11", "key_hotbar_12",
    "key_hotbar_alt_12", "key_inventory", "key_inventory_alt", "key_crafting",
    "key_crafting_alt", "key_minimap", "key_minimap_alt", "key_minimap_up",
    "key_minimap_down", "key_survival", "key_enter", "key_escape",
    "key_recall", "key_story_log", "key_achievement_panel", "key_directional_dig",
    "key_quest_log",
};

static char config_filename[] = "ilb_config.txt";

static config_strings * config_file_save_data = NULL;       // 0x16B498C
static int num_config_file_lines = 0;                       // 0x16B4990
static char config_file_line[ max_config_line_size ];       // 0x16B4590

//-----------------------------------------------------------------------------

void config_set_default_keys()
{
    static const int default_keys[ num_config_keys ] =
    {
        'a',               // key_move_left
        'd',               // key_move_right
        'w',               // key_move_up
        's',               // key_crouch
        ' ',               // key_jump
        'e',               // key_interact
        'r',               // key_recharge
        'z',               // key_sleep
        SDLK_LSHIFT,       // key_shift
        SDLK_LCTRL,        // key_ctrl
        '1',               // key_hotbar_1
        0,                 // key_hotbar_alt_1
        '2',               // key_hotbar_2
        0,                 // key_hotbar_alt_2
        '3',               // key_hotbar_3
        0,                 // key_hotbar_alt_3
        '4',               // key_hotbar_4
        0,                 // key_hotbar_alt_4
        '5',               // key_hotbar_5
        0,                 // key_hotbar_alt_5
        '6',               // key_hotbar_6
        'f',               // key_hotbar_alt_6
        '7',               // key_hotbar_7
        0,                 // key_hotbar_alt_7
        '8',               // key_hotbar_8
        0,                 // key_hotbar_alt_8
        '9',               // key_hotbar_9
        'h',               // key_hotbar_alt_9
        '0',               // key_hotbar_10
        'g',               // key_hotbar_alt_10
        '-',               // key_hotbar_11
        'w',               // key_hotbar_alt_11
        '=',               // key_hotbar_12
        'q',               // key_hotbar_alt_12
        'i',               // key_inventory
        'b',               // key_inventory_alt
        'p',               // key_crafting
        0,                 // key_crafting_alt
        'm',               // key_minimap
        '\t',              // key_minimap_alt
        SDLK_UP,           // key_minimap_up
        SDLK_DOWN,         // key_minimap_down
        'v',               // key_survival
        '\r',              // key_enter
        27,                // key_escape
        't',               // key_recall
        'l',               // key_story_log
        'c',               // key_achievement_panel
        'x',               // key_directional_dig
        'k',               // key_quest_log
    };

    for ( int i = 0; i < num_config_keys; i++ ) config.keys[ i ] = default_keys[ i ];
}

//-----------------------------------------------------------------------------
// Copies one line (up to the CR) and returns its length.

static int get_next_line( byte * source, byte * source_end, char * dest )
{
    if ( !source ) { assert_exp( source ); return -1; }
    if ( !source_end ) { assert_exp( source_end ); return -1; }
    if ( !( source_end > source ) ) { assert_exp( source_end > source ); return -1; }

    int line_size = 0;
    while ( *source != '\r' && source < source_end )
    {
        if ( line_size >= max_config_line_size )
        {
            dest[ 0 ] = 0;
            return line_size;
        }
        dest[ line_size++ ] = *source++;
    }

    if ( line_size >= max_config_line_size )
    {
        dest[ 0 ] = 0;
        return line_size;
    }

    dest[ line_size ] = 0;
    return line_size;
}

//-----------------------------------------------------------------------------
// Skips blanks, copies one word and returns the word's length (the skipped blanks are not counted).

int get_next_word( char * source, char * source_end, char * dest )
{
    if ( !source ) { assert_exp( source ); return -1; }
    if ( !source_end ) { assert_exp( source_end ); return -1; }
    if ( !( source_end >= source ) ) { assert_exp( source_end >= source ); return -1; }
    if ( !dest ) { assert_exp( dest ); return -1; }

    while ( ( *source == ' ' || *source == '\t' ) && source < source_end ) source++;

    int word_size = 0;
    while ( *source != ' ' && *source != '\t' && source < source_end )
    {
        if ( word_size >= max_config_word_size )
        {
            dest[ 0 ] = 0;
            return word_size;
        }
        dest[ word_size++ ] = *source++;
    }

    if ( word_size >= max_config_word_size )
    {
        dest[ 0 ] = 0;
        return word_size;
    }

    dest[ word_size ] = 0;
    return word_size;
}

//-----------------------------------------------------------------------------

static float clamp_float( float value, float low, float high )
{
    if ( value < low ) return low;
    if ( value > high ) return high;
    return value;
}

// the camera distance that suits a window size; applied whenever either dimension is read
static void config_set_camera_dist_from_window()
{
    float tiles_across = ( (float)config.window_width / (float)config.window_height < 2.0f ) ? 40.0f : 53.0f;
    config.camera_dist = fminf( 31.0f, fmaxf( 20.0f, (float)( config.window_width / 32 ) / tiles_across * 20.0f ) );
}

//-----------------------------------------------------------------------------

static void config_set_value( int index, const char * value )
{
    switch ( index )
    {
    case config_version:                config.version = atoi( value ); break;
    case config_fullscreen:             config.fullscreen = atoi( value ) != 0; break;
    case config_borderless:             config.borderless = atoi( value ) != 0; break;
    case config_vsync:                  config.vsync = atoi( value ) != 0; break;

    case config_window_width:
        config.window_width = atoi( value );
        if ( config.window_width < 1024 ) config.window_width = 1024;
        config_set_camera_dist_from_window();
        break;

    case config_window_height:
        config.window_height = atoi( value );
        if ( config.window_height < 720 ) config.window_height = 720;
        config_set_camera_dist_from_window();
        break;

    case config_camera_dist:            config.camera_dist = clamp_float( (float)atof( value ), 20.0f, 41.5f ); break;
    case config_gamma:                  config.gamma = clamp_float( (float)atof( value ), 0.1f, 2.0f ); break;
    case config_screen_shake:           config.screen_shake = atoi( value ) != 0; break;

    case config_quality_lighting:
        config.quality_lighting = atoi( value );
        if ( config.quality_lighting < 0 ) config.quality_lighting = 0;
        else if ( config.quality_lighting > 3 ) config.quality_lighting = 2;    // sic: above 3 becomes 2
        break;

    case config_bloom:                  config.bloom = atoi( value ) != 0; break;
    case config_music_volume:           config.music_volume = clamp_float( (float)atof( value ), 0.0f, 1.0f ); break;
    case config_combat_volume:          config.combat_volume = clamp_float( (float)atof( value ), 0.0f, 1.0f ); break;
    case config_sfx_volume:             config.sfx_volume = clamp_float( (float)atof( value ), 0.0f, 1.0f ); break;
    case config_show_helm:              config.show_helm = atoi( value ) != 0; break;
    case config_auto_walk_up_one:       config.auto_walk_up_one = atoi( value ) != 0; break;
    case config_always_show_item_names: config.always_show_item_names = atoi( value ) != 0; break;

    case config_default_style:
        config.default_style = atoi( value );
        if ( config.default_style < 0 ) config.default_style = 0;
        else if ( config.default_style >= 5 ) config.default_style = 4;
        break;

    case config_hp_bar:                 config.hp_bar = atoi( value ) != 0; break;

    case config_map_mode:
        config.map_mode = atoi( value );
        if ( config.map_mode < 0 ) config.map_mode = 0;
        else if ( config.map_mode >= 4 ) config.map_mode = 3;
        break;

    case config_map_transparency:       config.map_transparency = clamp_float( (float)atof( value ), 0.0f, 1.0f ); break;

    case config_latest_news:
        config.latest_news = atoi( value );
        if ( config.latest_news < 0 ) config.latest_news = 0;
        else if ( config.latest_news > 245 ) config.latest_news = 245;
        break;

    case config_first_tips:             config.first_tips = atoi( value ) != 0; break;
    case config_rain:                   config.rain = atoi( value ) != 0; break;
    case config_pickup_seeds:           config.pickup_seeds = atoi( value ) != 0; break;
    case config_gamepad:                config.gamepad = atoi( value ) != 0; break;
    case config_xbox_input:             config.xbox_input = atoi( value ); break;
    case config_direction_dig:          config.direction_dig = atoi( value ) != 0; break;
    case config_cloud_saves:            config.cloud_saves = atoi( value ) != 0; break;
    case config_verbose_loading:        config.verbose_loading = atoi( value ) != 0; break;
    case config_damage_numbers:         config.damage_numbers = atoi( value ); break;

    default:
        if ( index >= first_config_key && index < num_config_strings ) config.keys[ index - first_config_key ] = atoi( value );
        break;
    }
}

//-----------------------------------------------------------------------------

void config_decode( byte * file_data, dword file_size )
{
    assert_return( file_data );
    assert_return( file_size > 0 );

    byte * source = file_data;
    byte * source_end = file_data + file_size;

    while ( source < source_end )
    {
        char line[ max_config_line_size ];
        char name[ max_config_word_size ];
        char value[ max_config_word_size ];

        int line_size = get_next_line( source, source_end, line );

        int name_size = get_next_word( line, &line[ line_size ], name );
        get_next_word( &line[ name_size ], &line[ line_size ], value );

        for ( int i = 0; i < num_config_strings; i++ )
        {
            if ( _stricmp( name, config_names[ i ] ) == 0 )
            {
                config_set_value( i, value );
                break;
            }
        }

        source += line_size + 2;
    }
}

//-----------------------------------------------------------------------------

static void init_config_file_memory()
{
    assert_return( config_file_save_data == NULL );
    config_file_save_data = (config_strings *)drb_malloc( max_config_file_lines * sizeof( config_strings ), __FILE__, __LINE__, NULL );
    assert_return( config_file_save_data != NULL );

    num_config_file_lines = 0;
}

//-----------------------------------------------------------------------------

static void free_config_file_memory()
{
    assert_return( config_file_save_data != NULL );

    drb_free( config_file_save_data );
    config_file_save_data = NULL;
    num_config_file_lines = 0;
}

//-----------------------------------------------------------------------------

void config_file_add_line( char * line )
{
    assert_return( config_file_save_data != NULL );
    assert_return( num_config_file_lines >= 0 );
    assert_return( num_config_file_lines < max_config_file_lines );

    char * dest = config_file_save_data[ num_config_file_lines ].string;
    strcpy( dest, line );
    num_config_file_lines++;
    strcat( dest, "\r\n" );
}

//-----------------------------------------------------------------------------

static void config_file_add_bool_line( char * name, bool value )
{
    strcpy( config_file_line, name );
    strcat( config_file_line, value ? " 1" : " 0" );
    config_file_add_line( config_file_line );
}

static void config_file_add_int_line( char * name, int value )
{
    sprintf( config_file_line, "%s %i", name, value );
    config_file_add_line( config_file_line );
}

static void config_file_add_float_line( char * name, float value )
{
    sprintf( config_file_line, "%s %0.2f", name, value );
    config_file_add_line( config_file_line );
}

//-----------------------------------------------------------------------------

void config_save( config_data * data )
{
    init_config_file_memory();

    for ( int i = 0; i < num_config_strings; i++ )
    {
        char * name = config_names[ i ];
        switch ( i )
        {
        case config_version:                config_file_add_int_line( name, data->version ); break;
        case config_fullscreen:             config_file_add_bool_line( name, data->fullscreen ); break;
        case config_borderless:             config_file_add_bool_line( name, data->borderless ); break;
        case config_vsync:                  config_file_add_bool_line( name, data->vsync ); break;
        case config_window_width:           config_file_add_int_line( name, data->window_width ); break;
        case config_window_height:          config_file_add_int_line( name, data->window_height ); break;
        case config_camera_dist:            config_file_add_float_line( name, data->camera_dist ); break;
        case config_gamma:                  config_file_add_float_line( name, data->gamma ); break;
        case config_screen_shake:           config_file_add_bool_line( name, data->screen_shake ); break;
        case config_quality_lighting:       config_file_add_int_line( name, data->quality_lighting ); break;
        case config_bloom:                  config_file_add_bool_line( name, data->bloom ); break;
        case config_music_volume:           config_file_add_float_line( name, data->music_volume ); break;
        case config_combat_volume:          config_file_add_float_line( name, data->combat_volume ); break;
        case config_sfx_volume:             config_file_add_float_line( name, data->sfx_volume ); break;
        case config_show_helm:              config_file_add_bool_line( name, data->show_helm ); break;
        case config_auto_walk_up_one:       config_file_add_bool_line( name, data->auto_walk_up_one ); break;
        case config_always_show_item_names: config_file_add_bool_line( name, data->always_show_item_names ); break;
        case config_default_style:          config_file_add_int_line( name, data->default_style ); break;
        case config_hp_bar:                 config_file_add_bool_line( name, data->hp_bar ); break;
        case config_map_mode:               config_file_add_int_line( name, data->map_mode ); break;
        case config_map_transparency:       config_file_add_float_line( name, data->map_transparency ); break;
        case config_latest_news:            config_file_add_int_line( name, data->latest_news ); break;
        case config_first_tips:             config_file_add_bool_line( name, data->first_tips ); break;
        case config_rain:                   config_file_add_bool_line( name, data->rain ); break;
        case config_pickup_seeds:           config_file_add_bool_line( name, data->pickup_seeds ); break;
        case config_gamepad:                config_file_add_bool_line( name, data->gamepad ); break;
        case config_xbox_input:             config_file_add_int_line( name, data->xbox_input ); break;
        case config_direction_dig:          config_file_add_bool_line( name, data->direction_dig ); break;
        case config_cloud_saves:            config_file_add_bool_line( name, data->cloud_saves ); break;
        case config_verbose_loading:        config_file_add_bool_line( name, data->verbose_loading ); break;
        case config_damage_numbers:         config_file_add_bool_line( name, data->damage_numbers != 0 ); break;
        default:                            config_file_add_int_line( name, data->keys[ i - first_config_key ] ); break;
        }
    }

    bool saved_flag = game_globals.unknown_2078;
    game_globals.unknown_2078 = false;
    appdata_file_text_save( config_filename, config_file_save_data, num_config_file_lines );
    game_globals.unknown_2078 = saved_flag;

    free_config_file_memory();
}

//-----------------------------------------------------------------------------
// Sets the defaults, then reads the file over them. A missing file is created; a file from an
// older version is brought up to date and written back.

void config_load()
{
    config.version = current_config_version;
    config.fullscreen = false;
    config.borderless = false;
    config.vsync = true;
    config.window_width = 1920;
    config.window_height = 1080;
    config.camera_dist = 30.0f;
    config.gamma = 1.0f;
    config.quality_lighting = 2;
    config.screen_shake = true;
    config.bloom = true;
    config.music_volume = 1.0f;
    config.combat_volume = 1.0f;
    config.sfx_volume = 1.0f;
    config.show_helm = true;
    config.auto_walk_up_one = true;
    config.always_show_item_names = false;
    config.default_style = 0;
    config.hp_bar = false;
    config.map_mode = 0;
    config.map_transparency = 0.7f;
    config.latest_news = 0;
    config.first_tips = false;
    config.rain = true;
    config.pickup_seeds = true;
    config.gamepad = false;
    config.xbox_input = 0;
    config.direction_dig = false;
    config.cloud_saves = true;
    config.verbose_loading = false;
    config.damage_numbers = 0;

    sub_4610D0( 1920, 1080 );
    config_set_default_keys();

    bool saved_flag = game_globals.unknown_2078;
    game_globals.unknown_2078 = false;

    if ( !appdata_file_exist( config_filename ) )
    {
        config_save( &config );
        game_globals.unknown_2078 = saved_flag;
        return;
    }

    dword file_size = 0;
    byte * file_data = appdata_file_load( config_filename, &file_size );
    config_decode( file_data, file_size );
    drb_free( file_data );

    // older files: each case falls through to the next
    bool old_version = config.version < current_config_version;
    switch ( config.version )
    {
    case 0:
        config_set_default_keys();
    case 1:
        if ( config.keys[ config_key_crafting_alt - first_config_key ] == 'c' ) config.keys[ config_key_crafting_alt - first_config_key ] = 0;
    case 2:
        config.camera_dist = 30.0f;
    case 3:
        config.quality_lighting = 2;
        config.version = current_config_version;
        break;
    }

    if ( old_version ) config_save( &config );

    sub_4610D0( config.window_width, config.window_height );
    game_globals.unknown_2078 = saved_flag;
}
