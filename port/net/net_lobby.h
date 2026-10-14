// net_lobby.h - the game's side of the lobby protocol (port/net/net_lobby.c; server/src/protocol.rs), as the
// mod and the session code call it. Plain C, no system headers, so the files built without them can include it.
#pragma once

// A game on the server's list. `character` is the host's character as hex (see net_lobby.c), "" if the host
// did not send one (an older copy of the game).
typedef struct { int id, locked, age; char name[ 100 ], host[ 100 ], salt[ 20 ], character[ 481 ]; } lobby_game;

int lobby_connect( const char *address, const char *player, char *error, int error_size );
void lobby_disconnect( void );
void lobby_set_character( const char *hex );        // sent with the next lobby_host; 0 or "" for none
int lobby_host( const char *name, const char *password, char *error, int error_size );
int lobby_wait( char *joiner, int joiner_size );
int lobby_list( lobby_game *games, int max, char *error, int error_size );
int lobby_join( const lobby_game *game, const char *password, char *error, int error_size );
const char *lobby_build_id( void );
