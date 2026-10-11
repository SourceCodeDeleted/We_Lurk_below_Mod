// Entry point of the native Linux debug build of the draft (hand-written). See draft/native/native_platform.c.
#include "ILB_draft_decls.h"

typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );
void exit( int );
char *getenv( const char * );
int atoi( const char * );

void ILB_image_init( void );
void ILB_native_trace_init( void );
void gge_gl_load_nothing( void );
void ILB_native_assert_summary( void );

void abort( void );

// What the world looks like when a generator search never ends: per zone (320 rows of the 512 x 1952 map),
// the most common tile types in layer 0 and how many cells of the other layers are set.
void ILB_native_stuck( void )
{
    const int *world = *(const int **)ILB_ADDR( 0x38C793C );
    fprintf( stderr, "native: STUCK - a search is not ending. World block at %p\n", (void *)world );
    ILB_native_assert_summary();
    for ( int zone = 0; world && zone < 6; zone++ )
    {
        static int count[ 4096 ];
        int layers[ 5 ] = { 0 };
        memset( count, 0, sizeof( count ) );
        for ( int y = zone * 320; y < zone * 320 + 320; y++ )
            for ( int x = 0; x < 512; x++ )
            {
                unsigned int tile = (unsigned int)world[ y * 512 + x ];
                count[ tile < 4095 ? tile : 4095 ]++;
                for ( int layer = 1; layer < 5; layer++ )
                    layers[ layer ] += world[ layer * 512 * 1952 + y * 512 + x ] != 0;
            }
        fprintf( stderr, "  zone %d layer 0:", zone );
        for ( int shown = 0; shown < 8; shown++ )
        {
            int best = -1;
            for ( int t = 0; t < 4096; t++ )
                if ( count[ t ] > 0 && ( best < 0 || count[ t ] > count[ best ] ) ) best = t;
            if ( best < 0 ) break;
            fprintf( stderr, " type %d x%d", best, count[ best ] );
            count[ best ] = 0;
        }
        fprintf( stderr, " | set cells in layers 1-4: %d %d %d %d\n", layers[ 1 ], layers[ 2 ], layers[ 3 ], layers[ 4 ] );
    }
    abort();
}

// State of the scripted starts (ILB_SCRIPT), outside the function so that a session which begins inside a
// running game can run the two-player start again from the title menu (ILB_native_script_restart).
static unsigned int title_frame = 0xFFFFFFFF;
static int picked;
static unsigned int last_tick_seen = 0xFFFFFFFF;

static FILE *checksum_out;
static unsigned int checksum_last_tick = 0xFFFFFFFF, checksum_first_tick;
static int checksum_generation;
int fclose( FILE * );
int snprintf( char *, unsigned int, const char *, ... );

void ILB_native_script_restart( void )
{
    if ( checksum_out ) fclose( checksum_out );
    checksum_out = 0;
    checksum_last_tick = 0xFFFFFFFF;
    checksum_generation++;
    title_frame = 0xFFFFFFFF;
    picked = 0;
    last_tick_seen = 0xFFFFFFFF;
}

// Called once per frame of the game's main loop (from the SDL_PollEvent stand-in). The game gets no input
// here, so anything beyond sitting in the menus is done by calling its handlers directly at chosen frames.
void ILB_native_frame( unsigned int frame )
{
    static int limit = -1;
    if ( limit < 0 )
#ifdef ILB_WINDOW
        limit = getenv( "ILB_FRAMES" ) ? atoi( getenv( "ILB_FRAMES" ) ) : 0x7FFFFFFF;      // play until closed
#else
        limit = getenv( "ILB_FRAMES" ) ? atoi( getenv( "ILB_FRAMES" ) ) : 300;
#endif
    // Everything below goes by the game's own count of simulation steps (0x1703C3C), not by trips round the
    // main loop: a fast build makes several trips per step, and scripted actions must land on the same step
    // in every build. (In the headless build each trip is one step.)
    {
        unsigned int tick = *(unsigned int *)ILB_ADDR( 0x1703C3C );
        if ( tick == last_tick_seen )
            return;
        last_tick_seen = tick;
        frame = tick;
    }
    if ( frame == 0 )
        fprintf( stderr, "native: main loop reached\n" );
    if ( frame % 30 == 0 )
        fprintf( stderr, "native: frame %u, app mode %d\n", frame, *(int *)ILB_ADDR( 0x3A73CC0 ) );

    // ILB_SCRIPT=world: pick character slot 1 as if it held a character. The "load game" mode then finds
    // no saved world and generates a new one (game_new_world -> rlg_create), which is the code under test.
    const char *script = getenv( "ILB_SCRIPT" );
    // the scripts start a few frames after the title menu first shows (app mode 2), and not before frame 100;
    // how many frames the splash screen takes depends on how fast the build is
    // a run that goes straight into a game (network session, or a host carrying on alone): no waiting at the
    // title menu. Both copies of a session do this on the same steps. (The splash screen before it cannot be
    // skipped: the game loads its graphics while it shows.)
    int straight_in = getenv( "ILB_NET_FD" ) != 0 || getenv( "ILB_AUTOHOST_NAME" ) != 0 || ILB_input_net_in_place();
    if ( getenv( "ILB_AUTOHOST_NAME" ) != 0 && *(int *)ILB_ADDR( 0x3A73CC0 ) == 1 )     // (not for a session: both copies must take the same path)
    {
        // ... but its waiting can: no fade-in (state 0), and no waiting for the logo's sound to end (state 2).
        // State 1, where the loading happens, is left alone.
        int *state = (int *)ILB_ADDR( 0x3735524 ), *fade = (int *)ILB_ADDR( 0x3735528 );
        if ( *state == 0 && *fade < 96 ) *fade = 96;
        if ( *state == 2 ) { *state = 3; *fade = 5; }
    }
    if ( title_frame == 0xFFFFFFFF && *(int *)ILB_ADDR( 0x3A73CC0 ) == 2 )
        title_frame = straight_in ? frame + 2 : frame + 10 < 100 ? 100 : frame + 10;
    // ILB_WATCH_MISSILES: every 20 frames, the missiles alive (list head 0x38A5C2C) with the fields their
    // update uses: class, weapon flags, frames left (custom data +12), speed (+16), unit flags
    if ( getenv( "ILB_WATCH_MISSILES" ) && frame % 20 == 0 && *(int *)ILB_ADDR( 0x3A73CC0 ) == 11 )
    {
        int count = 0;
        for ( struct unit *m = *(struct unit **)ILB_ADDR( 0x38A5C2C ); m && count < 200; m = m->type_next, count++ )
            if ( count < 3 )
                fprintf( stderr, "native:   missile %x class %d weapon_flags %x frames_left %d speed %g flags %x at %.1f,%.1f\n",
                         m->id, m->unit_class, m->stats.weapon_flags, *(int *)&m->custom_data[ 12 ],
                         *(float *)&m->custom_data[ 16 ], m->flags, m->position.x, m->position.y );
        if ( count )
            fprintf( stderr, "native: frame %u: %d missiles alive\n", frame, count );
    }

    // ILB_WATCH_TOWN: every 120 frames, the invasion state and who is in the world: invaders left
    // (0x388C0AC), hour (0x5DFA00), the player's invasion chance / hour / happening (data dwords 1088-1090),
    // units per type, and each NPC (type 5) with its class and flags (bit 1 of custom data = hidden)
    if ( getenv( "ILB_WATCH_TOWN" ) && frame % 120 == 0 && *(int *)ILB_ADDR( 0x3A73CC0 ) == 11 )
    {
        char *units = *(char **)ILB_ADDR( 0x3A73C9C );
        int types[ 16 ] = { 0 }, invaders = 0;
        char *player = (char *)player_get_unit();
        fprintf( stderr, "native: town frame %u: invaders left %d, hour %d", frame, *(int *)ILB_ADDR( 0x388C0AC ), *(int *)ILB_ADDR( 0x5DFA00 ) );
        if ( player )
            fprintf( stderr, ", chance %d hour %d happening %d ok %d, player at %.0f,%.0f world %d\n", *(int *)( player + 2792 + 4352 ),
                     *(int *)( player + 2792 + 4356 ), *(int *)( player + 2792 + 4360 ), player[ 2792 + 3489 ],
                     ( (struct unit *)player )->position.x, ( (struct unit *)player )->position.y, ( (struct unit *)player )->world_id );
        for ( int i = 0; units && i < 0x4000; i++ )
        {
            struct unit *u = (struct unit *)( units + i * 8936 );
            if ( u->id == (dword)-1 || ( u->id & 0x3FFF ) != (dword)i )
                continue;
            if ( u->unit_type >= 0 && u->unit_type < 16 ) types[ u->unit_type ]++;
            if ( u->unit_type == 1 && ( *(unsigned *)u->custom_data & 0x20000000 ) ) invaders++;
            if ( u->unit_type == 5 || u->unit_type == 6 )
                fprintf( stderr, "native:   %s class %d flags %x data %x world %d at %.0f,%.0f\n", u->unit_type == 5 ? "npc" : "type6",
                         u->unit_class, u->flags, *(unsigned *)u->custom_data, u->world_id, u->position.x, u->position.y );
        }
        fprintf( stderr, "native:   units by type:" );
        for ( int t = 0; t < 16; t++ ) if ( types[ t ] ) fprintf( stderr, " %d:%d", t, types[ t ] );
        fprintf( stderr, " | monsters flagged invader %d\n", invaders );
    }

    // ILB_TEST_BLUEPRINT=<type>: at frame 300 run the building spawner for that blueprint type at the player
    if ( getenv( "ILB_TEST_BLUEPRINT" ) && frame == 300 && *(int *)ILB_ADDR( 0x3A73CC0 ) == 11 )
    {
        struct unit *player = (struct unit *)player_get_unit();
        int type = atoi( getenv( "ILB_TEST_BLUEPRINT" ) ), codes = 0;
        blueprint_fill_arrays( type );
        for ( int i = 0; i < 625; i++ )
            if ( dword_60C180[ i ] != -1 ) { codes++; fprintf( stderr, "native:   spawn code %d at %d,%d\n", dword_60C180[ i ], i % 25, i / 25 ); }
        fprintf( stderr, "native: blueprint type %d has %d spawn codes; spawning at the player\n", type, codes );
        sub_43BEF0( 0, (short)player->position.x, (int)player->position.y, -1, (int)dword_60C180 );
    }
    // ILB_TEST_SEASON=<0-3>: at frame 600 set the calendar's season and reload the season's object sheet, as
    // the game does when a season begins (test runs only; nothing is saved)
    if ( getenv( "ILB_TEST_SEASON" ) && frame == 600 && *(int *)ILB_ADDR( 0x3A73CC0 ) == 11 )
    {
        *(int *)ILB_ADDR( 0x3891918 ) = atoi( getenv( "ILB_TEST_SEASON" ) );
        objects_update_seasons_gfx();
    }
    // ILB_TEST_DROP: at frames 300 and 660 run the seasonal currency drop (forced, no dice) for up to 40
    // monsters and count the pumpkin (533) and snowflake (1022) items in the world before and after
    if ( getenv( "ILB_TEST_DROP" ) && ( frame == 300 || frame == 660 ) && *(int *)ILB_ADDR( 0x3A73CC0 ) == 11 )
    {
        char *units = *(char **)ILB_ADDR( 0x3A73C9C );
        struct unit *player = (struct unit *)player_get_unit();
        int count[ 2 ][ 2 ] = { { 0 } }, calls = 0;
        for ( int pass = 0; pass < 2; pass++ )
        {
            for ( int i = 0; units && i < 0x4000; i++ )
            {
                struct unit *u = (struct unit *)( units + i * 8936 );
                if ( u->id == (dword)-1 || ( u->id & 0x3FFF ) != (dword)i )
                    continue;
                if ( pass == 1 && u->unit_type == 3 && ( u->unit_class == 533 || u->unit_class == 1022 ) && count[ 1 ][ 0 ] + count[ 1 ][ 1 ] < 2 )
                    fprintf( stderr, "native:   item %x class %d stack %d at %.0f,%.0f\n", u->id, u->unit_class, *(int *)&u->custom_data[ 12 ], u->position.x, u->position.y );
                if ( u->unit_type == 3 && u->unit_class == 533 ) count[ pass ][ 0 ]++;
                if ( u->unit_type == 3 && u->unit_class == 1022 ) count[ pass ][ 1 ]++;
            }
            for ( int i = 0; pass == 0 && units && i < 0x4000 && calls < 40; i++ )
            {
                struct unit *u = (struct unit *)( units + i * 8936 );
                if ( u->id != (dword)-1 && ( u->id & 0x3FFF ) == (dword)i && u->unit_type == 1 && monster_will_give_exp( u, player ) )
                {
                    if ( calls < 3 )
                        fprintf( stderr, "native:   monster class %d level %d gives exp %d (player level %d)\n", u->unit_class, u->level,
                                 monster_will_give_exp( u, player ), player->level );
                    monster_drop_seasonal_currency( u, player, 1 );
                    calls++;
                }
            }
        }
        fprintf( stderr, "native: season %d, drop run for %d monsters: pumpkins %d -> %d, snowflakes %d -> %d\n", *(int *)ILB_ADDR( 0x3891918 ),
                 calls, count[ 0 ][ 0 ], count[ 1 ][ 0 ], count[ 0 ][ 1 ], count[ 1 ][ 1 ] );
    }
    // ILB_TEST_NPCS="a b ...": at frame 300 stand those NPC classes in a row at the player, 3 tiles apart
    if ( getenv( "ILB_TEST_NPCS" ) && frame == 300 && *(int *)ILB_ADDR( 0x3A73CC0 ) == 11 )
    {
        struct unit *player = (struct unit *)player_get_unit();
        const char *list = getenv( "ILB_TEST_NPCS" );
        for ( int i = 0; *list; i++ )
        {
            float x = player->position.x - 12.0f + 3.0f * i;
            npc_spawn( 0, atoi( list ), *(int *)&x, player->position.y );
            while ( *list && *list != ' ' ) list++;
            while ( *list == ' ' ) list++;
        }
    }

    // ILB_TEST_QUESTS="a b ...": from frame 300, one every 60 frames, complete that quest the way the game does
    // (state "objectives done", then quest_complete_ok) and print what the player was given
    if ( getenv( "ILB_TEST_QUESTS" ) && frame >= 300 && frame % 60 == 0 && *(int *)ILB_ADDR( 0x3A73CC0 ) == 11 )
    {
        struct unit *player = (struct unit *)player_get_unit();
        const char *list = getenv( "ILB_TEST_QUESTS" );
        int quest = -1, index = ( frame - 300 ) / 60;
        for ( int i = 0; *list; i++ )
        {
            int value = atoi( list );
            if ( i == index ) quest = value;
            while ( *list && *list != ' ' ) list++;
            while ( *list == ' ' ) list++;
        }
        if ( quest >= 0 && player )
        {
            player_set_quest_state( player, quest, 3 );
            quest_complete_ok( quest );
            fprintf( stderr, "native: quest %d completed; started quests now:", quest );
            for ( int q = 0; q < 56; q++ )
                if ( player_get_quest_state( player, q ) != 0 ) fprintf( stderr, " %d=%d", q, player_get_quest_state( player, q ) );
            fprintf( stderr, "\nnative:   inventory item classes:" );
            for ( int i = 0; i < 46; i++ )
            {
                dword id = *(dword *)( (char *)player + 2792 + 460 + 4 * i );
                if ( id != (dword)-1 )
                    fprintf( stderr, " %d", ( (struct unit *)( *(char **)ILB_ADDR( 0x3A73C9C ) + ( id & 0x3FFF ) * 8936 ) )->unit_class );
            }
            fprintf( stderr, "\n" );
        }
    }

    // ILB_SCRIPT=coop: the game's own Local Multiplayer with two saved characters (ILB_SLOT, ILB_SLOT2).
    // Frame 100: the set-up screen's "start" handler (0x4C3C80: two players, input types, character select).
    // Then each time the character select screen is up for the player whose turn it is, pick that slot.
    if ( script && script[ 0 ] == 'c' )
    {
        int mode = *(int *)ILB_ADDR( 0x3A73CC0 ), selecting = *(int *)ILB_ADDR( 0x6113EC );
        if ( frame == title_frame )
        {
            fprintf( stderr, "native: script - local multiplayer, two players\n" );
            // which device each player starts on: 1 keyboard and mouse, 2 controller. Local co-op needs one of
            // each; in a network game both start on keyboard and mouse and switch to whatever they touch
            *(int *)ILB_ADDR( 0x5E0114 ) = 1;
            *(int *)ILB_ADDR( 0x5E00AC ) = ILB_input_net_session() ? 1 : 2;
            sub_4C3C80();
        }
        if ( frame > title_frame + ( straight_in ? 3 : 30 ) && frame % ( straight_in ? 4 : 30 ) == 0 && mode == 6 && picked < selecting && selecting <= 2 )
        {
            int slot = atoi( getenv( selecting == 1 ? "ILB_SLOT" : "ILB_SLOT2" ) ? getenv( selecting == 1 ? "ILB_SLOT" : "ILB_SLOT2" ) : ( selecting == 1 ? "3" : "1" ) );
            fprintf( stderr, "native: script - player %d picks the character in slot %d\n", selecting, slot );
            picked = selecting;
            cs_do_select( 99 + slot );
        }
        if ( frame % 120 == 0 && mode == 11 )
            fprintf( stderr, "native: coop frame %u: players %d, clients %d, views %d\n", frame, *(int *)ILB_ADDR( 0x6113F0 ),
                     *(int *)ILB_ADDR( 0x6113E8 ), *(int *)ILB_ADDR( 0x6113F4 ) );
    }

    // ILB_TEST_MODE=<n>: at frame 100 ask for that app mode the way the menu buttons do (0x609908)
    if ( getenv( "ILB_TEST_MODE" ) && frame == title_frame )
        *(int *)ILB_ADDR( 0x609908 ) = atoi( getenv( "ILB_TEST_MODE" ) );

    // ILB_CHECKSUM=<file>: one line per simulation tick in play (tick counter 0x1703C3C): the random state and
    // a hash of every live unit's identity, position, movement, level and the start of its own data. Two runs
    // that got the same input must write the same file; the first different line is where they part.
    if ( getenv( "ILB_CHECKSUM" ) && *(int *)ILB_ADDR( 0x3A73CC0 ) == 11 )
    {
        #define out checksum_out
        #define last_tick checksum_last_tick
        #define first_tick checksum_first_tick
        unsigned int tick = *(unsigned int *)ILB_ADDR( 0x1703C3C );
        unsigned int ILB_native_rand_state( void );
        if ( out == 0 )
        {
            char name[ 1024 ];      // a session that began inside the running game gets a file of its own: <file>.s1, .s2, ...
            snprintf( name, sizeof name, checksum_generation ? "%s.s%d" : "%s", getenv( "ILB_CHECKSUM" ), checksum_generation );
            if ( ( out = fopen( name, "w" ) ) != 0 )
                first_tick = tick;
        }
        if ( out != 0 && tick != last_tick )
        {
            char *units = *(char **)ILB_ADDR( 0x3A73C9C );
            unsigned int hash = 2166136261u, live = 0, by_type[ 8 ] = { 0 };
            for ( int i = 0; units && i < 0x4000; i++ )
            {
                struct unit *u = (struct unit *)( units + i * 8936 );
                unsigned int h = 2166136261u;
                if ( u->id == (dword)-1 || ( u->id & 0x3FFF ) != (dword)i )
                    continue;
                #define MIX( p, n ) for ( unsigned int k = 0; k < ( n ); k++ ) h = ( h ^ ( (const unsigned char *)( p ) )[ k ] ) * 16777619u
                MIX( &u->id, 4 ); MIX( &u->unit_type, 8 ); MIX( &u->position, 8 ); MIX( &u->forces, 8 ); MIX( &u->level, 4 );
                MIX( u->custom_data, 64 );
                #undef MIX
                hash = ( hash ^ h ) * 16777619u;
                live++;
                if ( u->unit_type >= 0 && u->unit_type < 8 )
                    by_type[ u->unit_type ] = ( by_type[ u->unit_type ] ^ h ) * 16777619u;
            }
            fprintf( out, "%u rand %08x units %u hash %08x  players %08x monsters %08x items %08x objects %08x npcs %08x\n", tick - first_tick,
                     ILB_native_rand_state(), live, hash, by_type[ 0 ], by_type[ 1 ], by_type[ 3 ], by_type[ 4 ], by_type[ 5 ] );
            last_tick = tick;
        }
        #undef out
        #undef last_tick
        #undef first_tick
    }

    // ILB_TEST_KEYS="P 30 D D E 60 C3 30 E T:secret E E": drive the menus from the title screen on. A number
    // waits that many steps (default 8 between actions); P presses Play; C<n> picks character slot n on the
    // character screen; U D E X B are Up, Down, Enter, Escape, Backspace; T:<text> types text. Keys go to
    // the current screen's own key handlers (app mode table at 0x609910).
    if ( getenv( "ILB_TEST_KEYS" ) && frame >= title_frame )
    {
        static const char *next;
        static unsigned int due;
        if ( next == 0 ) { next = getenv( "ILB_TEST_KEYS" ); due = frame; }
        while ( *next == ' ' ) next++;
        if ( *next && frame >= due )
        {
            void **handlers = (void **)ILB_ADDR( 0x609910 + 48 * *(int *)ILB_ADDR( 0x3A73CC0 ) );
            void ( *key_down )( int ) = (void ( * )( int ))handlers[ 7 ], ( *character )( int ) = (void ( * )( int ))handlers[ 9 ];
            int key = 0;
            due = frame + 8;
            if ( *next >= '0' && *next <= '9' ) due = frame + atoi( next );
            else if ( *next == 'P' ) mm_do_play( 100 );
            else if ( *next == 'C' ) cs_do_select( 99 + atoi( next + 1 ) );
            else if ( *next == 'M' )                    // M<x>,<y>: the mouse moves there
            {
                int x = atoi( next + 1 ), y = 0;
                for ( const char *c = next; *c && *c != ' '; c++ ) if ( *c == ',' ) y = atoi( c + 1 );
                if ( handlers[ 3 ] ) ( (void ( * )( int, int, int ))handlers[ 3 ] )( x, y, 0 );
            }
            else if ( *next == 'L' ) { if ( handlers[ 4 ] ) ( (void ( * )( int, int ))handlers[ 4 ] )( 1, 0 ); }     // left click
            else if ( *next == 'A' || *next == 'J' )    // controller: A<button> pressed, J<value> = left stick up/down
            {
                if ( *next == 'A' && handlers[ 11 ] ) ( (void ( * )( int, int ))handlers[ 11 ] )( 1, atoi( next + 1 ) );
                if ( *next == 'J' && handlers[ 10 ] ) ( (void ( * )( int, int ))handlers[ 10 ] )( 1, atoi( next + 1 ) );
            }
            else if ( *next == 'T' && next[ 1 ] == ':' )
                for ( const char *c = next + 2; *c && *c != ' '; c++ ) { if ( key_down ) key_down( *c ); if ( character ) character( *c ); }
            else key = *next == 'U' ? 0x40000052 : *next == 'D' ? 0x40000051 : *next == 'E' ? 13 : *next == 'X' ? 27 : *next == 'B' ? 8 : 0;
            if ( key && key_down ) key_down( key );
            fprintf( stderr, "native: test keys - %.12s (app mode %d)\n", next, *(int *)ILB_ADDR( 0x3A73CC0 ) );
            while ( *next && *next != ' ' ) next++;
        }
    }

    {
        void ILB_input_test_quit( void );
        ILB_input_test_quit();          // ILB_TEST_QUIT: a scripted "quit from the pause menu" in a test session
    }

    // ILB_TEST_ANCIENT: from frame 300 use the ancient items on the hotbar one after the other (one every 150
    // frames, through the game's own hotbar routine) and report what appears in the world afterwards: units
    // by type (2 = missiles), and the asserts fire in the usual way.
    if ( getenv( "ILB_TEST_ANCIENT" ) && frame >= 300 && *(int *)ILB_ADDR( 0x3A73CC0 ) == 11 )
    {
        static int before[ 8 ], most[ 8 ], current = -1;
        char *units = *(char **)ILB_ADDR( 0x3A73C9C );
        struct unit *player = (struct unit *)player_get_unit();
        int now[ 8 ] = { 0 }, slot = ( frame - 300 ) / 150, phase = ( frame - 300 ) % 150;
        for ( int i = 0; units && i < 0x4000; i++ )
        {
            struct unit *u = (struct unit *)( units + i * 8936 );
            if ( u->id != (dword)-1 && ( u->id & 0x3FFF ) == (dword)i && u->unit_type >= 0 && u->unit_type < 8 ) now[ u->unit_type ]++;
        }
        for ( int t = 0; t < 8; t++ ) if ( now[ t ] > most[ t ] ) most[ t ] = now[ t ];
        if ( phase == 0 && slot < 6 && player )
        {
            int id = player_get_hotbar_ancient_id( player, slot );
            struct unit *item = id != -1 && units ? (struct unit *)( units + ( id & 0x3FFF ) * 8936 ) : 0;
            for ( int t = 0; t < 8; t++ ) before[ t ] = most[ t ] = now[ t ];
            current = slot;
            fprintf( stderr, "native: ancient slot %d: item class %d%s\n", slot, item ? item->unit_class : -1, item ? " - using it" : " (empty)" );
            if ( item ) player_use_hotbar_ancient( slot );
        }
        if ( ( phase == 8 || phase == 20 ) && current == slot )    // the plain particles alive shortly after the use
            for ( int i = 0; i < 0x800; i++ )
            {
                char *part = (char *)ILB_ADDR( 0x818608 ) + i * 1328;
                float *position = *(float **)( part + 12 );
                if ( *(int *)( part + 4 ) == -1 || *(int *)( part + 4 ) > 1 || !position ) continue;
                fprintf( stderr, "native:   particle %d kind %d at %.2f, %.2f (player %.2f, %.2f) size %.3f x %.3f depth %.2f colour %08x lit %d\n", i, *(int *)( part + 4 ),
                         position[ 0 ], position[ 1 ], player ? player->position.x : 0, player ? player->position.y : 0,
                         *(float *)( part + 20 ), *(float *)( part + 24 ), *(float *)( part + 16 ), *(unsigned *)( part + 28 ), !part[ 32 ] );
            }
        if ( phase == 149 && current == slot )
            fprintf( stderr, "native:   afterwards (most at one time, before -> after): missiles %d -> %d, monsters %d -> %d, items %d -> %d, objects %d -> %d, type 6: %d -> %d, type 7: %d -> %d\n",
                     before[ 2 ], most[ 2 ], before[ 1 ], most[ 1 ], before[ 3 ], most[ 3 ], before[ 4 ], most[ 4 ], before[ 6 ], most[ 6 ], before[ 7 ], most[ 7 ] );
    }

    // ILB_SCRIPT=load: as a player would - read the character slots from the save files (the character
    // select screen's own set-up, 0x42BCC0), then pick slot 1. Needs a character created earlier in the
    // window build; the level then loads with a real player.
    if ( script && script[ 0 ] == 'l' && frame == title_frame )
    {
        int slot = getenv( "ILB_SLOT" ) ? atoi( getenv( "ILB_SLOT" ) ) : 1;
        fprintf( stderr, "native: script - loading the character in slot %d\n", slot );
        sub_42BCC0();
        cs_do_select( 99 + slot );
    }
#ifdef ILB_CHEATS
    // ILB_SCRIPT=load with ILB_TEST_CHEATS set: press the testing shortcuts once the level is running
    if ( script && script[ 0 ] == 'l' && getenv( "ILB_TEST_CHEATS" ) )
    {
        int ILB_cheat_key( int key, int pressed );
        if ( frame == 900 ) ILB_cheat_key( '`', 1 );
        if ( frame == 1200 ) ILB_cheat_key( 0x4000003A, 1 );
        if ( frame == 1300 ) ILB_cheat_key( 0x4000003B, 1 );
        if ( frame == 1350 ) ILB_cheat_key( 0x4000003C, 1 );
        if ( frame == 1400 ) ILB_cheat_key( 0x4000003C, 1 );     // a second time: everything is topped up, nothing made
        // god mode: on, then the player is hit for far more than it has (both damage routines) and told to die
        if ( frame == 600 || frame == 700 )
        {
            struct unit *player = (struct unit *)player_get_unit();
            if ( frame == 600 ) ILB_cheat_key( 0x4000003D, 1 );
            fprintf( stderr, "native: god test frame %u: hit points %d", frame, ( (int *)player )[ 555 ] );
            unit_take_undefended_damage( player->id, -1, 100000, 0, -1 );
            fprintf( stderr, " -> %d after 100000 undefended damage\n", ( (int *)player )[ 555 ] );
            if ( frame == 600 ) { do_player_die( player ); fprintf( stderr, "native: god test: told to die, app mode %d, hit points %d\n", *(int *)ILB_ADDR( 0x3A73CC0 ), ( (int *)player )[ 555 ] ); }
            if ( frame == 600 ) ILB_cheat_key( 0x4000003D, 1 );     // off again: the hit at frame 700 must count
        }
    }
#endif
    if ( script && script[ 0 ] == 'w' && frame == title_frame )
    {
        // the title menu is up by now (app mode 2). The character select screen is skipped: its slot table
        // (260-byte records at 0x60E898: in use, has a second flag, enabled) is filled in by hand.
        fprintf( stderr, "native: script - selecting character slot 1 (players %d of %d)\n",
                 *(int *)ILB_ADDR( 0x6113EC ), *(int *)ILB_ADDR( 0x6113F0 ) );
        *(char *)ILB_ADDR( 0x60E898 ) = 1;          // slot 0: in use
        *(short *)ILB_ADDR( 0x60E89A ) = 1;         // slot 0: enabled
        cs_do_select( 100 );                        // csbtn_slot_start_id + 0
    }
    if ( (int)frame >= limit )
    {
        fprintf( stderr, "native: %u frames run, stopping\n", frame );
        ILB_native_assert_summary();
        exit( 0 );
    }
}

void net_session_from_environment( char **argv );
int ILB_input_net_in_place( void );
int ILB_input_net_session( void );
char **ILB_argv;      // for a copy that restarts itself into a network game (port/net/net_session.c)

int main( int argc, char **argv )
{
    ILB_native_trace_init();
    ILB_image_init();
#ifndef ILB_WINDOW
    gge_gl_load_nothing();      // headless: the renderer runs, OpenGL calls do nothing
#endif
    // as the original's start-up code does: legacy wide printf behaviour (see draft/glue/startup.c)
    *(unsigned int *)ILB_ADDR( 0x60A020 ) |= 0x24;
    *(unsigned int *)ILB_ADDR( 0x60A008 ) |= 2;
    // ILB_LOBBY_HOST / ILB_LOBBY_JOIN: match with another copy through the lobby server, exchange saves and
    // restart into the synchronised two-player session (port/net/net_session.c); returns if not asked for
    ILB_argv = argv;
    net_session_from_environment( argv );
    return gdi_init( argc, (HKEY)argv );        // the game's SDL_main (0x581B40)
}
