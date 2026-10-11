// Native test of src/ec_controller.cpp and src/ec_cursor.cpp with a scripted SDL pad.
#include "main.h"
#include "ec_config.h"
#include "ec_controller.h"
#include "ec_cursor.h"
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

    // no pad attached
    controller_start();
    CHECK( controller_type == controller_type_none && !controller_is_active() );
    CHECK( gamepad_confirm_string.length == 1 && gamepad_confirm_string.text[ 0 ] == 133 && gamepad_cancel_string.text[ 0 ] == 134 );

    // a pad appears
    fake_pads = 1;
    controller_start();
    CHECK( controller_type == controller_type_xbox );
    config.gamepad = false;
    CHECK( !controller_is_active() );           // no client: follows the gamepad setting
    config.gamepad = true;
    CHECK( controller_is_active() );

    client_data client = {};
    client.client_id = 1;
    client.controller_id = 2;
    game_globals.client = &client;
    game_globals.unknown_2038[ 0 ] = 2;
    CHECK( controller_is_active() );
    client.input_type = 1;
    CHECK( !controller_is_active() );           // this client is on the keyboard
    client.input_type = 0;

    // button state machine: up -> pressed -> held -> released -> up
    CHECK( handle_my_button_input( 2, true, 3 ) == button_state_pressed );
    CHECK( handle_my_button_input( 2, true, 3 ) == button_state_held );
    CHECK( handle_my_button_input( 2, false, 3 ) == button_state_released );
    CHECK( handle_my_button_input( 2, false, 3 ) == button_state_up );
    CHECK( handle_my_button_input( 2, true, 19 ) == 0 );     // asserts: no such button
    CHECK( handle_my_button_input( 4, true, 0 ) == 0 );      // asserts: no such controller

    // SDL button numbers to the game's numbering
    CHECK( controller_sdl_to_button( 0 ) == 0 && controller_sdl_to_button( 6 ) == 6 );
    CHECK( controller_sdl_to_button( 7 ) == 18 && controller_sdl_to_button( 10 ) == 15 && controller_sdl_to_button( 11 ) == 7 && controller_sdl_to_button( 14 ) == 10 );
    CHECK( controller_sdl_to_button( 99 ) == 0 );

    // axis events: dead zone, then / 1500; triggers are on or off
    CHECK( controller_handle_axis_input( 0, 5000 ) == 5000 );
    controller_get_ui_left_stick_direction();
    CHECK( handle_my_button_input( 2, false, my_button_stick_right ) == button_state_up );   // stayed up inside the dead zone
    controller_handle_axis_input( 0, 23000 );                // ( 23000 - 8000 ) / 1500 = 10 -> right
    controller_handle_axis_input( 1, -9500 );                // ( -9500 + 8000 ) / 1500 = -1 -> up
    controller_get_ui_left_stick_direction();
    controller_get_ui_left_stick_direction();
    CHECK( handle_my_button_input( 2, true, my_button_stick_right ) == button_state_held );
    CHECK( handle_my_button_input( 2, true, my_button_stick_up ) == button_state_held );
    CHECK( handle_my_button_input( 2, false, my_button_stick_left ) == button_state_up );
    CHECK( controller_handle_axis_input( 4, 31000 ) == 1 && controller_handle_axis_input( 5, 100 ) == 0 );
    CHECK( controller_handle_axis_input( 9, 100 ) == 0 );

    // polled sticks as -1..1
    vector2d stick;
    fake_axis[ 0 ] = 4000; fake_axis[ 1 ] = -4000;
    CHECK( controller_get_adjusted_left_stick( &stick ) == 0 && stick.x == 0.0f && stick.y == 0.0f );
    fake_axis[ 0 ] = 8000 + 12384;
    CHECK( controller_get_adjusted_left_stick( &stick ) == 1 && stick.x == 0.5f );
    fake_axis[ 1 ] = -32768;
    CHECK( controller_get_adjusted_left_stick( &stick ) == 2 && stick.y == -1.0f );
    CHECK( controller_get_adjusted_left_stick( NULL ) == -1 );
    CHECK( !controller_get_adjusted_right_stick( &stick ) );
    fake_axis[ 3 ] = 32767;
    CHECK( controller_get_adjusted_right_stick( &stick ) && stick.x == 0.0f && stick.y > 0.99f );

    // button prompt glyphs
    CHECK( get_ps_smallfont_char( 0 ) == 153 && get_ps_smallfont_char( 14 ) == 166 && get_ps_smallfont_char( 6 ) == invalid_id );
    CHECK( get_ps_special_char( 0 ) == 133 && get_ps_special_char( 8 ) == 161 && get_ps_special_char( 4 ) == invalid_id );
    controller_type = controller_type_playstation;
    CHECK( get_ps_smallfont_char( 0 ) == 169 && get_ps_smallfont_char( 11 ) == invalid_id );
    CHECK( get_ps_special_char( 3 ) == 140 && get_ps_special_char( 6 ) == invalid_id );

    // cursor: scaled, clamped, per client
    int x, y;
    cursor_set_position( 100, 200 );
    cursor_get_position( &x, &y, NULL );
    CHECK( x == 100 && y == 200 );
    cursor_set_position( 5000, -3 );
    cursor_get_position( &x, &y, NULL );
    CHECK( x == 1919 && y == 0 );
    game_globals.unknown_2044 = 2;                           // two views: half the width
    cursor_set_position( 1500, 10 );
    cursor_get_position( &x, &y, NULL );
    CHECK( x == 959 && y == 10 );
    game_globals.client = NULL;                              // client 0 has its own position
    cursor_get_position( &x, &y, NULL );
    CHECK( x == 0 && y == 0 );

    controller_shutdown();
    CHECK( fake_pad_closed == 1 && !controller_is_active() && controller_type == controller_type_none );

    printf( failures ? "%d check(s) FAILED\n" : "controller: all checks passed\n", failures );
    return failures != 0;
}
