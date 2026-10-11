//-----------------------------------------------------------------------------
// ec_cursor.cpp
// Reconstructed from ILB.exe: cursor_set_position 0x403D70; cursor_get_position from its inlined
// copies (36 of them); cursor_show from the end of main.
//
// cursor_get_position's third parameter is never used by the copies examined so far; treating it
// as "is the pointer inside the game area" is an assumption.
//
// Not reconstructed yet: 0x403F00, which formats "cursor: ( x, y )  World: ( x, y )" into
// temp_string for the debug display (it needs gge_view_to_world).

#include "main.h"
#include "ec_cursor.h"
#include "ec_vector.h"
#include "g_game_globals.h"
#include "pending.h"
#include <SDL.h>
#include <math.h>

static ivector2d cursor_position[ max_clients ];    // 0x651C88
static bool cursor_in_window = false;               // 0x16B4D98
bool system_cursor_visible = false;                 // 0x651AD3

//-----------------------------------------------------------------------------

void cursor_get_position( int * x, int * y, bool * in_window )
{
    dword index = 0;
    if ( game_globals.client ) index = game_globals.client->client_id;
    if ( !( index < max_clients ) )
    {
        assert_exp( index < max_clients );
        index = 0;
    }

    *x = cursor_position[ index ].x;
    *y = cursor_position[ index ].y;
    if ( in_window ) *in_window = cursor_in_window;
}

//-----------------------------------------------------------------------------
// x and y are in the units SDL reports; they are scaled to window pixels and clamped.

void cursor_set_position( int x, int y )
{
    dword index = 0;
    if ( game_globals.client ) index = game_globals.client->client_id;
    assert_return( index < max_clients );

    int height = window_height;
    int width = ( game_globals.unknown_2044 == 2 ) ? window_width / 2 : window_width;

    int pixel_x = (int)floor( (float)x * window_scale_x + 0.5f );
    int pixel_y = (int)floor( (float)y * window_scale_y + 0.5f );

    cursor_position[ index ].x = ( pixel_x < 0 ) ? 0 : ( pixel_x < width ) ? pixel_x : width - 1;
    cursor_position[ index ].y = ( pixel_y < 0 ) ? 0 : ( pixel_y < height ) ? pixel_y : height - 1;

    if ( pixel_x < 0 || pixel_x >= width || pixel_y < 0 || pixel_y >= height )
    {
        cursor_in_window = false;
        if ( !system_cursor_visible )
        {
            SDL_ShowCursor( 1 );
            system_cursor_visible = true;
        }
    }
    else
    {
        cursor_in_window = true;
        if ( system_cursor_visible )
        {
            SDL_ShowCursor( 0 );
            system_cursor_visible = false;
        }
    }
}

//-----------------------------------------------------------------------------

void cursor_show()
{
    if ( !system_cursor_visible )
    {
        SDL_ShowCursor( 1 );
        system_cursor_visible = true;
    }
}
