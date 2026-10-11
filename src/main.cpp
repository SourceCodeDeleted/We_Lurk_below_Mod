//-----------------------------------------------------------------------------
// main.cpp
// Reconstructed from ILB.exe: main 0x581B40 (reached from SDL's WinMain wrapper at 0x583AE0), with
// set_app_mode taken from its inlined copy in the main loop.
//
// Original names: set_app_mode, appmode_init, num_app_modes. main() is SDL_main. Everything it
// calls that is not reconstructed yet is declared in pending.h.
//
// Still to do in this file (see dump/by_file/_between_ec_excel_and_gdi_main.c):
//   0x57ECD0  loads all 22 binary tables and fixes them up
//   0x57F3B0  frees all tables
//   0x57FD70  appmode_init start: allocates world, unit and level-generator memory
//   0x57FFD0  appmode_exit start: frees sounds, models, materials, textures
//   0x5806A0  loads the shared graphics
//   0x580F50, 0x5811F0  two functions that test game_is_active
//   0x581280  SDL event dispatch (main_process_event in pending.h)
//   0x5817D0  client start-up, the non-server half of main's initialisation
//   the app_modes table contents (0x609910), which need every mode's functions declared

#include "main.h"
#include "main_app_modes.h"
#include "ec_fileio.h"
#include "ec_excel.h"
#include "ec_config.h"
#include "ec_fmod.h"
#include "ec_cursor.h"
#include "ec_controller.h"
#include "gge_main.h"
#include "g_game_globals.h"
#include "ec_save_memory.h"
#include "pending.h"
#include <SDL.h>
#include <windows.h>

int app_mode = appmode_init;
int next_app_mode = -1;
game_globals_data game_globals;

//-----------------------------------------------------------------------------

void set_app_mode( int mode )
{
    assert_return( mode >= appmode_init && mode < num_app_modes );

    if ( app_mode >= appmode_init && app_mode < num_app_modes )
    {
        if ( app_modes[ app_mode ].end ) app_modes[ app_mode ].end();
    }

    app_mode = mode;
    next_app_mode = -1;

    if ( app_modes[ mode ].start ) app_modes[ mode ].start();
}

//-----------------------------------------------------------------------------
// The game folder comes from HKCU\Software\Graybeard Games\ILB "Path", else the current directory.
// As in the binary, the registry value is read as wide characters straight into the 8-bit
// game_path, so a path found there is only usable up to its first character.

static bool main_find_game_path()
{
    HKEY key;
    if ( RegOpenKeyExW( HKEY_CURRENT_USER, L"Software\\Graybeard Games\\ILB", 0, KEY_READ, &key ) == ERROR_SUCCESS )
    {
        DWORD size = 1024;
        LSTATUS result = RegGetValueW( key, NULL, L"Path", RRF_RT_REG_SZ, NULL, game_path, &size );
        RegCloseKey( key );
        if ( result == ERROR_SUCCESS ) return true;
    }

    static wchar_t current_directory[ 1024 ];
    DWORD length = GetCurrentDirectoryW( 1024, current_directory );

    int i = 0;
    for ( ; current_directory[ i ] != 0; i++ ) game_path[ i ] = (char)current_directory[ i ];
    game_path[ i ] = 0;
    strcat( game_path, "\\" );

    return length != 0;
}

//-----------------------------------------------------------------------------

int main( int argc, char * argv[] )
{
    memset( &game_globals, 0, sizeof( game_globals ) );
    main_reset_startup_globals();

    for ( int i = 1; i < argc; i++ )
    {
        safestring_create_from_char( &temp_string, argv[ i ] );
        if ( temp_string.length == 2 && temp_string.text[ 0 ] == L'-' && ( temp_string.text[ 1 ] == L's' || temp_string.text[ 1 ] == L'S' ) )
        {
            debug_log_add_line( L"Server mode" );   // has no effect: the log is not started yet
            game_globals.server_mode = true;
        }
    }

    assert_system_init();
    memory_system_init();

    if ( main_find_game_path() ) packfile_load();

    read_data_excel_file( &string_excel_data );
    debug_log_start();
    save_memory_allocate();
    sub_42ECA0();
    config_load();

    bool started = false;
    if ( game_globals.server_mode )
    {
        game_globals.unknown_2070 = 0;
        if ( SDL_Init( SDL_INIT_VIDEO ) == 0 && sub_412AC0() )
        {
            if ( !sub_412E60() )
            {
                safestring_create_from_id( &temp_string, 78 );
                MessageBoxW( NULL, temp_string.text, L"Fatal Error", 0 );
            }
            else
            {
                sub_412C50();
                if ( gdi_init() )
                {
                    if ( app_window ) SDL_ShowWindow( app_window );
                    set_app_mode( appmode_server );
                    started = true;
                }
            }
        }
    }
    else
    {
        started = sub_5817D0();
    }

    if ( !started )
    {
        debug_log_save( true );
        free_data_excel_file( &string_excel_data );
        packfile_close();
        memory_system_close();
        return 1;
    }

    bool quit = false;
    while ( !quit )
    {
        SDL_Event event;
        while ( SDL_PollEvent( &event ) ) quit |= main_process_event( &event );

        if ( app_modes[ app_mode ].update ) app_modes[ app_mode ].update();

        if ( next_app_mode != -1 ) set_app_mode( next_app_mode );

        quit |= ( app_mode == appmode_exit );
    }

    debug_log_save( true );

    if ( !game_globals.server_mode )
    {
        controller_shutdown();
        cursor_show();
        fmod_sound_engine_close();
        gge_shutdown();
    }

    SDL_DestroyWindow( app_window );
    app_window = NULL;
    SDL_Quit();

    save_memory_free();

    free_data_excel_file( &string_excel_data );
    packfile_close();
    memory_system_close();

    return 0;
}
