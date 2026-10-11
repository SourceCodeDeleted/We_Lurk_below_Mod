// fix_stuck_player.c - a player who loads inside solid ground is lifted out (hand-written, NOT part of the
// original game).
//
// To remove: configure with -DILB_FIX_STUCK_PLAYER=OFF (and delete this file if wanted). The only reference to
// it is one guarded call at the start of dungeon_place_unit, placed by tools/draft_fix.py (ENTRY_HOOKS).
//
// A character's save holds the position it was last at. When that spot is solid in the world being loaded
// (seen with a character that had last been played as the second player in someone else's world), the
// original only raises a debug assert and leaves the player buried, unable to see or move. Here the player
// is moved straight up to the first place with room to stand; if there is none within reach, to the world's
// start position.
#ifdef ILB_FIX_STUCK_PLAYER
#include "ILB_draft_decls.h"

typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );

#define TILES               ( *(char **)ILB_ADDR( 0x38854F4 ) )     // 100 bytes per tile, 512 wide, per world
#define WORLD_SIZE          99942404
#define WORLD_WIDTH         512
#define WORLD_HEIGHT        1952
#define ROOM_TO_STAND       3                                       // tiles; a player is about 2.85 tall

static int solid( unsigned int world, int x, int y )                // the test dungeon_place_unit itself makes
{
    return *(int *)( TILES + WORLD_SIZE * world + 100 * ( ( x & ( WORLD_WIDTH - 1 ) ) + ( y << 9 ) ) ) < 0;
}

// Called at the start of dungeon_place_unit, for every unit being put into the world.
void ILB_fix_stuck_player( struct unit *unit )
{
    int x, y;
    if ( unit == 0 || unit->unit_type != 0 || unit->world_id >= 3 || TILES == 0 )
        return;
    x = (int)unit->position.x;
    y = (int)unit->position.y;
    if ( unit->position.x < 0 || x >= WORLD_WIDTH || y < ROOM_TO_STAND || y >= WORLD_HEIGHT || !solid( unit->world_id, x, y ) )
        return;
    for ( int up = y - 1; up >= ROOM_TO_STAND && up > y - 400; up-- )
    {
        int clear = 1;
        for ( int k = 0; k < ROOM_TO_STAND; k++ )
            clear &= !solid( unit->world_id, x, up - k );
        if ( clear )
        {
            fprintf( stderr, "fix stuck player: was inside solid ground at %d,%d; moved up to %d,%d\n", x, y, x, up );
            unit->position.y = (float)up + 0.9999f;         // standing on the tile below, as the game places feet
            return;
        }
    }
    dungeon_get_start( &unit->position.x );
    fprintf( stderr, "fix stuck player: was inside solid ground at %d,%d; moved to the world's start\n", x, y );
}
#endif
