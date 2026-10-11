// mod_local_coop.c - a way into the game's own Local Multiplayer (hand-written, NOT part of the original).
//
// To remove: configure with -DILB_MOD_LOCAL_COOP=OFF (and delete this file if wanted). The only reference
// to it is one guarded call in draft/native/native_platform.c (key events).
//
// This build contains the complete two-player set-up screen ("Local Multiplayer": Player 1 / Player 2,
// Keyboard/Mouse or Controller, "Proceed to Character Select/Create") and everything behind it - two
// characters in one world, split screen - but nothing in its menus leads there: no code asks for that
// screen (app mode 8). This module adds the missing door and nothing else:
//
//   F5 on the title menu    open the Local Multiplayer set-up screen
//
// One player uses keyboard and mouse, the other a controller (the screen swaps which is which).
#ifdef ILB_MOD_LOCAL_COOP
#include "ILB_draft_decls.h"

#define APP_MODE                ( *(int *)ILB_ADDR( 0x3A73CC0 ) )
#define APP_MODE_WANTED         ( *(int *)ILB_ADDR( 0x609908 ) )    // what the menu buttons set (mm_do_play: 6)
#define APP_MODE_TITLE          2
#define APP_MODE_MULTI_SELECT   8
#define KEY_F5                  0x4000003E

// Called for every key press and release, before the game sees it. Returns 1 if the key was taken.
int ILB_mod_local_coop_key( int key, int pressed )
{
    if ( APP_MODE != APP_MODE_TITLE || key != KEY_F5 )
        return 0;
    if ( pressed )
        APP_MODE_WANTED = APP_MODE_MULTI_SELECT;
    return 1;
}
#endif
