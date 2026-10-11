//-----------------------------------------------------------------------------
// ec_controller.cpp
// Reconstructed from ILB.exe: controller_reset 0x402FC0, controller_set_gamepad 0x403160,
// controller_start 0x403200, controller_is_active 0x403270, controller_handle_axis_input 0x403300,
// controller_sdl_to_button 0x4034E0, handle_my_button_input 0x4035B0,
// controller_get_ui_left_stick_direction 0x403680, controller_get_adjusted_left_stick 0x4037E0,
// controller_get_adjusted_right_stick 0x403960, controller_set_left_mode 0x403AB0,
// get_ps_smallfont_char 0x403B10, get_ps_special_char 0x403C60 (the xbox versions are inlined in
// them). controller_shutdown is the block inlined at the end of main.
//
// controller_reset, controller_start and controller_shutdown are names chosen here.
//
// Guesses, to be confirmed when the code that uses them is reconstructed:
//   - client_data.input_type == 1 means "this client uses the keyboard"
//   - game_globals.unknown_2038 == 1 means a single local player
//   - the two glyph strings set in controller_start are the confirm / cancel button prompts

#include "main.h"
#include "ec_config.h"
#include "ec_controller.h"
#include "g_game_globals.h"
#include "pending.h"
#include <SDL.h>

int controller_type = controller_type_none;                         // 0x5D2C7C

static SDL_GameController * main_gamepad = NULL;                    // 0x16B4994
static char controller_name_line[ 1024 ];                           // 0x16B4998
static bool controller_is_connected = false;                        // 0x651AD2
static controller_data controllers[ max_num_controllers ];          // 0x651AD8

//-----------------------------------------------------------------------------

static int button_state_step( int state, bool down )
{
    if ( down ) return ( state == button_state_up || state == button_state_released ) ? button_state_pressed : button_state_held;
    return ( state == button_state_pressed || state == button_state_held ) ? button_state_released : button_state_up;
}

//-----------------------------------------------------------------------------
// Logs which pad is attached, then clears the state of all four controllers.

void controller_reset()
{
    controller_type = controller_type_none;

    if ( main_gamepad )
    {
        debug_log_format_line( L"Controller Vendor = %i\n", SDL_GameControllerGetVendor( main_gamepad ) );
        sprintf( controller_name_line, "Controller name: %s\n", SDL_GameControllerName( main_gamepad ) );
        debug_log_add_char_str( controller_name_line );

        controller_type = controller_type_xbox;
        if ( game_globals.unknown_2074 == 2 ) SDL_SetHint( "SDL_WINRT_HANDLE_BACK_BUTTON", "true" );
    }

    for ( int i = 0; i < max_num_controllers; i++ )
    {
        memset( &controllers[ i ], 0, sizeof( controller_data ) );
        controllers[ i ].left_mode = 3;
    }
}

//-----------------------------------------------------------------------------
// opens the first joystick SDL recognises as a game controller

void controller_set_gamepad()
{
    if ( main_gamepad != NULL ) return;

    assert_return( controller_is_connected == false );
    controller_is_connected = false;

    int num_joysticks = SDL_NumJoysticks();
    for ( int i = 0; i < num_joysticks; i++ )
    {
        if ( main_gamepad == NULL && SDL_IsGameController( i ) )
        {
            main_gamepad = SDL_GameControllerOpen( i );
            SDL_JoystickInstanceID( SDL_GameControllerGetJoystick( main_gamepad ) );
            controller_is_connected = true;
            return;
        }
    }
}

//-----------------------------------------------------------------------------

void controller_start()
{
    controller_set_gamepad();
    controller_reset();

    // one-character strings holding a button glyph: 133 / 134 for xbox, 137 / 138 for playstation
    gamepad_confirm_string.length = 1;
    gamepad_confirm_string.text[ 0 ] = (wchar_t)( 4 * ( controller_type == controller_type_playstation ) + 133 );
    gamepad_confirm_string.text[ 1 ] = 0;
    gamepad_cancel_string.length = 1;
    gamepad_cancel_string.text[ 0 ] = (wchar_t)( 4 * ( controller_type == controller_type_playstation ) + 134 );
    gamepad_cancel_string.text[ 1 ] = 0;
}

//-----------------------------------------------------------------------------

void controller_shutdown()
{
    if ( main_gamepad )
    {
        SDL_GameControllerClose( main_gamepad );
        main_gamepad = NULL;
        controller_is_connected = false;
        controller_reset();
    }
}

//-----------------------------------------------------------------------------
// Should input be read from the pad right now?

bool controller_is_active()
{
    if ( main_gamepad == NULL || !controller_is_connected ) return false;

    if ( game_globals.client )
    {
        if ( game_globals.client->input_type == 1 ) return false;
        if ( game_globals.unknown_2038[ 0 ] == 1 ) return config.gamepad;
        return true;
    }

    // in the menus with more than one player: the player whose menu it is decides
    if ( (dword)game_globals.unknown_2038[ 2 ] > 1 )
    {
        dword menu_id = game_globals.unknown_2038[ 1 ] - 1;
        assert_return_false( menu_id < max_clients );
        return menu_slots[ menu_id ].input_device == 2;
    }

    return config.gamepad;
}

//-----------------------------------------------------------------------------
// Stores one SDL axis event. Sticks: dead zone removed, then divided by 1500 (about -16..16).
// Triggers: 1 when pulled past 30000.

int controller_handle_axis_input( int axis, int value )
{
    dword id = 0;
    if ( game_globals.client )
    {
        id = game_globals.client->controller_id;
        assert_return_zero( id != invalid_id );
        assert_return_zero( id < max_num_controllers );
    }

    switch ( axis )
    {
    case 0:
    case 1:
    case 2:
    case 3:
        controllers[ id ].axis[ axis ] = 0;
        if ( value < -controller_dead_zone ) controllers[ id ].axis[ axis ] = ( value + controller_dead_zone ) / 1500;
        else if ( value > controller_dead_zone ) controllers[ id ].axis[ axis ] = ( value - controller_dead_zone ) / 1500;
        return value;

    case 4:
    case 5:
        controllers[ id ].axis[ axis ] = ( value > controller_full_zone );
        return controllers[ id ].axis[ axis ];
    }

    return 0;
}

//-----------------------------------------------------------------------------

int controller_sdl_to_button( int sdl_button )
{
    switch ( sdl_button )
    {
    case 0:  return 0;
    case 1:  return 1;
    case 2:  return 2;
    case 3:  return 3;
    case 4:  return 4;
    case 5:  return 5;
    case 6:  return 6;
    case 7:  return 18;
    case 8:  return 17;
    case 9:  return 16;
    case 10: return 15;
    case 11: return 7;
    case 12: return 8;
    case 13: return 9;
    case 14: return 10;
    }

    assert_exp( sdl_button == -1 );
    return 0;
}

//-----------------------------------------------------------------------------
// advances one button's state and returns the new state

dword handle_my_button_input( dword id, bool down, int my_button )
{
    assert_return_zero( id != invalid_id );
    assert_return_zero( id < max_num_controllers );
    assert_return_zero( my_button >= 0 );
    assert_return_zero( my_button < num_controller_buttons );

    controllers[ id ].button[ my_button ] = button_state_step( controllers[ id ].button[ my_button ], down );
    return controllers[ id ].button[ my_button ];
}

//-----------------------------------------------------------------------------
// turns the left stick into four menu "buttons"

void controller_get_ui_left_stick_direction()
{
    dword id = 0;
    if ( game_globals.client )
    {
        id = game_globals.client->controller_id;
        assert_return( id != invalid_id );
        assert_return( id < max_num_controllers );
    }

    controller_data * c = &controllers[ id ];
    c->button[ my_button_stick_up ] = button_state_step( c->button[ my_button_stick_up ], c->axis[ 1 ] < 0 );
    c->button[ my_button_stick_down ] = button_state_step( c->button[ my_button_stick_down ], c->axis[ 1 ] > 0 );
    c->button[ my_button_stick_left ] = button_state_step( c->button[ my_button_stick_left ], c->axis[ 0 ] < 0 );
    c->button[ my_button_stick_right ] = button_state_step( c->button[ my_button_stick_right ], c->axis[ 0 ] > 0 );
}

//-----------------------------------------------------------------------------

static int remove_dead_zone( int value )
{
    if ( value < -controller_dead_zone ) return value + controller_dead_zone;
    if ( value > controller_dead_zone ) return value - controller_dead_zone;
    return 0;
}

// Left stick as -1..1 on each axis. Returns 0 when centred, 1 when moved, 2 when pushed all the
// way on either axis, and -1 on error.

int controller_get_adjusted_left_stick( vector2d * direction_percent )
{
    if ( !direction_percent ) { assert_exp( direction_percent ); return -1; }

    direction_percent->x = 0.0f;
    direction_percent->y = 0.0f;

    if ( game_globals.client )
    {
        dword id = game_globals.client->controller_id;
        if ( !( id != invalid_id ) ) { assert_exp( id != invalid_id ); return -1; }
        if ( !( id < max_num_controllers ) ) { assert_exp( id < max_num_controllers ); return -1; }
    }
    if ( !( main_gamepad != NULL ) ) { assert_exp( main_gamepad != NULL ); return -1; }

    int x = SDL_GameControllerGetAxis( main_gamepad, SDL_CONTROLLER_AXIS_LEFTX );
    int y = SDL_GameControllerGetAxis( main_gamepad, SDL_CONTROLLER_AXIS_LEFTY );

    int result = 0;
    if ( x < -controller_dead_zone || x > controller_dead_zone || y < -controller_dead_zone || y > controller_dead_zone ) result = 1;
    if ( x < -controller_full_zone || x > controller_full_zone || y < -controller_full_zone || y > controller_full_zone ) result = 2;

    direction_percent->x = (float)remove_dead_zone( x ) / 24768.0f;
    direction_percent->y = (float)remove_dead_zone( y ) / 24768.0f;
    return result;
}

//-----------------------------------------------------------------------------

bool controller_get_adjusted_right_stick( vector2d * direction_percent )
{
    assert_return_false( direction_percent );

    direction_percent->x = 0.0f;
    direction_percent->y = 0.0f;

    if ( game_globals.client )
    {
        dword id = game_globals.client->controller_id;
        assert_return_false( id != invalid_id );
        assert_return_false( id < max_num_controllers );
    }
    assert_return_false( main_gamepad != NULL );

    int x = SDL_GameControllerGetAxis( main_gamepad, SDL_CONTROLLER_AXIS_RIGHTX );
    int y = SDL_GameControllerGetAxis( main_gamepad, SDL_CONTROLLER_AXIS_RIGHTY );

    if ( remove_dead_zone( x ) == 0 && remove_dead_zone( y ) == 0 ) return false;

    direction_percent->x = (float)remove_dead_zone( x ) / 24768.0f;
    direction_percent->y = (float)remove_dead_zone( y ) / 24768.0f;
    return true;
}

//-----------------------------------------------------------------------------

void controller_set_left_mode( int mode )
{
    dword id = 0;
    if ( game_globals.client )
    {
        id = game_globals.client->controller_id;
        assert_return( id != invalid_id );
        assert_return( id < max_num_controllers );
    }

    controllers[ id ].left_mode = mode;
}

//-----------------------------------------------------------------------------
// Font characters that draw button prompts. sdl_code is an SDL_GameControllerButton; the result
// is a character in the game's font, or invalid_id.

dword get_xbox_smallfont_char( dword sdl_code )
{
    switch ( sdl_code )
    {
    case 0:  return 153;
    case 1:  return 154;
    case 2:  return 155;
    case 3:  return 156;
    case 4:  return 159;
    case 5:  return 160;
    case 9:  return 157;
    case 10: return 158;
    case 11: return 165;
    case 12: return 167;
    case 13: return 168;
    case 14: return 166;
    }

    assert_exp( sdl_code == invalid_id );
    return invalid_id;
}

dword get_ps_smallfont_char( dword sdl_code )
{
    if ( controller_type != controller_type_playstation ) return get_xbox_smallfont_char( sdl_code );

    switch ( sdl_code )
    {
    case 0:  return 169;
    case 1:  return 170;
    case 2:  return 171;
    case 3:  return 172;
    case 4:  return 175;
    case 5:  return 176;
    case 9:  return 173;
    case 10: return 174;
    }

    assert_exp( sdl_code == invalid_id );
    return invalid_id;
}

//-----------------------------------------------------------------------------

dword get_xbox_special_char( dword sdl_code )
{
    switch ( sdl_code )
    {
    case 0:  return 133;
    case 1:  return 134;
    case 2:  return 135;
    case 3:  return 136;
    case 6:  return 154;
    case 7:  return 157;
    case 8:  return 161;
    case 9:  return 155;
    case 10: return 156;
    case 11: return 150;
    case 12: return 151;
    }

    assert_exp( sdl_code == invalid_id );
    return invalid_id;
}

dword get_ps_special_char( dword sdl_code )
{
    if ( controller_type != controller_type_playstation ) return get_xbox_special_char( sdl_code );

    switch ( sdl_code )
    {
    case 0:  return 137;
    case 1:  return 138;
    case 2:  return 139;
    case 3:  return 140;
    }

    assert_exp( sdl_code == invalid_id );
    return invalid_id;
}
