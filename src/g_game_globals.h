//-----------------------------------------------------------------------------
// g_game_globals.h
//
// Placeholder header (the original location of these declarations is not known). game_globals is
// one 8352-byte struct at 0x60F3B0 that main() clears at start-up. Only the fields needed so far
// are laid out; the rest is padding until the code that uses it is reconstructed.
//
// Original names (assert text): game_globals, client, client_id, max_clients, and - not placed
// yet - player_id, game_state, uses_shared_town, menu_total_players, menu_player_count.
// Every unknown_* field and server_mode are names chosen here.

#ifndef _G_GAME_GLOBALS_H_
#define _G_GAME_GLOBALS_H_

#define max_clients     4

struct client_data
{
    dword       unknown_0;
    dword       client_id;          //  4  index into per-client arrays, < max_clients
    dword       input_type;         //  8  name chosen; 1 seems to mean keyboard (the pad is ignored for this client)
    dword       game_state;         // 12  original name; game_state_* values not recovered yet
    dword       unknown_16;
    dword       unknown_20;
    dword       unknown_24;
    byte        unknown_28[ 4 ];
    dword       controller_id;      // 32  name chosen; index into the controller state table, < max_num_controllers
    // more fields follow (player_id, ...)
};

struct game_globals_data            // 0x20A0 bytes
{
    byte            unknown_0000[ 0x14 ];
    client_data *   client;             // 0x0014  the client whose input or drawing is being handled, or NULL
    byte            unknown_0018[ 0x2020 ];
    int             unknown_2038[ 3 ];  // 0x2038  set to 1 at start-up
    int             unknown_2044;       // 0x2044  set to 1 at start-up; 2 = two views side by side
                                        //         (the cursor area and UI scale use half the window width)
    byte            unknown_2048[ 0x20 ];
    bool            server_mode;        // 0x2068  started with -s
    int             unknown_206C;       // 0x206C
    int             unknown_2070;       // 0x2070  cleared before the window is created
    int             unknown_2074;       // 0x2074
    bool            unknown_2078;       // 0x2078  saved, cleared and restored around every settings file access
    byte            unknown_2079[ 0x27 ];   // 0x2079  flags and UI scale factors written by 0x4610D0
};

extern game_globals_data game_globals;

// Table of four 48-byte entries at 0x60F2C0, just before game_globals, one per menu player.
// Name and field names are chosen here.
struct menu_slot_data
{
    int         unknown_0;
    int         index;
    int         input_device;       // 2 = game controller
    int         unknown_C[ 9 ];
};
extern menu_slot_data menu_slots[ max_clients ];

#endif
