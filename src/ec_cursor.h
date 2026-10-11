//-----------------------------------------------------------------------------
// ec_cursor.h
//
// The game draws its own cursor; each client has a cursor position in window pixels. The system
// cursor is shown only while the pointer is outside the game's area.
//
// cursor_get_position and cursor_set_position are original names; cursor_show (the block inlined
// at the end of main) and the variable names are chosen here.

#ifndef _EC_CURSOR_H_
#define _EC_CURSOR_H_

void cursor_get_position( int * x, int * y, bool * in_window );
void cursor_set_position( int x, int y );
void cursor_show();

#endif
