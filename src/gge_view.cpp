//-----------------------------------------------------------------------------
// gge_view.cpp  (placeholder: one function)
// Reconstructed from ILB.exe: gge_view_set_ports 0x4126C0, which the graphics module table calls at
// start-up. Its owning file is a guess (it sits between gge_textures and gge_view in the binary);
// its name and the variable names are chosen here.
//
// Not reconstructed yet: gge_view_to_world and the rest of the view code.

#include "main.h"
#include "gge_view.h"
#include "pending.h"

view_port_data window_port;     // 0x651EB8
view_port_data view_port;       // 0x651EC8

//-----------------------------------------------------------------------------
// with two views side by side each view gets half the window width

void gge_view_set_ports()
{
    int width = ( game_globals.unknown_2044 == 2 ) ? window_width / 2 : window_width;

    view_port.x = 0.0f;
    view_port.y = 0.0f;
    window_port.x = 0.0f;
    window_port.y = 0.0f;
    view_port.width = (float)width;
    view_port.height = (float)window_height;
    window_port.width = (float)window_width;
    window_port.height = (float)window_height;
}
