//-----------------------------------------------------------------------------
// main_app_modes.h
//
// Not an original file name: the app mode declarations belong to main.h, but are kept apart until
// main.h's real contents are known.
//
// The program is a set of app modes. Each has start / end / update functions and input handlers;
// main() runs the current mode's update once per loop and switches mode when one is requested.
//
// Original names: appmode_init, num_app_modes, set_app_mode (assert in main.cpp line 498) and the
// appmode_game_* / appmode_news_init / appmode_intro_init / appmode_char_create_* functions.
// The other mode names are chosen from the source file each mode's functions live in.

#ifndef _MAIN_APP_MODES_H_
#define _MAIN_APP_MODES_H_

enum
{
    appmode_init = 0,           // main.cpp: allocates world, unit and level-generator memory
    appmode_splash,             // ec_splash.cpp
    appmode_main_menu,          // g_main_menu.cpp
    appmode_news,               // g_news.cpp
    appmode_options_menu,       // g_options_menu.cpp
    appmode_community,          // g_community.cpp
    appmode_char_select,        // g_char_select.cpp
    appmode_char_create,        // g_char_create.cpp
    appmode_multi_select,       // g_multi_select.cpp
    appmode_intro,              // g_intro.cpp
    appmode_loading,            // g_loading.cpp (by position in the binary)
    appmode_game,               // g_game.cpp
    appmode_exit,               // main.cpp: frees everything; main() leaves its loop in this mode
    appmode_account,            // g_account.cpp (by position in the binary)
    appmode_14,                 // functions at 0x495D10 - 0x495DB0, file not identified
    appmode_server,             // gdi_main.cpp / s_cmd.cpp: text console, started with -s
    appmode_unused,

    num_app_modes
};

struct app_mode_functions       // 48 bytes; table at 0x609910
{
    void ( * start )();
    void ( * end )();
    void ( * update )();
    // input handlers, called from the event dispatcher (0x581280); their parameters are not recovered yet
    void * mousemove;
    void * lbutton;
    void * rbutton;
    void * mousewheel;
    void * keydown;
    void * keyup;
    void * character;
    void * controller_axis;
    void * controller_button;
};

extern app_mode_functions app_modes[ num_app_modes ];
extern int app_mode;            // 0x3A73CC0
extern int next_app_mode;       // 0x609908, -1 when no change is requested

void set_app_mode( int mode );

#endif
