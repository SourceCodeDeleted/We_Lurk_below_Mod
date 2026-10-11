// mod_coop_spawn.c - in a two-player game both players start next to the Homestone (hand-written, NOT part of
// the original game).
//
// To remove: configure with -DILB_MOD_COOP_SPAWN=OFF (and delete this file if wanted). The only references
// to it are guarded calls at the start of the loading screen's last step and of process_npcs, placed by
// tools/draft_fix.py (ENTRY_HOOKS, shared with the other modules).
//
// The original puts every player where that character's save says it last stood. For the second player
// that position comes from a different world, so it is anywhere - often inside the ground - and far from
// player 1. With this module, when a game with two or more players has finished loading and the world has a
// Homestone, every player is moved to it, one a little to the left and one a little to the right. Without a
// Homestone they are put at the world's start position on the surface (where a new character begins).
// Single-player games are not touched.
//
// In a network game only the player who joined is moved: the host was in the middle of playing when the
// other arrived and stays where it was.
#ifdef ILB_MOD_COOP_SPAWN
#include "ILB_draft_decls.h"

typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );
int snprintf( char *, unsigned int, const char *, ... );
char *getenv( const char * );

#define UNIT_ARRAY          ( *(char **)ILB_ADDR( 0x3A73C9C ) )
#define UNIT_SIZE           8936
#define MAX_UNITS           0x4000
#define UNIT_TYPE_OBJECT    4
#define OBJECT_HOMESTONE    34                                  // what player_attempt_recall looks for nearby
#define CLIENTS             ( (int *)ILB_ADDR( 0x60F2C0 ) )     // 12 dwords each, the first is the player's unit id
#define MAX_CLIENTS         4
#define NUM_CLIENTS         ( *(int *)ILB_ADDR( 0x6113E8 ) )

int ILB_input_net_session( void );
static int loading_seen;

// Called on every step of the loading screen.
void ILB_mod_coop_spawn_loading( void )
{
    loading_seen = 1;
}

static struct unit *unit_at( int index )
{
    struct unit *unit = (struct unit *)( UNIT_ARRAY + index * UNIT_SIZE );
    return UNIT_ARRAY && unit->id != (dword)-1 && ( unit->id & 0x3FFF ) == (dword)index ? unit : 0;
}

// Called at the start of process_npcs (every step of play); acts once, on the first step after loading.
void ILB_mod_coop_spawn( void )
{
    struct unit *homestone = 0;
    vector2d start;
    int placed = 0;
    if ( !loading_seen )
        return;
    loading_seen = 0;
    if ( NUM_CLIENTS < 2 )
        return;
    if ( ILB_input_net_session() && getenv( "ILB_NET_JOINER_NAME" ) )
    {
        // on both copies, on the same step: the game's own message line (as its "Day N" notice is made)
        char text[ 100 ];
        int n = 0;
        snprintf( text, sizeof text, "%s has joined the game", getenv( "ILB_NET_JOINER_NAME" ) );
        for ( ; text[ n ] && n < 99; n++ ) stru_60B960.text[ n ] = (unsigned char)text[ n ];
        stru_60B960.text[ n ] = 0;
        stru_60B960.length = n;
        sub_5610F0( -256, 0 );
    }
    for ( int i = 0; i < MAX_UNITS && homestone == 0; i++ )
    {
        struct unit *unit = unit_at( i );
        if ( unit && unit->unit_type == UNIT_TYPE_OBJECT && unit->unit_class == OBJECT_HOMESTONE && unit->world_id == 0 )
            homestone = unit;
    }
    if ( homestone == 0 )
        dungeon_get_start( &start.x );  // no Homestone: where the game puts a new character, on the surface
    else
        start = homestone->position;
    for ( int client = ILB_input_net_session() ? 1 : 0; client < MAX_CLIENTS && client < NUM_CLIENTS; client++ )
    {
        dword id = (dword)CLIENTS[ 12 * client ];
        struct unit *player = id != (dword)-1 ? unit_at( id & 0x3FFF ) : 0;
        if ( player == 0 || player->id != id || player->unit_type != 0 || player->world_id != 0 )
            continue;
        // the game moves a unit by the distance waiting in unwrapped_position (dungeon_move_unit)
        player->unwrapped_position.x = start.x + ( ( placed + client ) % 2 ? 2.5f : -2.5f ) * ( 1 + placed / 2 ) - player->position.x;
        player->unwrapped_position.y = start.y - player->position.y;
        dungeon_move_unit( player );
        placed++;
    }
    fprintf( stderr, "mod coop spawn: %d players placed at %s (%.0f, %.0f)\n", placed, homestone ? "the Homestone" : "the world's start on the surface", start.x, start.y );
}
#else
void ILB_mod_coop_spawn( void ) {}       // the shared hooks call these whether or not the module is in
void ILB_mod_coop_spawn_loading( void ) {}
#endif
