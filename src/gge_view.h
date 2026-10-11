//-----------------------------------------------------------------------------
// gge_view.h
//
// Placeholder for the view code's header: only the per-view matrices the material code reads.
// The table is at 0x651EF0, 152 bytes per view. Which matrix is which is a guess from how the
// shaders are fed (the first is skipped for screen-space drawing, which points to it being the
// camera's view matrix); all names here are chosen.

#ifndef _GGE_VIEW_H_
#define _GGE_VIEW_H_

#include "ec_matrix.h"
#include "g_game_globals.h"

struct view_data            // 152 bytes
{
    matrix      view_matrix;            //   0
    matrix      projection_matrix;      //  64
    byte        unknown_128[ 24 ];
};

extern view_data views[ max_clients ];
extern matrix screen_matrix;            // 0x3855AC8: used in place of view_matrix for screen-space drawing

// 0x651EB8: the whole window, and 0x651EC8: one view's part of it (view_port, declared in
// ec_matrix.h because matrix_unproject reads it). Names chosen.
extern view_port_data window_port;

void gge_view_set_ports();              // 0x4126C0, name chosen

#endif
