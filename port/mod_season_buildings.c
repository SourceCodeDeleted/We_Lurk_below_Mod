// mod_season_buildings.c - town buildings decorated for the in-game season (hand-written, NOT part of the
// original).
//
// To remove: configure with -DILB_MOD_SEASON_BUILDINGS=OFF (and delete this file if wanted). The references
// to it are guarded calls placed by tools/draft_fix.py: at the start of process_npcs and of the loading
// screen's step (ENTRY_HOOKS, shared with the other modules), at the start of
// dungeon_event_respawn_buildings (ENTRY_HOOKS), and two expressions in dungeon_spawn_blueprint and
// objects_update_seasons_gfx (VALUE_HOOKS).
//
// Every release of the game ships building templates decorated for the time of year it came out. This
// build's are the Christmas ones: coloured lights or icicle lights along the roofs, a Christmas tree in the
// tavern, and Jolly Ol' Graybeard standing in the bank - all year round. An autumn release has none of
// that; its furniture sheet has pumpkins painted on instead. With this module:
//
//   Spring, Summer   no holiday decoration
//   Autumn           no Christmas decoration; pumpkins on the furniture (see below)
//   Winter           the Christmas decoration and Jolly Ol' Graybeard, as in the original
//
// How: outside Winter the four Christmas tiles of the templates place nothing, and what an earlier Winter
// (or a save made by the original) left standing is taken away. When Winter comes the game's own routine
// for adding a new release's decoration to existing buildings (dungeon_event_respawn_buildings) puts it
// back; that routine only places what is missing.
//
// The autumn furniture is art, so it comes from the owner's own autumn copy of the game:
// tools/make_season_npcs.py writes mods/objects_halloween.raw (the top 512x256 pixels of the object sheet)
// next to the game's pack file. Without the file Autumn simply has no pumpkins.
#ifdef ILB_MOD_SEASON_BUILDINGS
#include "ILB_draft_decls.h"

typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );
FILE *fopen( const char *, const char * );
unsigned int fread( void *, unsigned int, unsigned int, FILE * );
int fclose( FILE * );

#define SEASON              ( *(int *)ILB_ADDR( 0x3891918 ) )       // 0 spring, 1 summer, 2 autumn, 3 winter
#define SEASON_WINTER       3
#define UNIT_ARRAY          ( *(char **)ILB_ADDR( 0x3A73C9C ) )
#define UNIT_SIZE           8936
#define MAX_UNITS           0x4000
#define UNIT_TYPE_OBJECT    4
#define UNIT_TYPE_NPC       5
#define NPC_SANTA           12      // Jolly Ol' Graybeard, template tile 196
#define OBJECT_XMAS_FIRST   146     // christmas_tree (tile 197), icicle_lights (198), christmas_lights (199)
#define OBJECT_XMAS_LAST    148
#define TILE_XMAS_FIRST     196
#define TILE_XMAS_LAST      199
#define OBJECT_TEXTURE      ( *(unsigned int *)ILB_ADDR( 0x5E057C ) )   // objects_base.gs3 with the season's sheet on top
#define OBJECT_BASE_FILE    ( (char *)ILB_ADDR( 0x5E1D20 ) )
#define OBJECT_AUTUMN_FILE  ( (char *)ILB_ADDR( 0x5E07CC ) )
#define SHEET_WIDTH         512
#define SHEET_HEIGHT        512
#define PATCH_HEIGHT        256

static int loaded = 1;      // a world has been loaded since the last look
static int applied = -1;    // the season the buildings were last made to fit

// Called on every step of the loading screen.
void ILB_mod_season_buildings_loading( void )
{
    loaded = 1;
}

// Called where dungeon_spawn_blueprint reads a template tile: what the tile places in this season.
int ILB_mod_season_building_tile( int tile )
{
    return SEASON != SEASON_WINTER && tile >= TILE_XMAS_FIRST && tile <= TILE_XMAS_LAST ? -1 : tile;
}

// Called at the start of dungeon_event_respawn_buildings: outside Winter there is nothing to add.
int ILB_mod_season_buildings_no_respawn( void )
{
    return SEASON != SEASON_WINTER;
}

// Counts the Christmas decoration in the surface world, and takes it away if asked to.
static int christmas_units( int remove )
{
    int found = 0;
    for ( int i = 0; UNIT_ARRAY != 0 && i < MAX_UNITS; i++ )
    {
        struct unit *unit = (struct unit *)( UNIT_ARRAY + i * UNIT_SIZE );
        if ( unit->id == (dword)-1 || ( unit->id & 0x3FFF ) != (dword)i || unit->world_id != 0 )
            continue;
        if ( ( unit->unit_type == UNIT_TYPE_NPC && unit->unit_class == NPC_SANTA )
          || ( unit->unit_type == UNIT_TYPE_OBJECT && unit->unit_class >= OBJECT_XMAS_FIRST && unit->unit_class <= OBJECT_XMAS_LAST ) )
        {
            if ( remove )
                unit_delete( unit );
            found++;
        }
    }
    return found;
}

// Called at the start of process_npcs (every step of play); acts on the first step after a world was loaded
// and when the season changes.
void ILB_mod_season_buildings( void )
{
    if ( SEASON < 0 || SEASON > 3 || ( !loaded && SEASON == applied ) )
        return;
    loaded = 0;
    applied = SEASON;
    if ( SEASON == SEASON_WINTER )
    {
        int before = christmas_units( 0 );
        dungeon_event_respawn_buildings();      // does nothing while Jolly Ol' Graybeard is in the world
        fprintf( stderr, "mod season buildings: Winter, Christmas decorations %d -> %d\n", before, christmas_units( 0 ) );
    }
    else
        fprintf( stderr, "mod season buildings: season %d, %d Christmas decorations put away\n", SEASON, christmas_units( 1 ) );
}

// Called after objects_update_seasons_gfx has made the object texture for Autumn: the same again, as the
// game's own gge_texture_update_combined_gs3files does it, with the autumn furniture put into the base sheet.
void ILB_mod_season_buildings_art( void )
{
    static int patch[ SHEET_WIDTH * PATCH_HEIGHT ];
    static int have_patch = -1;
    int width = 0, height = 0, sprites;
    int *pixels;

    if ( have_patch < 0 )
    {
        FILE *file = fopen( "mods/objects_halloween.raw", "rb" );
        have_patch = file != 0 && fread( patch, 4, SHEET_WIDTH * PATCH_HEIGHT, file ) == SHEET_WIDTH * PATCH_HEIGHT;
        if ( file != 0 )
            fclose( file );
        if ( !have_patch )
            fprintf( stderr, "mod season buildings: mods/objects_halloween.raw not found or too short; no pumpkins on the furniture in Autumn (tools/make_season_npcs.py makes it)\n" );
    }
    if ( !have_patch || OBJECT_TEXTURE >= 0x100 )
        return;
    sprites = gge_gs3_new_sprites( OBJECT_BASE_FILE );
    if ( sprites == -1 )
        return;
    gge_gs3_get_sprite_size( sprites, (_DWORD *)&width, &height );
    pixels = (int *)gge_gs3_decompress( sprites, 1 );
    gge_gs3_free_sprites( sprites );
    if ( pixels == 0 || width != SHEET_WIDTH || height != SHEET_HEIGHT )
        return;
    for ( int i = 0; i < SHEET_WIDTH * PATCH_HEIGHT; i++ )
        pixels[ i ] = patch[ i ];
    sprites = gge_gs3_new_sprites( OBJECT_AUTUMN_FILE );
    if ( sprites == -1 )
        return;
    gge_gs3_decompress( sprites, 0 );           // on top of the base sheet, as the game does
    gge_gs3_free_sprites( sprites );
    copy_texture_to_card( OBJECT_TEXTURE, pixels, SHEET_WIDTH, SHEET_HEIGHT );
}
#else
void ILB_mod_season_buildings( void ) {}            // the shared hooks call these whether or not the module is in
void ILB_mod_season_buildings_loading( void ) {}
#endif
