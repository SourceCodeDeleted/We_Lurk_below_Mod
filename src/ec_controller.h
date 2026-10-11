//-----------------------------------------------------------------------------
// ec_controller.h
//
// Game controller input through SDL. One physical pad (main_gamepad) is opened; its state is
// kept per controller id (up to four), taken from the current client.
//
// Original names: the functions listed in ec_controller.cpp's header, and from assert text
// main_gamepad, controller_is_connected, max_num_controllers, num_controller_buttons, my_button,
// sdl_button, sdl_code, direction_percent, menu_id. Everything else here is chosen, and the
// meaning given to individual buttons and states is read from how the code uses them.

#ifndef _EC_CONTROLLER_H_
#define _EC_CONTROLLER_H_

#include "ec_vector.h"

#define max_num_controllers         4
#define num_controller_buttons      19

#define controller_dead_zone        8000
#define controller_full_zone        30000

// which glyph set to draw button prompts with (0x5D2C7C)
enum
{
    controller_type_xbox = 0,
    controller_type_playstation,    // nothing in this build selects it; the glyph tables still handle it
    controller_type_none,
};

// button and stick-direction states
enum
{
    button_state_up = 0,
    button_state_pressed,           // went down this update
    button_state_held,
    button_state_released,          // went up this update
};

// "my_button" numbering. 0-10 and 15-18 come from SDL buttons (controller_sdl_to_button); 11-14 are
// the left stick treated as four buttons for menus.
enum
{
    my_button_stick_up = 11,
    my_button_stick_down,
    my_button_stick_left,
    my_button_stick_right,
};

struct controller_data      // 108 bytes; four at 0x651AD8
{
    int         axis[ 6 ];          // left x, left y, right x, right y (dead zone removed, / 1500), triggers (0 or 1)
    int         left_mode;          // set by controller_set_left_mode; 3 after a reset
    int         unknown_1C;
    int         button[ num_controller_buttons ];
};

extern int controller_type;

void controller_reset();
void controller_set_gamepad();
void controller_start();
void controller_shutdown();
bool controller_is_active();
int controller_handle_axis_input( int axis, int value );
int controller_sdl_to_button( int sdl_button );
dword handle_my_button_input( dword id, bool down, int my_button );
void controller_get_ui_left_stick_direction();
int controller_get_adjusted_left_stick( vector2d * direction_percent );
bool controller_get_adjusted_right_stick( vector2d * direction_percent );
void controller_set_left_mode( int mode );
dword get_xbox_smallfont_char( dword sdl_code );
dword get_ps_smallfont_char( dword sdl_code );
dword get_xbox_special_char( dword sdl_code );
dword get_ps_special_char( dword sdl_code );

#endif
