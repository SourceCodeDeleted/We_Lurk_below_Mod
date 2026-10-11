// native_input.c - recording and replaying the player's input, for determinism tests (and later for sending
// one player's input to another machine).
//
//   ILB_RECORD=<file>   write every input event the game is given, with the simulation tick it arrived in
//   ILB_REPLAY=<file>   give the game those events again at the same ticks; the real keyboard, mouse and
//                       controller are ignored (closing the window still works)
//
// The game learns about input in two ways: events (SDL_PollEvent) and by asking for the current state of the
// keyboard and mouse (SDL_GetKeyboardState, SDL_GetMouseState). While recording or replaying, the state it
// is told is rebuilt here from the events it has been given, so a replay answers every question exactly as
// the recording did. The random seed is stored in the file too.
//
// The controller is handled the same way, and a replay needs no controller plugged in: the stick positions
// the game asks for are rebuilt from the axis events, and what it asks about the device itself (how many,
// is it a game controller, its name and vendor) is answered from the recording, in the order it was asked.
//
// Two copies of the game playing together (ILB_NET_HOST=<port> on one, ILB_NET_JOIN=<address>:<port> on the
// other) use the same machinery: see "two copies over a network" below.
//
// File: "ILBREC1\n", the seed (4 bytes), then records of 4 + 56 bytes: the tick and SDL2's 56-byte event, or
// tick 0xFFFFFFFF and the answer to a device question (kind, value, text).
// Limits: the starting saves and the window size must be the same as when recording (tools/record_game.sh
// and tools/replay_game.sh look after the saves).
#include "ILB_draft_decls.h"

typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );
FILE *fopen( const char *, const char * );
unsigned int fread( void *, unsigned int, unsigned int, FILE * );
unsigned int fwrite( const void *, unsigned int, unsigned int, FILE * );
int fflush( FILE * );
char *getenv( const char * );
void *memcpy( void *, const void *, unsigned int );
int memcmp( const void *, const void *, unsigned int );

#define TICK            ( *(unsigned int *)ILB_ADDR( 0x1703C3C ) )      // the game's count of simulation steps
#define EVENT_SIZE      56
#define SDL_QUIT        0x100
#define SDL_KEYDOWN     0x300
#define SDL_KEYUP       0x301
#define SDL_MOUSEMOTION 0x400
#define SDL_MOUSEDOWN   0x401
#define SDL_MOUSEUP     0x402
#define SDL_PAD_AXIS    0x650
#define QUERY_RECORD    0xFFFFFFFF

static int mode = -1;                   // 0 off, 1 recording, 2 replaying, 3 two copies over a network
static int net_guest;                   // 0 = hosting copy (player 1), 1 = joining copy (player 2)
#define GAME_WIDTH      ( *(int *)ILB_ADDR( 0x652138 ) )            // the window width the game works with
#define GAME_VIEWS      ( *(int *)ILB_ADDR( 0x6113F4 ) )
static int one_view( void );
static int view_base_width;
static void net_start( void );
static int window_closing;
static void net_at_exit( void );
static int net_epoch_pending;
void net_link_adopt( int fd );
unsigned long strtoul( const char *, char **, int );
int atexit( void ( * )( void ) );
int atoi( const char * );
static int net_poll( int ( *real_poll )( void * ), void *event );
static FILE *file;
static unsigned int seed, have_seed;
static unsigned int next_tick, have_next, events_done;
static unsigned char next_event[ EVENT_SIZE ];
// The devices as the game is told about them, one set per player: in a network game each copy's keyboard,
// mouse or controller belongs to its own player, and the game asks about "the" keyboard or "the"
// controller while it is busy with one particular player. (Recording and replaying use set 0 only.)
#define PLAYERS 2
static unsigned char all_keys[ PLAYERS ][ 512 ];
static int all_mouse_x[ PLAYERS ], all_mouse_y[ PLAYERS ], all_mouse_buttons[ PLAYERS ];
static short all_pad_axis[ PLAYERS ][ 8 ];
static int event_player = -1;           // whose event the game is handling right now; -1 = none
static int event_is_pad;                 // ... and whether it came from a controller
static int state_player( void );
#define keys            ( all_keys[ state_player() ] )
#define mouse_x         ( all_mouse_x[ state_player() ] )
#define mouse_y         ( all_mouse_y[ state_player() ] )
#define mouse_buttons   ( all_mouse_buttons[ state_player() ] )
#define pad_axis        ( all_pad_axis[ state_player() ] )

static void read_next( void )
{
    have_next = fread( &next_tick, 4, 1, file ) == 1 && fread( next_event, EVENT_SIZE, 1, file ) == 1;
    if ( !have_next )
        fprintf( stderr, "native input: end of the recording after %u events, at tick %u\n", events_done, TICK );
}

static void start( void )
{
    char magic[ 8 ];
    mode = 0;
    if ( getenv( "ILB_REPLAY" ) )
    {
        file = fopen( getenv( "ILB_REPLAY" ), "rb" );
        if ( file == 0 || fread( magic, 8, 1, file ) != 1 || memcmp( magic, "ILBREC1\n", 8 ) != 0 || fread( &seed, 4, 1, file ) != 1 )
        {
            fprintf( stderr, "native input: cannot replay %s\n", getenv( "ILB_REPLAY" ) );
            return;
        }
        mode = 2;
        have_seed = 1;
        read_next();
        fprintf( stderr, "native input: replaying %s (seed %u)\n", getenv( "ILB_REPLAY" ), seed );
    }
    else if ( getenv( "ILB_NET_FD" ) )
    {
        // a joining copy started for a session by the lobby step: the connection exists already and the seed
        // was agreed there; the session itself begins at the title menu (ILB_input_snapshot_start_state)
        net_link_adopt( atoi( getenv( "ILB_NET_FD" ) ) );
        seed = (unsigned int)strtoul( getenv( "ILB_NET_SEED" ) ? getenv( "ILB_NET_SEED" ) : "1", 0, 10 );
        net_epoch_pending = 1;
        atexit( net_at_exit );
    }
    else if ( getenv( "ILB_NET_HOST" ) || getenv( "ILB_NET_JOIN" ) )
        net_start();
    else if ( getenv( "ILB_RECORD" ) )
    {
        file = fopen( getenv( "ILB_RECORD" ), "wb" );
        if ( file == 0 )
        {
            fprintf( stderr, "native input: cannot write %s\n", getenv( "ILB_RECORD" ) );
            return;
        }
        mode = 1;
        fwrite( "ILBREC1\n", 8, 1, file );
        fprintf( stderr, "native input: recording to %s\n", getenv( "ILB_RECORD" ) );
    }
}

int ILB_input_active( void )
{
    if ( mode < 0 ) start();
    return mode != 0;
}

// The seed the game is about to give srand: recorded, or replaced by the recorded one.
unsigned int ILB_input_seed( unsigned int wanted )
{
    if ( !ILB_input_active() )
        return wanted;
    if ( mode == 1 && !have_seed )
    {
        seed = wanted;
        have_seed = 1;
        fwrite( &seed, 4, 1, file );
        fflush( file );
    }
    return seed;
}

static void apply( const unsigned char *event )        // keep the keyboard and mouse state in step
{
    unsigned int type = *(const unsigned int *)event;
    if ( type == SDL_KEYDOWN || type == SDL_KEYUP )
    {
        unsigned int scancode = *(const unsigned int *)( event + 16 );
        if ( scancode < sizeof keys ) keys[ scancode ] = type == SDL_KEYDOWN;
    }
    else if ( type == SDL_MOUSEMOTION )
    {
        mouse_buttons = *(const int *)( event + 16 );
        mouse_x = *(const int *)( event + 20 );
        mouse_y = *(const int *)( event + 24 );
    }
    else if ( type == SDL_PAD_AXIS )
    {
        if ( event[ 12 ] < 8 ) pad_axis[ event[ 12 ] ] = *(const short *)( event + 16 );
    }
    else if ( type == SDL_MOUSEDOWN || type == SDL_MOUSEUP )
    {
        int bit = 1 << ( event[ 16 ] - 1 );
        mouse_buttons = type == SDL_MOUSEDOWN ? mouse_buttons | bit : mouse_buttons & ~bit;
        mouse_x = *(const int *)( event + 20 );
        mouse_y = *(const int *)( event + 24 );
    }
}

// One step of the game's event loop. `real_poll` is the real SDL_PollEvent, or null in the headless build.
// Returns 1 with an event for the game, or 0 when there is none for now.
int ILB_input_poll( int ( *real_poll )( void * ), void *event )
{
    if ( !ILB_input_active() )
        return real_poll ? real_poll( event ) : 0;
    if ( mode == 3 )
        return net_poll( real_poll, event );
    if ( mode == 1 )
    {
        unsigned int tick = TICK, type;
        if ( real_poll == 0 || !real_poll( event ) )
            return 0;
        type = *(unsigned int *)event;
        if ( !have_seed ) ILB_input_seed( 0 );          // an event before the game seeded: keep the file's layout
        // Everything the game may react to, window events included: losing the keyboard focus pauses it. Left
        // out are the two kinds that carry pointers (system messages 0x201, dropped files 0x1000 and up).
        if ( type != 0x201 && type < 0x1000 )
        {
            fwrite( &tick, 4, 1, file );
            fwrite( event, EVENT_SIZE, 1, file );
            if ( type == SDL_QUIT ) fflush( file );
            events_done++;
        }
        apply( event );
        return 1;
    }
    // replaying: real input is thrown away, except closing the window
    while ( real_poll && real_poll( event ) )
        if ( *(unsigned int *)event == SDL_QUIT )
            return 1;
    if ( have_next && next_tick != QUERY_RECORD && next_tick <= TICK )
    {
        memcpy( event, next_event, EVENT_SIZE );
        apply( next_event );
        events_done++;
        read_next();
        return 1;
    }
    return 0;
}

const unsigned char *ILB_input_keys( void )                 { return keys; }
int ILB_input_mouse( int *x, int *y )                       { if ( x ) *x = mouse_x; if ( y ) *y = mouse_y; return mouse_buttons; }
int ILB_input_replay_finished( void )                       { return mode == 2 && !have_next; }

// ---- the controller
// A question the game asks SDL about the controller device. `kind` names the question (any small number,
// the same in every build); the answer is recorded, or taken from the recording.
int ILB_input_query( int kind, int real_value, const char *real_text, const char **text )
{
    static char texts[ 4 ][ 48 ];
    static unsigned int which;
    unsigned char record[ EVENT_SIZE ] = { 0 };
    unsigned int marker = QUERY_RECORD;
    if ( text ) *text = real_text;
    if ( !ILB_input_active() )
        return real_value;
    if ( mode == 1 )
    {
        if ( !have_seed ) ILB_input_seed( 0 );
        *(int *)record = kind;
        *(int *)( record + 4 ) = real_value;
        for ( int i = 0; real_text && real_text[ i ] && i < 47; i++ ) record[ 8 + i ] = real_text[ i ];
        fwrite( &marker, 4, 1, file );
        fwrite( record, EVENT_SIZE, 1, file );
        return real_value;
    }
    if ( mode == 3 )            // both copies are told the same thing: one game controller, player 2's
    {
        if ( text ) *text = "Network player";
        return kind == 1 || kind == 2 || kind == 3;
    }
    if ( !have_next || next_tick != QUERY_RECORD || *(int *)next_event != kind )
    {
        fprintf( stderr, "native input: the game asked controller question %d at tick %u where the recording has %s; answering 0\n", kind, TICK,
                 !have_next ? "ended" : next_tick != QUERY_RECORD ? "an input event" : "another question" );
        if ( text ) *text = "";
        return 0;
    }
    real_value = *(int *)( next_event + 4 );
    if ( text )
    {
        memcpy( texts[ which & 3 ], next_event + 8, 48 );
        *text = texts[ which++ & 3 ];
    }
    read_next();
    return real_value;
}

int ILB_input_pad_axis( int axis )                          { return axis >= 0 && axis < 8 ? pad_axis[ axis ] : 0; }
int ILB_input_net_guest( void )                             { return ILB_input_active() && mode == 3 && net_guest; }
int ILB_input_replaying( void )                             { return ILB_input_active() && mode >= 2; }    // devices are not the real ones

// ---- two copies over a network
// Both copies run the whole game with two players, as local co-op does: player 1 on keyboard and mouse,
// player 2 on a controller. The hosting copy contributes its keyboard and mouse, the joining copy its
// controller; neither uses its own devices directly. Each simulation step works like this on both sides:
//   - input gathered while the step counter is t is sent, to be used when the counter is t + NET_DELAY;
//   - the game is given the host's events for the current step, then the joiner's, always in that order;
//   - the game may not take its next step before that has happened (ILB_input_tick_allowed, called from the
//     game's own "is a step due" routine), so a copy that is ahead waits for the other.
// The copies stay identical because they are given identical input on identical steps; nothing else is
// sent. Both must start from the same saves and the same build (see the checksum file to verify).
//
// Wire format, both directions: "hello" = 'H', seed (4 bytes); a step = 'T', step (4), count (4), events.
int net_link_host( int port );
int net_link_join( const char *host, int port, int seconds );
int net_link_send( const void *data, int size );
int net_link_receive( void *data, int size, int wait );
int atoi( const char * );
void *memset( void *, int, unsigned int );
void exit( int );

#define NET_DELAY       3                   // steps between pressing and acting: 48 ms, hides ordinary lag
#define NET_RING        256                 // steps kept; far more than either side can be ahead
#define NET_MAX_EVENTS  192                 // per step and side; a burst beyond that loses its tail
typedef struct { unsigned int count; unsigned char events[ NET_MAX_EVENTS ][ EVENT_SIZE ]; } net_bucket;

static net_bucket net_mine[ NET_RING ], net_theirs[ NET_RING ];
static unsigned int net_sent_through;       // my buckets for steps below this have been sent
static unsigned int net_have_through;       // their buckets for steps below this have arrived
static int net_delivered_through = -1;      // the game has been given everything for steps up to this
static unsigned int net_deliver_position;
static unsigned char net_inbox[ 1 << 16 ];
static unsigned int net_inbox_size;
static unsigned int net_gathered, net_received;
static FILE *net_fake;                      // ILB_NET_FAKE=<recording>: this copy's "devices" for tests
static unsigned int net_fake_tick, net_fake_have;
static unsigned char net_fake_event[ EVENT_SIZE ];

void net_link_adopt( int fd );
void net_session_keep( int in_step );
void net_session_continue( int in_step, char **argv );
extern char **ILB_argv;
unsigned long strtoul( const char *, char **, int );
int atexit( void ( * )( void ) );
static int net_out_of_step;
static const char *net_ended_why;       // set when an in-place session has just lost its other player
static int announced;                   // the game has been told about player 2's controller
static int net_in_place;                // the session began inside a running game (no restart)

// When a session started through the lobby ends, each player's own progress goes back to the real saves.
static void net_at_exit( void )
{
    net_session_keep( !net_out_of_step );
}

static void net_lost( const char *why )
{
    fprintf( stderr, "native input: %s at step %u; stopping\n", why, TICK );
    // the other player is gone but this player's game is intact: save it as the game does when it is
    // closed, so that what was played is not lost
    if ( !net_out_of_step && *(int *)ILB_ADDR( 0x3A73CC0 ) == 11 )
        game_save( 0, 0 );
    if ( net_in_place )
    {
        // carry on in this window: port/mod_net_menu.c sees the flag on the next pass of the main loop,
        // keeps the progress and puts a host back into its game
        net_ended_why = why;
        return;
    }
    net_session_continue( !net_out_of_step, ILB_argv );     // a session that began with a restart: restart back
    exit( 0 );
}

// A hash of the game state, taken on the same step by both copies and compared (every NET_CHECK steps):
// the random state would be best but lives in native_platform.c; every live unit's identity, position and
// movement catches a disagreement within a step or two of it happening.
#define NET_CHECK 32
unsigned int ILB_native_rand_state( void );
static unsigned int net_state_hash( void )
{
    char *units = *(char **)ILB_ADDR( 0x3A73C9C );
    unsigned int hash = 2166136261u ^ ILB_native_rand_state();
    for ( int i = 0; units && i < 0x4000; i++ )
    {
        struct unit *u = (struct unit *)( units + i * 8936 );
        const unsigned char *p;
        if ( u->id == (dword)-1 || ( u->id & 0x3FFF ) != (dword)i )
            continue;
        p = (const unsigned char *)&u->position;
        hash = ( hash ^ u->id ) * 16777619u;
        for ( int k = 0; k < 8; k++ ) hash = ( hash ^ p[ k ] ) * 16777619u;
        p = (const unsigned char *)&u->forces;
        for ( int k = 0; k < 8; k++ ) hash = ( hash ^ p[ k ] ) * 16777619u;
    }
    return hash;
}
static unsigned int net_my_hash[ 64 ], net_my_hash_step[ 64 ], net_their_hash[ 64 ], net_their_hash_step[ 64 ];

static void net_compare( unsigned int step )
{
    unsigned int slot = ( step / NET_CHECK ) % 64;
    if ( net_my_hash_step[ slot ] == step + 1 && net_their_hash_step[ slot ] == step + 1 && net_my_hash[ slot ] != net_their_hash[ slot ] )
    {
        net_out_of_step = 1;
        net_lost( "the two copies of the game no longer agree (this is a bug; nothing from this session is kept)" );
    }
}

static void net_start( void )
{
    unsigned char hello[ 5 ] = { 'H' };
    if ( getenv( "ILB_NET_HOST" ) )
    {
        extern long time( long * );
        if ( !net_link_host( atoi( getenv( "ILB_NET_HOST" ) ) ) ) return;
        seed = getenv( "ILB_SEED" ) ? (unsigned int)atoi( getenv( "ILB_SEED" ) ) : (unsigned int)time( 0 );
        memcpy( hello + 1, &seed, 4 );
        net_link_send( hello, 5 );
    }
    else
    {
        char host[ 128 ];
        const char *where = getenv( "ILB_NET_JOIN" );
        int i = 0, got = 0;
        for ( ; where[ i ] && where[ i ] != ':' && i < 127; i++ ) host[ i ] = where[ i ];
        host[ i ] = 0;
        if ( !net_link_join( host, where[ i ] == ':' ? atoi( where + i + 1 ) : 27960, 30 ) ) return;
        while ( got < 5 )
        {
            int n = net_link_receive( hello + got, 5 - got, 1 );
            if ( n <= 0 ) return;
            got += n;
        }
        if ( hello[ 0 ] != 'H' ) return;
        memcpy( &seed, hello + 1, 4 );
        net_guest = 1;
    }
    have_seed = 1;
    mode = 3;
    net_sent_through = net_have_through = NET_DELAY;        // nothing can be meant for the first steps
    if ( getenv( "ILB_NET_FAKE" ) && ( net_fake = fopen( getenv( "ILB_NET_FAKE" ), "rb" ) ) != 0 )
    {
        unsigned char skip[ 12 ];
        fread( skip, 12, 1, net_fake );
        net_fake_have = fread( &net_fake_tick, 4, 1, net_fake ) == 1 && fread( net_fake_event, EVENT_SIZE, 1, net_fake ) == 1;
    }
    fprintf( stderr, "native input: connected as %s (seed %u)\n", net_guest ? "player 2, the joining copy" : "player 1, the hosting copy", seed );
}

// Which of this copy's own events take part: keyboard, text and mouse, and the controller's sticks and
// buttons. A player may use either and change at any time (see ILB_input_select_client).
// ILB_NET_INPUT=kbm or =pad limits a copy to one of them. That is only needed when two copies run on ONE
// machine: both would otherwise listen to the same controller, and one controller would move both players.
static int net_keep( unsigned int type )
{
    const char *only = getenv( "ILB_NET_INPUT" );
    int keyboard_or_mouse = type >= 0x300 && type < 0x500, controller = type >= 0x650 && type <= 0x652;
    if ( only && only[ 0 ] == 'k' ) return keyboard_or_mouse;
    if ( only && only[ 0 ] == 'p' ) return controller;
    return keyboard_or_mouse || controller;
}

static void net_gather( const unsigned char *event )
{
    net_bucket *bucket = &net_mine[ ( TICK + NET_DELAY ) % NET_RING ];
    unsigned int type = *(const unsigned int *)event;
    static unsigned int seen_kinds[ 16 ], seen;
    for ( unsigned int i = 0; i <= seen && i < 16; i++ )       // say once which kinds of event this copy's devices give
    {
        if ( i < seen && seen_kinds[ i ] == type ) break;
        if ( i == seen )
        {
            seen_kinds[ seen++ ] = type;
            fprintf( stderr, "native input: first event of kind 0x%x from this copy's devices (%s)\n", type, net_keep( type ) ? "sent" : "not used" );
            break;
        }
    }
    if ( net_keep( type ) && bucket->count < NET_MAX_EVENTS )
    {
        memcpy( bucket->events[ bucket->count++ ], event, EVENT_SIZE );
        net_gathered++;
    }
}

static void net_receive( void )
{
    for ( ;; )
    {
        int n = net_link_receive( net_inbox + net_inbox_size, (int)( sizeof net_inbox - net_inbox_size ), 0 );
        unsigned int used = 0;
        if ( n < 0 ) { net_lost( "the other player has left" ); return; }
        net_inbox_size += (unsigned int)( n > 0 ? n : 0 );
        int progress;
        do
        {
        progress = 0;
        while ( net_inbox_size - used >= 9 && net_inbox[ used ] == 'C' )       // the other copy's state hash for a step
        {
            unsigned int step, hash;
            memcpy( &step, net_inbox + used + 1, 4 );
            memcpy( &hash, net_inbox + used + 5, 4 );
            net_their_hash[ ( step / NET_CHECK ) % 64 ] = hash;
            net_their_hash_step[ ( step / NET_CHECK ) % 64 ] = step + 1;      // + 1: zero means nothing stored
            net_compare( step );
            used += 9;
            progress = 1;
        }
        while ( net_inbox_size - used >= 9 && net_inbox[ used ] == 'T' )
        {
            unsigned int step, count;
            memcpy( &step, net_inbox + used + 1, 4 );
            memcpy( &count, net_inbox + used + 5, 4 );
            if ( count > NET_MAX_EVENTS || step != net_have_through )
            {
                fprintf( stderr, "native input: input for step %u with %u events arrived where step %u was due\n", step, count, net_have_through );
                net_lost( "the other copy sent something unexpected" );
                return;
            }
            if ( net_inbox_size - used < 9 + count * EVENT_SIZE ) break;
            net_theirs[ step % NET_RING ].count = count;
            memcpy( net_theirs[ step % NET_RING ].events, net_inbox + used + 9, count * EVENT_SIZE );
            net_have_through = step + 1;
            net_received += count;
            used += 9 + count * EVENT_SIZE;
            progress = 1;
        }
        }
        while ( progress );
        if ( used < net_inbox_size && net_inbox[ used ] != 'T' && net_inbox[ used ] != 'C' )
        {
            fprintf( stderr, "native input: unknown message kind 0x%02x (%u bytes waiting, %u used)\n", net_inbox[ used ], net_inbox_size, used );
            net_lost( "the other copy sent something unexpected" );
            return;
        }
        memcpy( net_inbox, net_inbox + used, net_inbox_size - used );      // overlapping ranges move downwards only
        net_inbox_size -= used;
        if ( n <= 0 ) return;
    }
}

static int net_poll( int ( *real_poll )( void * ), void *event )
{
    unsigned int tick = TICK;
    event_player = -1;                  // the game has finished with the previous event
    if ( net_ended_why )                // the link is gone; the menu module ends the session on this pass
    {
        while ( real_poll && real_poll( event ) )
            if ( *(unsigned int *)event == SDL_QUIT ) return 1;
        return 0;
    }
    unsigned char mine[ EVENT_SIZE ];
    // this copy's devices (or the test recording standing in for them)
    while ( real_poll && real_poll( mine ) )
    {
        if ( *(unsigned int *)mine == SDL_QUIT ) { window_closing = 1; memcpy( event, mine, EVENT_SIZE ); return 1; }
        if ( net_fake == 0 ) net_gather( mine );
    }
    while ( net_fake_have && net_fake_tick <= tick )
    {
        if ( net_fake_tick != QUERY_RECORD ) net_gather( net_fake_event );
        net_fake_have = fread( &net_fake_tick, 4, 1, net_fake ) == 1 && fread( net_fake_event, EVENT_SIZE, 1, net_fake ) == 1;
    }
    // everything gathered for steps that can no longer receive input goes out
    while ( net_sent_through < tick + NET_DELAY )
    {
        net_bucket *bucket = &net_mine[ net_sent_through % NET_RING ];
        unsigned char head[ 9 ] = { 'T' };
        memcpy( head + 1, &net_sent_through, 4 );
        memcpy( head + 5, &bucket->count, 4 );
        if ( !net_link_send( head, 9 ) || ( bucket->count && !net_link_send( bucket->events, (int)( bucket->count * EVENT_SIZE ) ) ) )
        {
            net_lost( "the other player has left" );
            return 0;
        }
        net_sent_through++;
        net_mine[ ( net_sent_through + NET_DELAY - 1 ) % NET_RING ].count = 0;     // the bucket that opens next
    }
    if ( one_view() && view_base_width && GAME_VIEWS == 1 && GAME_WIDTH != view_base_width )
        GAME_WIDTH = view_base_width;           // the game went back to a single view (title menu)
    net_receive();
    if ( net_ended_why )
        return 0;
    if ( tick % 600 == 0 && net_delivered_through < (int)tick )
        fprintf( stderr, "native input: step %u, events sent %u, received %u\n", tick, net_gathered, net_received );
    // the game's input for this step: the host's events, then the joiner's
    if ( net_delivered_through < (int)tick && tick < net_sent_through && tick < net_have_through )
    {
        net_bucket *first = net_guest ? &net_theirs[ tick % NET_RING ] : &net_mine[ tick % NET_RING ];
        net_bucket *second = net_guest ? &net_mine[ tick % NET_RING ] : &net_theirs[ tick % NET_RING ];
        // The game opens its controller when SDL announces one (event 0x653), not at start-up. Both copies
        // are told so once, on the same step, before any other input: that is player 2's controller.
        if ( tick >= NET_DELAY && !announced )
        {
            announced = 1;
            memset( event, 0, EVENT_SIZE );
            *(unsigned int *)event = 0x653;
            return 1;
        }
        if ( tick >= NET_DELAY && net_deliver_position < first->count + second->count )
        {
            const unsigned char *next = net_deliver_position < first->count ? first->events[ net_deliver_position ]
                                                                           : second->events[ net_deliver_position - first->count ];
            event_player = net_deliver_position < first->count ? 0 : 1;        // the host is player 1, the joiner player 2
            event_is_pad = *(const unsigned int *)next >= 0x650;
            memcpy( event, next, EVENT_SIZE );
            apply( next );
            net_deliver_position++;
            return 1;
        }
        net_delivered_through = (int)tick;
        net_deliver_position = 0;
        if ( tick % NET_CHECK == 0 )                    // both copies are at the same point of the same step here
        {
            unsigned char message[ 9 ] = { 'C' };
            unsigned int slot = ( tick / NET_CHECK ) % 64, hash = net_state_hash();
            if ( getenv( "ILB_NET_DEBUG" ) )
            {
                char *units = *(char **)ILB_ADDR( 0x3A73C9C );
                int by_type[ 8 ] = { 0 };
                for ( int i = 0; units && i < 0x4000; i++ )
                {
                    struct unit *u = (struct unit *)( units + i * 8936 );
                    if ( u->id != (dword)-1 && ( u->id & 0x3FFF ) == (dword)i && u->unit_type >= 0 && u->unit_type < 8 ) by_type[ u->unit_type ]++;
                }
                for ( int i = 0; units && i < 0x4000; i++ )
                {
                    struct unit *u = (struct unit *)( units + i * 8936 );
                    if ( u->id == (dword)-1 || ( u->id & 0x3FFF ) != (dword)i || u->unit_type != 0 ) continue;
                    fprintf( stderr, "native input:   player unit %x class %d level %d at %.3f,%.3f forces %.3f,%.3f data", u->id, u->unit_class, u->level,
                             u->position.x, u->position.y, u->forces.x, u->forces.y );
                    for ( int k = 0; k < 64; k += 4 ) fprintf( stderr, " %x", *(unsigned int *)&u->custom_data[ k ] );
                    fprintf( stderr, "\n" );
                }
                fprintf( stderr, "native input:   cursors %d,%d and %d,%d; mouse scale %.3f,%.3f; game width %d height %d views %d; client devices %d %d; pad handle %x present %d\n",
                         *(int *)ILB_ADDR( 0x651C88 ), *(int *)ILB_ADDR( 0x651C8C ), *(int *)ILB_ADDR( 0x651C90 ), *(int *)ILB_ADDR( 0x651C94 ),
                         *(float *)ILB_ADDR( 0x652148 ), *(float *)ILB_ADDR( 0x65214C ), *(int *)ILB_ADDR( 0x652138 ), *(int *)ILB_ADDR( 0x65213C ),
                         *(int *)ILB_ADDR( 0x6113F4 ), *(int *)ILB_ADDR( 0x60F2C8 ), *(int *)ILB_ADDR( 0x60F2F8 ), *(int *)ILB_ADDR( 0x16B4994 ), *(char *)ILB_ADDR( 0x651AD2 ) );
                fprintf( stderr, "native input: step %u state hash %08x (random state %08x; units: players %d monsters %d missiles %d items %d objects %d npcs %d other %d)\n",
                         tick, hash, ILB_native_rand_state(), by_type[ 0 ], by_type[ 1 ], by_type[ 2 ], by_type[ 3 ], by_type[ 4 ], by_type[ 5 ], by_type[ 6 ] + by_type[ 7 ] );
            }
            net_my_hash[ slot ] = hash;
            net_my_hash_step[ slot ] = tick + 1;
            memcpy( message + 1, &tick, 4 );
            memcpy( message + 5, &hash, 4 );
            if ( !net_link_send( message, 9 ) ) { net_lost( "the other player has left" ); return 0; }
            net_compare( tick );
        }
    }
    return 0;
}

// May the game take its next simulation step? (Asked by its own timing routine, 0x406D30.)
int ILB_input_tick_allowed( void )
{
    return mode != 3 || net_delivered_through >= (int)TICK;
}

// ---- one view per copy
// The game draws two players side by side, each in half of the window, and lays out every menu and panel for
// that half; the layout is part of the game state, so both copies must agree on it. For each copy to show
// only its own player at full size, both copies tell the game - from the moment it creates the two views
// until it goes back to one - that the window is twice as wide as it really is. Each view is then as large
// as a real window; the hosting copy shows the left one (player 1), the joining copy the right one.
// ILB_NET_SPLIT=1 switches this off (both copies then show the split screen, as local co-op does).

static int one_view( void )
{
    return mode == 3 && !getenv( "ILB_NET_SPLIT" );
}

// Called at the start of gge_texture_memory_rendering_surface_add: the game is about to make `views` views.
void ILB_input_views( unsigned int views )
{
    if ( !ILB_input_active() || !one_view() || views < 1 || views > 2 )
        return;
    if ( view_base_width == 0 )
        view_base_width = GAME_WIDTH;
    GAME_WIDTH = view_base_width * (int)views;
}

// How far left the picture is shifted when it goes to the real window, and the width the game believes in.
int ILB_input_full_views( void )    { return one_view(); }     // asked by the game's picture-scale routine (0x4610D0)
int ILB_input_view_offset( void )   { return one_view() && view_base_width && GAME_WIDTH > view_base_width && net_guest ? view_base_width : 0; }
int ILB_input_view_width( int real ) { return one_view() && view_base_width && GAME_WIDTH > view_base_width ? GAME_WIDTH : real; }

// ---- whose device
// Whose keyboard, mouse or controller the game is asking about: the player whose event it is handling, else
// the player it is working on (game_globals.client, 0x60F3C4, points into the client table at 0x60F2C0).
static int state_player( void )
{
    char *client;
    if ( mode != 3 )
        return 0;
    if ( event_player >= 0 )
        return event_player;
    client = *(char **)ILB_ADDR( 0x60F3C4 );
    if ( client >= (char *)ILB_ADDR( 0x60F2C0 ) && client < (char *)ILB_ADDR( 0x60F2C0 ) + 48 * PLAYERS )
        return (int)( client - (char *)ILB_ADDR( 0x60F2C0 ) ) / 48;
    return 0;
}

// Called at the start of the game's two "which player gets this event" routines (0x465B00 for keyboard and
// mouse, 0x465B40 for the controller). The original picks the first player set up for that kind of device:
// one keyboard player and one controller player, fixed when the game starts. In a network game the event
// goes to the player whose copy it came from, and that player's device setting (1 keyboard and mouse,
// 2 controller; the third field of the client record) follows whatever they last used - so either player
// can play with either, and change in the middle. Returns the player's client record (also made the
// current one), or 0 to let the game decide.
int *ILB_input_select_client( void )
{
    int *client;
    if ( mode != 3 || event_player < 0 )
        return 0;
    client = (int *)ILB_ADDR( 0x60F2C0 + 48 * event_player );
    if ( client[ 0 ] == -1 )            // that player is not in the game (yet)
        return 0;
    client[ 2 ] = event_is_pad ? 2 : 1;
    *(int **)ILB_ADDR( 0x60F3C4 ) = client;
    return client;
}

int ILB_input_net_session( void )                           { return ILB_input_active() && mode == 3; }

// ---- the game as it was at start-up
// Two copies can only stay in step if they start from the same state, and a game that has been played is
// full of leftovers: which music was playing, where the unit table's search stands, timers, flags. A freshly
// started program has none of that. So that a host need not restart when somebody joins, each copy records
// its state at the moment it first reaches the title menu, and can be put back to exactly that moment:
//   - the game's global variables (the original program's .data section, about 55 MB, mostly zero);
//   - the memory the game allocated: it has a single allocator (drb_malloc / drb_free) which keeps its own
//     table of blocks in those globals. Blocks that exist at the recording are copied, and are never really
//     freed afterwards (ILB_game_free), so that the recorded pointers stay good; blocks made later are freed
//     when the state is put back;
//   - the OpenGL and sound objects that exist at the recording, kept alive the same way by the backend
//     (gge_gl_snapshot / gge_gl_restore) and the mixer (snd_mixer_snapshot).
// What is outside the game - the window, the sound device, this file's own variables - simply carries on.
#define GLOBALS_START   0x5C8000
#define GLOBALS_END     0x3A7C000
#define BLOCK_TABLE     0x17044F8                   // 0x2000 entries of 4120 bytes: free flag at +0,
#define BLOCK_ENTRY     4120                        // size at +0x1010, address at +0x1014 (see drb_malloc)
#define BLOCK_COUNT     0x2000
void *malloc( unsigned int );
void free( void * );
void gge_gl_snapshot( void );
void gge_gl_restore( void );
__attribute__(( weak )) void snd_mixer_snapshot( void ) {}
__attribute__(( weak )) void snd_mixer_forget_all( void ) {}     // the real ones are in port/snd_mixer.c (builds with sound)
static unsigned char *start_globals;
static struct { unsigned char *address, *contents; unsigned int size; int freed_by_game; } start_blocks[ BLOCK_COUNT ];
static int start_block_count;
static void net_epoch( int guest, unsigned int agreed_seed );
static int quit_player = -1, quit_kind;     // who chose to leave the session (ILB_input_net_quit)

void *ILB_game_malloc( unsigned int size )
{
    return malloc( size );
}

void ILB_game_free( void *block )
{
    for ( int i = 0; i < start_block_count; i++ )
        if ( start_blocks[ i ].address == block )
        {
            start_blocks[ i ].freed_by_game = 1;        // kept: the start state points at it
            return;
        }
    free( block );
}

static unsigned char *block_address( int i )   { return *(unsigned char **)ILB_ADDR( BLOCK_TABLE + BLOCK_ENTRY * i + 0x1014 ); }
static unsigned int block_size( int i )        { return *(unsigned int *)ILB_ADDR( BLOCK_TABLE + BLOCK_ENTRY * i + 0x1010 ); }
static int block_in_use( int i )               { return *(unsigned char *)ILB_ADDR( BLOCK_TABLE + BLOCK_ENTRY * i ) == 0 && block_address( i ) != 0; }

// Called at the start of the title menu's set-up routine (0x493AB0). The first time: a joining copy that
// was started for a session begins the session here; any other copy records its start state.
void ILB_input_snapshot_start_state( void )
{
    unsigned int total = 0;
    if ( mode < 0 ) start();
    if ( net_epoch_pending )
    {
        net_epoch_pending = 0;
        net_epoch( 1, seed );
        return;
    }
    if ( start_globals != 0 || getenv( "ILB_NET_FD" ) != 0 || ( start_globals = malloc( GLOBALS_END - GLOBALS_START ) ) == 0 )
        return;
    memcpy( start_globals, ILB_ADDR( GLOBALS_START ), GLOBALS_END - GLOBALS_START );
    for ( int i = 0; i < BLOCK_COUNT; i++ )
        if ( block_in_use( i ) && ( start_blocks[ start_block_count ].contents = malloc( block_size( i ) ? block_size( i ) : 1 ) ) != 0 )
        {
            start_blocks[ start_block_count ].address = block_address( i );
            start_blocks[ start_block_count ].size = block_size( i );
            memcpy( start_blocks[ start_block_count ].contents, block_address( i ), block_size( i ) );
            total += block_size( i );
            start_block_count++;
        }
    gge_gl_snapshot();
    snd_mixer_snapshot();
    fprintf( stderr, "native input: start state recorded (%d memory blocks, %u MB)\n", start_block_count, total >> 20 );
}

// Back to the recorded moment. The caller has already let the game leave the screen it was in.
static int restore_start_state( void )
{
    if ( start_globals == 0 )
        return 0;
    for ( int i = 0; i < BLOCK_COUNT; i++ )            // what the game allocated since and still holds
        if ( block_in_use( i ) )
        {
            int from_start = 0;
            for ( int k = 0; k < start_block_count; k++ ) from_start |= start_blocks[ k ].address == block_address( i );
            if ( !from_start ) free( block_address( i ) );
        }
    gge_gl_restore();
    snd_mixer_forget_all();
    memcpy( ILB_ADDR( GLOBALS_START ), start_globals, GLOBALS_END - GLOBALS_START );
    for ( int k = 0; k < start_block_count; k++ )
    {
        memcpy( start_blocks[ k ].address, start_blocks[ k ].contents, start_blocks[ k ].size );
        start_blocks[ k ].freed_by_game = 0;
    }
    return 1;
}

static void leave_current_screen( void )       // as the game itself changes screens (game_return_to_mainmenu)
{
    void ( *leave )( void );
    int *client = *(int **)ILB_ADDR( 0x60F3C4 );
    if ( client ) client[ 3 ] = 0;
    leave = *(void ( ** )( void ))ILB_ADDR( 0x609914 + 48 * *(int *)ILB_ADDR( 0x3A73CC0 ) );
    if ( leave ) leave();
}

static void enter_title_menu( void )
{
    *(unsigned char *)ILB_ADDR( 0x5D40DC + 3 ) = 1;     // running, not paused (a window that is not in front pauses itself)
    *(int *)ILB_ADDR( 0x3A73CC0 ) = 2;
    *(int *)ILB_ADDR( 0x609908 ) = -1;
    ( *(void ( ** )( int ))ILB_ADDR( 0x609910 + 48 * 2 ) )( 0 );
}

// ---- a two-player session
// Both copies begin a session at the same point - the start of the title menu's set-up, in the start state -
// and from there the scripted two-player start runs on both (draft/native/native_main.c). The joining copy
// gets there by being started fresh for the session; the host by being put back to its start state inside
// the running program, with its picture frozen meanwhile, so that the host sees no restart.
void ILB_native_set_seed( unsigned int seed );
void ILB_native_script_restart( void );
int setenv( const char *, const char *, int );
int unsetenv( const char * );
static int real_pad_handle;

static void net_epoch( int guest, unsigned int agreed_seed )
{
    snd_mixer_forget_all();
    *(unsigned char *)ILB_ADDR( 0x5D40DC + 3 ) = 1;
    TICK = 0;
    seed = agreed_seed;
    have_seed = 1;
    ILB_native_set_seed( agreed_seed );
    // the game's one controller: in a session it is a stand-in on both copies (see the wrappers in
    // native_platform.c); this copy's real one, if the game had it open, is kept for afterwards
    real_pad_handle = *(int *)ILB_ADDR( 0x16B4994 );
    *(int *)ILB_ADDR( 0x16B4994 ) = 0;
    *(char *)ILB_ADDR( 0x651AD2 ) = 0;
    // where each player's cursor is: it was left wherever this copy's mouse last was, and a character faces
    // its cursor. Both copies start from "top left, outside the window" until a mouse event says otherwise.
    for ( int player = 0; player < 4; player++ )
    {
        *(int *)ILB_ADDR( 0x651C88 + 8 * player ) = 0;
        *(int *)ILB_ADDR( 0x651C8C + 8 * player ) = 0;
    }
    *(char *)ILB_ADDR( 0x16B4D98 ) = 0;
    memset( all_mouse_x, 0, sizeof all_mouse_x );
    memset( all_mouse_y, 0, sizeof all_mouse_y );
    memset( net_mine, 0, sizeof net_mine );
    memset( net_theirs, 0, sizeof net_theirs );
    memset( all_keys, 0, sizeof all_keys );
    memset( all_mouse_buttons, 0, sizeof all_mouse_buttons );
    memset( all_pad_axis, 0, sizeof all_pad_axis );
    memset( net_my_hash_step, 0, sizeof net_my_hash_step );
    memset( net_their_hash_step, 0, sizeof net_their_hash_step );
    net_sent_through = net_have_through = NET_DELAY;
    net_delivered_through = -1;
    net_deliver_position = net_inbox_size = net_gathered = net_received = 0;
    net_out_of_step = announced = 0;
    net_ended_why = 0;
    quit_player = -1;
    quit_kind = 0;
    event_player = -1;
    net_guest = guest;
    mode = 3;
    setenv( "ILB_SCRIPT", "coop", 1 );
    setenv( "ILB_SLOT", "1", 1 );
    setenv( "ILB_SLOT2", "2", 1 );
    ILB_native_script_restart();
    fprintf( stderr, "native input: two-player session begins as %s (seed %u)\n", guest ? "player 2" : "player 1", agreed_seed );
}

// Leaving the screen the game is in, the way the game itself does when it changes screens. Leaving a game
// SAVES it, into the folder the program is in at that moment - so the callers do this before they change
// folders: a host leaves its own game in its own folder, and a session is left in the session folder.
void ILB_input_leave_screen( void )
{
    leave_current_screen();
}

// Inside the running program, for the host and for a joiner alike. Returns 0 if it cannot be done here and
// the copy has to be started afresh instead: no recorded start state, or (a joiner) a window of another size
// than the host's - the size of the picture is part of what the game computes from.
int ILB_input_net_begin( int guest, unsigned int agreed_seed )
{
    if ( mode < 0 ) start();
    if ( start_globals == 0 )
        return 0;
    restore_start_state();              // (the caller has left the screen this copy was in: ILB_input_leave_screen)
    appdata_file_load();                // the settings file of the session folder, which is the host's
    if ( guest && ( *(int *)ILB_ADDR( 0x60A030 ) != GAME_WIDTH || *(int *)ILB_ADDR( 0x60A034 ) != *(int *)ILB_ADDR( 0x65213C ) ) )
    {
        fprintf( stderr, "native input: the host plays at %dx%d and this window is %dx%d; starting afresh at the host's size\n",
                 *(int *)ILB_ADDR( 0x60A030 ), *(int *)ILB_ADDR( 0x60A034 ), GAME_WIDTH, *(int *)ILB_ADDR( 0x65213C ) );
        return 0;
    }
    net_in_place = 1;
    net_epoch( guest, agreed_seed );
    enter_title_menu();
    return 1;
}

// The session is over (the other player left, or the game went back to the title menu): this copy is an
// ordinary single game again, at the title menu of its start state. Returns 1 if a session was ended.
int ILB_input_net_end( void )
{
    if ( mode != 3 || !net_in_place )
        return 0;
    mode = 0;
    net_in_place = 0;
    event_player = -1;
    unsetenv( "ILB_SCRIPT" );
    restore_start_state();              // (the caller has left the game's screen while still in the session folder)
    appdata_file_load();                // this copy's own settings again (the caller is back in its own folder)
    ILB_native_script_restart();
    enter_title_menu();
    return 1;
}

int ILB_input_net_in_place( void )                          { return mode == 3 && net_in_place; }
int ILB_input_net_out_of_step( void )                       { return net_out_of_step; }

// For port/mod_net_menu.c: has an in-place session just ended by itself? Returns the reason once, else 0.
const char *ILB_input_net_ended( void )
{
    const char *why = net_ended_why;
    net_ended_why = 0;
    return why;
}

// ---- one player leaving
// Called at the start of the game's two ways out of a game: back to the main menu (kind 1) and exit to the
// desktop (kind 2). In the original these end the game for everybody playing on that machine - and in a
// session both copies carry out both players' input, so one player picking "quit" in the pause menu would
// end it for both. In a session the game's own action is therefore not carried out (the two copies stay
// identical up to this step); it is recorded who is leaving, and port/mod_net_menu.c takes this copy out of
// the session or lets the other player carry on. Returns 0 when the original should run: no session, or
// this copy's window is being closed (which only ends this copy; the other notices the lost connection).


int ILB_input_net_quit( int kind )
{
    int player = event_player;
    if ( mode != 3 || !net_in_place || window_closing )
        return 0;
    if ( quit_player >= 0 )
        return 1;                       // already leaving
    if ( player < 0 )
        player = state_player();        // not reached from an input event: the player the game is working on
    quit_player = player;
    quit_kind = kind;
    net_ended_why = player == net_guest ? "this player has left the game" : "the other player has left the game";
    fprintf( stderr, "native input: player %d chose to %s at step %u\n", player + 1, kind == 2 ? "exit" : "go back to the main menu", TICK );
    return 1;
}

// For port/mod_net_menu.c when a session has ended: 0 nobody chose to leave (the connection was lost),
// 1 this copy's player left, 2 the other player left; *kind = 1 main menu, 2 exit. Asking clears it.
int ILB_input_net_leaver( int *kind )
{
    int who = quit_player < 0 ? 0 : quit_player == net_guest ? 1 : 2;
    if ( kind ) *kind = quit_kind;
    quit_player = -1;
    quit_kind = 0;
    return who;
}

// ILB_TEST_QUIT="<player 1|2> <kind 1|2> <step>": both copies of a test session make that player quit on
// that step, the way the pause menu would (draft/native/native_main.c calls this once per step).
void ILB_input_test_quit( void )
{
    const char *plan = getenv( "ILB_TEST_QUIT" );
    int player = 0, kind = 0, step = 0;
    int sscanf( const char *, const char *, ... );
    if ( plan == 0 || mode != 3 || quit_player >= 0 || sscanf( plan, "%d %d %d", &player, &kind, &step ) != 3
         || (int)TICK != step || *(int *)ILB_ADDR( 0x3A73CC0 ) != 11 )
        return;
    event_player = player - 1;
    if ( kind == 2 ) game_force_shutdown(); else game_return_to_mainmenu();
    event_player = -1;
}
