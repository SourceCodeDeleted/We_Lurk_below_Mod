// mod_net_menu.c - "Single player / Local multiplayer / Host a game / Join a game" after Play, and the
// screens for hosting and joining through the lobby server (hand-written, NOT part of the original game).
//
// To remove: configure with -DILB_MOD_NET_MENU=OFF (and delete this file if wanted). The only references to
// it are guarded calls at the start of mm_do_play (the Play button) and cs_do_select (a character picked),
// placed by tools/draft_fix.py (ENTRY_HOOKS).
//
// The screens live in the game's app mode 16, which the original leaves empty, and are drawn with the
// game's own font over its menu background, the way its "Player 2 begin selecting" screen (mode 14) is.
//
//   Play -> Game type:  Single player        the original behaviour
//                       Local multiplayer    the game's own two-player set-up screen (mode 8)
//                       Host a game          pick a character -> game name, optional password -> play
//                       Join a game          pick a character -> list of games -> (password) -> join
//
// A host does not wait on a screen: the game starts with the chosen character right away, alone, while the
// game stays on the server's list. When somebody joins, the host's game is saved, the files are exchanged
// and both copies restart into the two-player game (a pause of a few seconds for the host). When the other
// player leaves again, the host carries on alone and the game goes back on the list.
//
// Input works as in the game's own menus: the mouse highlights the line under it and a click picks it; a
// controller's stick or d-pad moves the highlight, A picks, B goes back; the keyboard does the same with
// Up/Down, Enter and Escape. The two text fields are typed on the keyboard (Tab or Up/Down changes field).
// Characters are picked on the game's own character screen. When two players are matched, the files are
// exchanged and this copy restarts itself into the synchronised game (port/net/net_session.c).
// The server's address: environment variable ILB_LOBBY, else the first line of lobby.txt next to the pack
// file, else this machine.
#ifdef ILB_MOD_NET_MENU
#include "ILB_draft_decls.h"

typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );
int snprintf( char *, unsigned int, const char *, ... );
FILE *fopen( const char *, const char * );
int fclose( FILE * );
unsigned int strlen( const char * );
char *strcpy( char *, const char * );

// port/net/ (built with the system headers; see there)
typedef struct { int id, locked, age; char name[ 100 ], host[ 100 ], salt[ 20 ]; } lobby_game;
int lobby_connect( const char *address, const char *player, char *error, int error_size );
void lobby_disconnect( void );
int lobby_host( const char *name, const char *password, char *error, int error_size );
int lobby_wait( char *joiner, int joiner_size );
int lobby_list( lobby_game *games, int max, char *error, int error_size );
int lobby_join( const lobby_game *game, const char *password, char *error, int error_size );
const char *net_session_lobby_address( void );
const char *net_session_player_name( void );
void net_session_root( int is_host, char *root, int size );
int net_session_prepare( int is_host, int slot, const char *session_root, unsigned int *seed, char *error, int error_size );
void net_session_run( int is_host, const char *session_root, unsigned int seed, char **argv );
void net_session_remember_host( const char *name, const char *password, int slot );
void net_session_enter( const char *session_root );
void net_session_leave( int in_step );
int ILB_input_net_begin( int guest, unsigned int agreed_seed );
__attribute__(( weak )) void ILB_native_freeze_picture( int on ) {}     // the real one: draft/native/native_platform.c (builds with a window)
int ILB_input_net_end( void );
void ILB_input_leave_screen( void );
int ILB_input_net_in_place( void );
int ILB_input_net_out_of_step( void );
const char *ILB_input_net_ended( void );
int ILB_input_net_leaver( int *kind );
char *getenv( const char * );
int atoi( const char * );
extern char **ILB_argv;

#define APP_MODE            ( *(int *)ILB_ADDR( 0x3A73CC0 ) )
#define APP_MODE_WANTED     ( *(int *)ILB_ADDR( 0x609908 ) )
#define MODE_TITLE          2
#define MODE_CHARACTERS     6
#define MODE_LOCAL_SETUP    8
#define MODE_NET_MENU       16                                      // unused by the original
#define MODE_HANDLERS( m )  ( (void **)ILB_ADDR( 0x609910 + 48 * ( m ) ) )  // init, exit, update, mouse move, left button,
                                                                    // right button, wheel, key down, key up, character, ...
#define WINDOW_WIDTH        ( *(int *)ILB_ADDR( 0x652138 ) )
#define WINDOW_HEIGHT       ( *(int *)ILB_ADDR( 0x65213C ) )
#define KEY_UP              0x40000052
#define KEY_DOWN            0x40000051
#define KEY_ENTER           13
#define KEY_ESCAPE          27
#define KEY_BACKSPACE       8
#define KEY_TAB             9
#define WHITE               ( -1 )
#define YELLOW              ( (int)0xFFFFFF00 )
#define GREY                ( (int)0xFF9A9A9A )
#define RED                 ( (int)0xFFFF6060 )

enum { SCREEN_TYPE, SCREEN_HOST_FORM, SCREEN_HOST_WAIT, SCREEN_LIST, SCREEN_PASSWORD, SCREEN_MESSAGE };
enum { WANT_NOTHING, WANT_HOST, WANT_JOIN };

static int screen, cursor, want, slot, bypass_play, connected;
static int hosting_while_playing, joiner_arrived, autohost_done;
static int session_is_host, session_reached_game;
static char notice[ 100 ];
static int notice_wait;
static char game_name[ 33 ], password[ 33 ], message[ 200 ], other_player[ 100 ];
static lobby_game games[ 32 ];
static int game_count, chosen_game;

// ---------------------------------------------------------------------------------------- drawing
static float text_width( const char *text, float scale )
{
    int n = 0;
    for ( ; text[ n ] && n < 250; n++ ) stru_60B960.text[ n ] = (unsigned char)text[ n ];
    stru_60B960.text[ n ] = 0;
    stru_60B960.length = n;
    return ui_font_pixel_strlen( &stru_60B960, scale ).m128_f32[ 0 ];
}

static void text_at( const char *text, float x, float y, float scale, int color )       // x < 0: centred
{
    float width = text_width( text, scale );
    if ( x < 0 ) x = ( (float)WINDOW_WIDTH - width ) * 0.5f;
    ui_font_draw_string( color, &stru_60B960.length, (_OWORD)*(unsigned int *)&x, *(int *)&y, *(unsigned int *)&scale, 1 );
}

static float line_y( int line )     { return (float)WINDOW_HEIGHT * 0.30f + (float)line * (float)WINDOW_HEIGHT * 0.055f; }
static float big( void )            { return flt_61143C + 1.0f; }
static float normal( void )         { return flt_61143C + 0.6f; }

static void title( const char *text, const char *hint )
{
    text_at( text, -1, (float)WINDOW_HEIGHT * 0.16f, big(), YELLOW );
    if ( hint ) text_at( hint, -1, (float)WINDOW_HEIGHT * 0.90f, normal(), GREY );
}

// The lines that can be picked, as last drawn: where the mouse has to be to mean each of them.
#define MAX_LINES 40
static struct { float left, right, top, bottom; } line_area[ MAX_LINES ];
static int lines_drawn;

static void option( int line, const char *text )
{
    float scale = normal(), width = text_width( text, scale ), y = line_y( line );
    if ( line >= 0 && line < MAX_LINES )
    {
        line_area[ line ].left = ( (float)WINDOW_WIDTH - width ) * 0.5f - (float)WINDOW_HEIGHT * 0.02f;
        line_area[ line ].right = ( (float)WINDOW_WIDTH + width ) * 0.5f + (float)WINDOW_HEIGHT * 0.02f;
        line_area[ line ].top = y - (float)WINDOW_HEIGHT * 0.012f;
        line_area[ line ].bottom = y + (float)WINDOW_HEIGHT * 0.043f;
        if ( line + 1 > lines_drawn ) lines_drawn = line + 1;
    }
    text_at( text, -1, y, scale, cursor == line ? YELLOW : WHITE );     // the highlighted line is yellow, as in the game's menus
}

// ---------------------------------------------------------------------------------------- what the choices do
static void show( int which )       { screen = which; cursor = 0; }
static void say( const char *text ) { snprintf( message, sizeof message, "%s", text ); show( SCREEN_MESSAGE ); }

static void leave_lobby( void )
{
    if ( connected ) lobby_disconnect();
    connected = 0;
    hosting_while_playing = joiner_arrived = 0;
}

static int reach_lobby( void )
{
    char error[ 160 ];
    if ( connected ) return 1;
    if ( !lobby_connect( net_session_lobby_address(), net_session_player_name(), error, sizeof error ) )
    {
        say( error );
        return 0;
    }
    return connected = 1;
}

static void refresh_list( void )
{
    char error[ 160 ];
    if ( !reach_lobby() ) return;
    game_count = lobby_list( games, 32, error, sizeof error );
    if ( game_count < 0 )
    {
        leave_lobby();
        say( error );
        return;
    }
    show( SCREEN_LIST );
}

// The two players are matched: exchange the files and restart into the game. Returns only on failure.
static void start_session( int is_host )
{
    char error[ 200 ], root[ 1024 ];
    unsigned int seed = 0;
    net_session_root( is_host, root, sizeof root );
    if ( net_session_prepare( is_host, slot, root, &seed, error, sizeof error ) )
    {
        if ( getenv( "ILB_NET_RESTART" ) == 0 )
        {
            // Neither copy restarts. The game is put back to its start state inside the running program and
            // loads the session's saves (draft/native/native_input.c). The host's picture is frozen until its
            // game is back, so the host sees neither menus nor a loading screen; the joiner's only until its
            // loading screen, which it does see.
            ILB_native_freeze_picture( is_host ? 1 : 2 );
            ILB_input_leave_screen();           // a host's own game is saved by this, in its own folder
            net_session_enter( root );
            if ( ILB_input_net_begin( !is_host, seed ) )
            {
                session_is_host = is_host;
                session_reached_game = 0;
                hosting_while_playing = joiner_arrived = 0;
                want = WANT_NOTHING;            // the character screen is the game's own again
                connected = 0;                  // the connection is the game's now
                return;
            }
            ILB_native_freeze_picture( 0 );     // not possible here: a fresh run of the program after all
        }
        net_session_run( is_host, root, seed, ILB_argv );   // a fresh run of the program, straight into the session
        snprintf( error, sizeof error, "could not start the game" );
    }
    connected = 0;                      // the connection was the game's; it is of no use any more
    lobby_disconnect();
    say( error );
}

static void join_chosen( void )
{
    char error[ 160 ];
    if ( !lobby_join( &games[ chosen_game ], password, error, sizeof error ) )
    {
        say( error );                   // wrong password, game gone, ...: still connected, back to the list afterwards
        return;
    }
    start_session( 0 );
}

static void choose( void )
{
    char error[ 160 ];
    switch ( screen )
    {
        case SCREEN_TYPE:
            if ( cursor == 0 )          // single player: what Play always did
            {
                bypass_play = 1;
                mm_do_play( 100 );
                bypass_play = 0;
            }
            else if ( cursor == 1 )
                APP_MODE_WANTED = MODE_LOCAL_SETUP;
            else if ( cursor == 2 || cursor == 3 )
            {
                want = cursor == 2 ? WANT_HOST : WANT_JOIN;
                APP_MODE_WANTED = MODE_CHARACTERS;      // comes back through ILB_mod_net_menu_character
            }
            else
                APP_MODE_WANTED = MODE_TITLE;
            break;
        case SCREEN_HOST_FORM:
            if ( cursor < 2 ) { cursor++; break; }
            if ( cursor == 3 ) { show( SCREEN_TYPE ); break; }
            if ( game_name[ 0 ] == 0 ) { cursor = 0; break; }
            if ( !reach_lobby() ) break;
            if ( !lobby_host( game_name, password, error, sizeof error ) )
            {
                leave_lobby();
                say( error );
                break;
            }
            // on the list: now play, alone for the time being (ILB_mod_net_menu_background watches for a joiner)
            net_session_remember_host( game_name, password, slot );
            hosting_while_playing = 1;
            joiner_arrived = 0;
            want = WANT_NOTHING;
            sub_42BCC0();                       // the character screen's own set-up, then "this slot was picked"
            cs_do_select( 99 + slot );
            break;
        case SCREEN_HOST_WAIT:          // the only choice is to cancel
            leave_lobby();
            show( SCREEN_TYPE );
            break;
        case SCREEN_LIST:
            if ( cursor < game_count )
            {
                chosen_game = cursor;
                password[ 0 ] = 0;
                if ( games[ chosen_game ].locked ) show( SCREEN_PASSWORD );
                else join_chosen();
            }
            else if ( cursor == game_count ) refresh_list();
            else { leave_lobby(); show( SCREEN_TYPE ); }
            break;
        case SCREEN_PASSWORD:
            if ( cursor == 1 ) show( SCREEN_LIST );
            else join_chosen();
            break;
        case SCREEN_MESSAGE:
            if ( connected && want == WANT_JOIN ) refresh_list();
            else show( SCREEN_TYPE );
            break;
    }
}

static int lines_on_screen( void )
{
    switch ( screen )
    {
        case SCREEN_TYPE:       return 5;
        case SCREEN_HOST_FORM:  return 4;
        case SCREEN_LIST:       return game_count + 2;
        case SCREEN_PASSWORD:   return 2;
        default:                return 1;
    }
}

static char *field_being_typed( void )
{
    if ( screen == SCREEN_HOST_FORM && cursor == 0 ) return game_name;
    if ( screen == SCREEN_HOST_FORM && cursor == 1 ) return password;
    if ( screen == SCREEN_PASSWORD && cursor == 0 ) return password;
    return 0;
}

// ---------------------------------------------------------------------------------------- the mode's handlers
static void on_enter_mode( void )
{
    sub_495D10();                       // the same set-up the game's "next player" text screen uses
}

static void on_leave_mode( void )
{
    sub_495D90();
}

static void go_back( void );
static void move_highlight( int line );

static void on_key_down( int key )
{
    int lines = lines_on_screen();
    char *field = field_being_typed();
    if ( key == KEY_UP ) move_highlight( ( cursor + lines - 1 ) % lines );
    else if ( key == KEY_DOWN || key == KEY_TAB ) move_highlight( ( cursor + 1 ) % lines );
    else if ( key == KEY_ENTER ) { channel_play( 1, 1.0f ); choose(); }
    else if ( key == KEY_BACKSPACE && field && field[ 0 ] ) field[ strlen( field ) - 1 ] = 0;
    else if ( key == KEY_ESCAPE ) go_back();
}

static void on_character( int key )     // called after on_key_down for the same press, with Shift applied
{
    char *field = field_being_typed();
    unsigned int length;
    if ( field == 0 || key < 32 || key > 126 ) return;
    length = strlen( field );
    if ( length == 0 && key == ' ' ) return;        // the server refuses names that start with a space
    if ( length < 32 ) { field[ length ] = (char)key; field[ length + 1 ] = 0; }
}

static int line_under( int x, int y )
{
    for ( int i = 0; i < lines_drawn && i < MAX_LINES; i++ )
        if ( (float)x >= line_area[ i ].left && (float)x < line_area[ i ].right && (float)y >= line_area[ i ].top && (float)y < line_area[ i ].bottom )
            return i;
    return -1;
}

static int mouse_line = -1;             // the line the mouse is over, if any

static void move_highlight( int line )
{
    if ( line != cursor ) channel_play( 1, 1.0f );      // the tick the game's menus make
    cursor = line;
}

static void on_mouse_move( int x, int y, int modifiers )
{
    sub_42A680( x, y );                 // what the game's own menus do with the mouse: this moves the pointer on the screen
    mouse_line = line_under( x, y );
    if ( mouse_line >= 0 ) move_highlight( mouse_line );
}

static void on_left_button( int down, int modifiers )
{
    if ( down && mouse_line >= 0 )      // a click picks the line it is on, and nothing when it is on none
    {
        cursor = mouse_line;
        channel_play( 1, 1.0f );
        choose();
    }
}

static void go_back( void )
{
    if ( screen == SCREEN_TYPE ) APP_MODE_WANTED = MODE_TITLE;
    else if ( screen == SCREEN_PASSWORD ) show( SCREEN_LIST );
    else { leave_lobby(); show( SCREEN_TYPE ); }
}

// A controller, as SDL numbers its buttons and sticks: A 0, B 1, d-pad up 11 and down 12; left stick's
// vertical axis 1. The stick moves the highlight once per push.
static void on_controller_button( int down, int button )
{
    int lines = lines_on_screen();
    if ( !down ) return;
    if ( button == 11 ) move_highlight( ( cursor + lines - 1 ) % lines );
    else if ( button == 12 ) move_highlight( ( cursor + 1 ) % lines );
    else if ( button == 0 ) { channel_play( 1, 1.0f ); choose(); }
    else if ( button == 1 ) go_back();
}

static void on_controller_axis( int axis, int value )
{
    static int pushed;
    int lines = lines_on_screen();
    if ( axis != 1 ) return;
    if ( value > -12000 && value < 12000 ) { pushed = 0; return; }
    if ( pushed ) return;
    pushed = 1;
    move_highlight( value < 0 ? ( cursor + lines - 1 ) % lines : ( cursor + 1 ) % lines );
}

static void on_update( void )
{
    char line[ 220 ];
    if ( sub_406D30() != 0 )            // one simulation step is due: the game's menu housekeeping
    {
        sub_4609E0();
        sub_406CD0();
    }
    sub_54B250();
    sub_495BA0( 1 );
    lines_drawn = 0;
    switch ( screen )
    {
        case SCREEN_TYPE:
            title( "Play", 0 );
            option( 0, "Single player" );
            option( 1, "Local multiplayer" );
            option( 2, "Host a game" );
            option( 3, "Join a game" );
            option( 4, "Back" );
            break;
        case SCREEN_HOST_FORM:
            title( "Host a game", "Type the name and, if you want one, a password" );
            snprintf( line, sizeof line, "Game name: %s%s", game_name, cursor == 0 ? "_" : "" );
            option( 0, line );
            snprintf( line, sizeof line, "Password (optional): %s%s", password, cursor == 1 ? "_" : "" );
            option( 1, line );
            option( 2, "Start hosting" );
            option( 3, "Back" );
            break;
        case SCREEN_HOST_WAIT:
            title( "Host a game", "Enter or Escape to cancel" );
            snprintf( line, sizeof line, "\"%s\" is on the list%s", game_name, password[ 0 ] ? " (password protected)" : "" );
            text_at( line, -1, line_y( 0 ), normal(), WHITE );
            text_at( "Waiting for a player to join...", -1, line_y( 2 ), normal(), YELLOW );
            break;
        case SCREEN_LIST:
            title( "Join a game", 0 );
            if ( game_count == 0 )
                text_at( "Nobody is hosting a game right now.", -1, line_y( -1 ), normal(), GREY );
            for ( int i = 0; i < game_count; i++ )
            {
                snprintf( line, sizeof line, "%s%s  -  hosted by %s", games[ i ].name, games[ i ].locked ? "  [password]" : "", games[ i ].host );
                option( i, line );
            }
            option( game_count, "Refresh the list" );
            option( game_count + 1, "Back" );
            break;
        case SCREEN_PASSWORD:
            title( "Join a game", "Type the password and press Enter" );
            snprintf( line, sizeof line, "Password for %s: %s_", games[ chosen_game ].name, password );
            text_at( line, -1, line_y( -1 ), normal(), WHITE );
            option( 0, "Join" );
            option( 1, "Back" );
            break;
        case SCREEN_MESSAGE:
            title( "Play", 0 );
            text_at( message, -1, line_y( -1 ), normal(), RED );
            option( 0, "Continue" );
            break;
    }
    sub_54AAC0();
    ui_font_draw();
    gge_menu_render( (void *)(int)0xFF0C2030 );
}

static void install( void )
{
    void **handlers = MODE_HANDLERS( MODE_NET_MENU );
    if ( handlers[ 2 ] == (void *)on_update )
        return;
    handlers[ 0 ] = (void *)on_enter_mode;
    handlers[ 1 ] = (void *)on_leave_mode;
    handlers[ 2 ] = (void *)on_update;
    handlers[ 3 ] = (void *)on_mouse_move;
    handlers[ 4 ] = (void *)on_left_button;
    handlers[ 7 ] = (void *)on_key_down;
    handlers[ 9 ] = (void *)on_character;
    handlers[ 10 ] = (void *)on_controller_axis;
    handlers[ 11 ] = (void *)on_controller_button;
}

// ---------------------------------------------------------------------------------------- while playing
// Called on every pass of the game's main loop, in every screen (from draft/native/native_platform.c).
void ILB_mod_net_menu_background( void )
{
    // a copy that was restarted after a two-player session to carry on alone: host again, load the character
    if ( getenv( "ILB_NET_FD" ) )
        return;                                 // this run IS a two-player session; its connection is the game's
    if ( ILB_input_net_in_place() )
    {
        // a session running in this window: over when the other player is gone, or when the game has gone
        // back to its title menu (a player chose to quit, which takes both copies there on the same step)
        // A session running in this window is over when a player chose to leave from the pause menu, or
        // when the connection is gone (the other copy was closed, or the copies disagreed).
        const char *why = ILB_input_net_ended();
        int kind = 0, leaver, i_left;
        if ( why == 0 )
            return;
        leaver = ILB_input_net_leaver( &kind );                 // 0 connection lost, 1 this player, 2 the other
        i_left = leaver == 1;
        fprintf( stderr, "net menu: the two-player game is over (%s)\n", why );
        ILB_native_freeze_picture( !i_left && session_is_host );    // a host carrying on alone sees no menus
        ILB_input_leave_screen();                               // the session's game is saved by this, in the session folder
        net_session_leave( !ILB_input_net_out_of_step() );      // progress kept, back in this copy's own folder
        ILB_input_net_end();                                    // ... and its own start state and settings: the title menu
        if ( i_left )
        {
            if ( kind == 2 ) APP_MODE_WANTED = 12;              // "exit": the program's own way out, from the title menu
            return;
        }
        snprintf( notice, sizeof notice, "%s has left the game", getenv( session_is_host ? "ILB_NET_JOINER_NAME" : "ILB_NET_HOST_NAME" )
                                                               ? getenv( session_is_host ? "ILB_NET_JOINER_NAME" : "ILB_NET_HOST_NAME" ) : "The other player" );
        if ( !session_is_host )
        {
            say( notice );                                      // the host is gone: shown on the Play screen
            notice[ 0 ] = 0;
            install();
            want = WANT_NOTHING;
            APP_MODE_WANTED = MODE_NET_MENU;
            return;
        }
        // the host was left alone: back into its own game, and back on the list
        {
            char error[ 160 ];
            if ( reach_lobby() && lobby_host( game_name, password, error, sizeof error ) )
                hosting_while_playing = 1;
            else
                fprintf( stderr, "net menu: could not put the game back on the list; playing on alone\n" );
            want = WANT_NOTHING;
            sub_42BCC0();
            cs_do_select( 99 + slot );
        }
        return;
    }
    // a line on the screen for a host who is back in its own game: "<name> has left the game"
    if ( notice[ 0 ] && APP_MODE == 11 && ++notice_wait > 90 )
    {
        int n = 0;
        for ( ; notice[ n ] && n < 99; n++ ) stru_60B960.text[ n ] = (unsigned char)notice[ n ];
        stru_60B960.text[ n ] = 0;
        stru_60B960.length = n;
        sub_5610F0( -256, 0 );
        notice[ 0 ] = 0;
        notice_wait = 0;
    }
    if ( !autohost_done && APP_MODE == MODE_TITLE && getenv( "ILB_AUTOHOST_NAME" ) )
    {
        char error[ 160 ];
        autohost_done = 1;
        install();
        slot = getenv( "ILB_AUTOHOST_SLOT" ) ? atoi( getenv( "ILB_AUTOHOST_SLOT" ) ) : 1;
        snprintf( game_name, sizeof game_name, "%s", getenv( "ILB_AUTOHOST_NAME" ) );
        snprintf( password, sizeof password, "%s", getenv( "ILB_AUTOHOST_PASSWORD" ) ? getenv( "ILB_AUTOHOST_PASSWORD" ) : "" );
        if ( reach_lobby() && lobby_host( game_name, password, error, sizeof error ) )
            hosting_while_playing = 1;
        else
            fprintf( stderr, "net menu: could not put the game back on the list; playing on alone\n" );
        want = WANT_NOTHING;
        sub_42BCC0();
        cs_do_select( 99 + slot );
        return;
    }
    if ( !hosting_while_playing )
        return;
    if ( !joiner_arrived )
    {
        int state = lobby_wait( other_player, sizeof other_player );
        if ( state < 0 )
        {
            fprintf( stderr, "net menu: the game server is gone; this game is no longer on the list\n" );
            leave_lobby();
            return;
        }
        joiner_arrived = state > 0;
    }
    if ( joiner_arrived && APP_MODE == 11 )     // in play (not on the loading screen): save, then start together
    {
        fprintf( stderr, "net menu: %s is joining; saving and starting the two-player game\n", other_player );
        hosting_while_playing = joiner_arrived = 0;
        game_save( 0, 0 );
        start_session( 1 );                     // returns only if it failed
        fprintf( stderr, "net menu: %s\n", message );
    }
}

// ---------------------------------------------------------------------------------------- the two hooks
// At the start of mm_do_play: 1 = the Play button opens the game type screen instead.
int ILB_mod_net_menu_play( int button_event )
{
    if ( button_event != 100 || bypass_play || ILB_input_net_in_place() || getenv( "ILB_NET_FD" ) )
        return 0;
    install();
    channel_play( 1, 1.0f );
    want = WANT_NOTHING;
    leave_lobby();
    show( SCREEN_TYPE );
    APP_MODE_WANTED = MODE_NET_MENU;
    return 1;
}

// At the start of cs_do_select: 1 = this character was picked for hosting or joining, not to be loaded.
int ILB_mod_net_menu_character( unsigned int button )
{
    char path[ 160 ];
    FILE *file;
    if ( want == WANT_NOTHING || APP_MODE != MODE_CHARACTERS || ILB_input_net_in_place() || getenv( "ILB_NET_FD" ) )
        return 0;
    if ( button < 100 || button > 199 )
    {
        want = WANT_NOTHING;            // "back" and the like: this was not about a network game after all
        return 0;
    }
    snprintf( path, sizeof path, "native_appdata/Graybeard Games/It Lurks Below/ilbc_%u.sav", button - 99 );
    if ( ( file = fopen( path, "rb" ) ) == 0 )
    {
        want = WANT_NOTHING;            // an empty slot: the game goes on to make a new character, as always
        return 0;
    }
    fclose( file );
    slot = (int)( button - 99 );
    channel_play( 1, 1.0f );
    if ( want == WANT_HOST )
    {
        if ( game_name[ 0 ] == 0 ) snprintf( game_name, sizeof game_name, "%.20s's game", net_session_player_name() );
        password[ 0 ] = 0;
        show( SCREEN_HOST_FORM );
    }
    else
        refresh_list();
    APP_MODE_WANTED = MODE_NET_MENU;
    return 1;
}
#endif
