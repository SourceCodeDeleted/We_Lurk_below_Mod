// mod_season_outfits.c - town vendors dressed for the in-game season (hand-written, NOT part of the original).
//
// To remove: configure with -DILB_MOD_SEASON_OUTFITS=OFF (and delete this file if wanted). The only reference
// to it is one guarded call at the start of process_npcs, placed by tools/draft_fix.py (ENTRY_HOOKS, shared).
//
// The original never changes the vendors' clothes: each release ships one vendor sheet drawn for the time
// of year it came out (this build: Christmas). With this module the sheet follows the game's calendar:
//
//   Spring, Summer   plain outfits
//   Autumn           Halloween costumes
//   Winter           Christmas costumes
//
// The three sheets are made from the owner's copies of the game by tools/make_season_npcs.py and read from
// mods/npcs_<name>.raw next to the game's pack file (512x256 pixels, as the game's sheet decoder produces
// them). A missing file leaves the vendors as they are. Only the art changes; the holiday events do not.
#ifdef ILB_MOD_SEASON_OUTFITS
#include "ILB_draft_decls.h"

typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );
int snprintf( char *, unsigned int, const char *, ... );
FILE *fopen( const char *, const char * );
unsigned int fread( void *, unsigned int, unsigned int, FILE * );
int fclose( FILE * );

#define SEASON              ( *(int *)ILB_ADDR( 0x3891918 ) )       // 0 spring .. 3 winter (objects_update_seasons_gfx)
#define VENDOR_TEXTURE      ( *(unsigned int *)ILB_ADDR( 0x5E029C ) )   // made from units\npcs\npcs.gs3 at start-up
#define APP_MODE            ( *(int *)ILB_ADDR( 0x3A73CC0 ) )
#define APP_MODE_GAME       11
#define SHEET_WIDTH         512
#define SHEET_HEIGHT        256

// Called at the start of process_npcs (every frame of play); does something only when the season changed.
void ILB_mod_season_outfits( void )
{
    static const char *const sheet_of_season[ 4 ] = { "plain", "plain", "halloween", "christmas" };
    static int applied = -1;
    static int pixels[ SHEET_WIDTH * SHEET_HEIGHT ];
    char path[ 64 ];
    FILE *file;

    if ( APP_MODE != APP_MODE_GAME || SEASON < 0 || SEASON > 3 || SEASON == applied || VENDOR_TEXTURE >= 0x100 )
        return;
    applied = SEASON;
    snprintf( path, sizeof path, "mods/npcs_%s.raw", sheet_of_season[ SEASON ] );
    file = fopen( path, "rb" );
    if ( file == 0 || fread( pixels, 4, SHEET_WIDTH * SHEET_HEIGHT, file ) != SHEET_WIDTH * SHEET_HEIGHT )
    {
        fprintf( stderr, "mod season outfits: %s not found or too short; vendors left as they are (tools/make_season_npcs.py makes it)\n", path );
        if ( file != 0 )
            fclose( file );
        return;
    }
    fclose( file );
    copy_texture_to_card( VENDOR_TEXTURE, pixels, SHEET_WIDTH, SHEET_HEIGHT );     // as the game reloads a sheet (update_ambient_texture)
    fprintf( stderr, "mod season outfits: season %d, vendors wear %s\n", SEASON, sheet_of_season[ SEASON ] );
}
#else
void ILB_mod_season_outfits( void ) {}       // the per-step hook calls it whether or not the module is in
#endif
