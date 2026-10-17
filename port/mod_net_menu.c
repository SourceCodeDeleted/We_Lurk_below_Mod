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
#include "net/net_lobby.h"
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

// ---------------------------------------------------------------------------------------- the list of games
// Drawn the way the game draws its character screen (draw_char_select, 0x42C5C0): one panel per game with the
// host's character in it, five at a time, and in place of that screen's delete icon a join icon. The art is
// the game's UI picture (ui_gfx\ui.gs3): entries of its rectangle table at 0x5D0158 (pixels) / 0xB05F50 (UV).
// The host's character comes from the lobby server (net_lobby.c: lobby_set_character); its portrait is made
// by the character screen's own code in the character slot records, which that screen reloads from the
// saves whenever it opens.
#define UI_PANEL            193                                     // the slot panel
#define UI_PANEL_HOVER      171
#define UI_FRAME            127                                     // the portrait frame
#define UI_JOIN             195                                     // the icon next to the trash icon (194) in the picture
#define UI_ICON_HOVER       172                                     // the trash icon's highlight, the same size
#define UI_WIDTH( k )       ( *(float *)ILB_ADDR( 0x5D0158 + 16 * ( k ) + 8 ) )
#define UI_HEIGHT( k )      ( *(float *)ILB_ADDR( 0x5D0158 + 16 * ( k ) + 12 ) )
#define UI_UV( k )          ( (const int *)ILB_ADDR( 0xB05F50 + 16 * ( k ) ) )
#define UI_SCALE            ( *(float *)ILB_ADDR( 0x611448 ) )
#define SLOT( i )           ( (unsigned char *)ILB_ADDR( 0x60E898 + 260 * ( i ) ) )      // the character screen's slot records
#define LOADING_PROGRESS    ( *(int *)ILB_ADDR( 0x38A5A94 ) )
#define VISIBLE             5
#define BUTTON_GAME         100                                     // + row: the panel
#define BUTTON_JOIN         200                                     // + row: the join icon
#define BUTTON_REFRESH      998
#define BUTTON_BACK         999
#define WINDOW_MENU         108                                     // the window sub_495D10 makes for this mode

// What the host sends of its character: the parts of its slot record the character screen shows.
typedef struct
{
    char name[ 32 ];
    int level, character_class, race, style;
    unsigned char look[ 0x9C ];
} lobby_character;

static int list_top;                    // the first game shown
static int ui_model = -1;
static int portrait_made[ VISIBLE ];
static int buttons[ 2 * VISIBLE + 2 ][ 19 ];     // the game's button records (button_definition_data)
static int buttons_added;
static int text_hover;                  // Refresh or Back under the mouse: BUTTON_REFRESH, BUTTON_BACK or 0

static int bits( float value )         { return *(int *)&value; }

static void character_to_hex( const unsigned char *slot, char *hex )
{
    lobby_character c = { { 0 } };
    const unsigned char *bytes = (const unsigned char *)&c;
    for ( int i = 0; i < 31 && slot[ 3 + i ]; i++ ) c.name[ i ] = (char)slot[ 3 + i ];
    c.level = *(const int *)( slot + 0x44 );
    c.character_class = *(const int *)( slot + 0x48 );
    c.race = *(const int *)( slot + 0xA4 );
    c.style = *(const int *)( slot + 0xE8 );
    for ( int i = 0; i < 0x9C; i++ ) c.look[ i ] = slot[ 0x4C + i ];
    for ( unsigned int i = 0; i < sizeof c; i++ )
    {
        hex[ 2 * i ] = "0123456789abcdef"[ bytes[ i ] >> 4 ];
        hex[ 2 * i + 1 ] = "0123456789abcdef"[ bytes[ i ] & 15 ];
    }
    hex[ 2 * sizeof c ] = 0;
}

static int character_from_hex( const char *hex, lobby_character *c )
{
    unsigned char *bytes = (unsigned char *)c;
    for ( unsigned int i = 0; i < sizeof *c; i++ )
    {
        int high = hex[ 2 * i ], low = high ? hex[ 2 * i + 1 ] : 0;
        if ( !( ( high >= '0' && high <= '9' ) || ( high >= 'a' && high <= 'f' ) ) || !( ( low >= '0' && low <= '9' ) || ( low >= 'a' && low <= 'f' ) ) )
            return 0;
        bytes[ i ] = (unsigned char)( ( ( high <= '9' ? high - '0' : high - 'a' + 10 ) << 4 ) | ( low <= '9' ? low - '0' : low - 'a' + 10 ) );
    }
    c->name[ 31 ] = 0;
    // it came over the network: only what the character screen itself could hold
    return hex[ 2 * sizeof *c ] == 0 && c->level >= 1 && c->level <= 1000 && c->character_class >= 0
        && c->character_class < num_character_class_definitions_60E894 && c->style >= 0 && c->style <= 4;
}

// The host's own character, for the list: from its slot record, filled while the character screen is up.
static void offer_character( int chosen_slot )
{
    char hex[ 2 * sizeof( lobby_character ) + 1 ];
    character_to_hex( SLOT( chosen_slot - 1 ), hex );
    lobby_set_character( hex );
}

static void free_portraits( void )
{
    for ( int i = 0; i < VISIBLE; i++ )
    {
        int *handle = (int *)( SLOT( i ) + 0xF0 );         // material, texture, model
        if ( !portrait_made[ i ] ) continue;
        if ( handle[ 2 ] != -1 ) sub_4113E0( handle[ 2 ] );
        if ( handle[ 0 ] != -1 ) gge_material_free( handle[ 0 ] );
        if ( handle[ 1 ] != -1 ) gge_texture_free( handle[ 1 ] );
        handle[ 0 ] = handle[ 1 ] = handle[ 2 ] = -1;
        SLOT( i )[ 0 ] = 0;
        portrait_made[ i ] = 0;
    }
}

// Slot record i holds the character of the i-th game shown, and its portrait.
static void make_portraits( void )
{
    free_portraits();
    for ( int i = 0; i < VISIBLE && list_top + i < game_count; i++ )
    {
        lobby_character c;
        unsigned char *slot = SLOT( i );
        int *handle = (int *)( slot + 0xF0 );
        if ( !games[ list_top + i ].character[ 0 ] || !character_from_hex( games[ list_top + i ].character, &c ) )
            continue;
        memset( slot, 0, 260 );
        slot[ 0 ] = 1;                                      // in use
        slot[ 2 ] = 1;                                      // selectable: the name is drawn white
        for ( int k = 0; k < 31 && c.name[ k ]; k++ ) slot[ 3 + k ] = (unsigned char)c.name[ k ];
        *(int *)( slot + 0x44 ) = c.level;
        *(int *)( slot + 0x48 ) = c.character_class;
        for ( int k = 0; k < 0x9C; k++ ) slot[ 0x4C + k ] = c.look[ k ];
        *(int *)( slot + 0xA4 ) = c.race != 0;
        *(int *)( slot + 0xE8 ) = c.style;
        slot[ 0xEC ] = 1;
        handle[ 0 ] = handle[ 1 ] = handle[ 2 ] = -1;
        sub_42E6B0( i );                                    // create_slot_graphics
        portrait_made[ i ] = 1;
    }
}

static int __cdecl on_button( unsigned int id, int pressed );

static void remove_buttons( void )
{
    if ( !buttons_added ) return;
    for ( int i = 0; i < 2 * VISIBLE + 2; i++ )
        if ( buttons[ i ][ 0 ] ) ui_core_button_remove( (button_definition_data *)buttons[ i ] );
    memset( buttons, 0, sizeof buttons );
    buttons_added = 0;
}

static void add_button( int index, int id, float x, float y, float w, float h )
{
    int *b = buttons[ index ];
    memset( b, 0, sizeof buttons[ index ] );
    b[ 0 ] = WINDOW_MENU;
    b[ 1 ] = -1;
    b[ 2 ] = -1;
    b[ 3 ] = id;
    b[ 4 ] = (int)( x + 0.5f ); b[ 5 ] = (int)( y + 0.5f ); b[ 6 ] = (int)( w + 0.5f ); b[ 7 ] = (int)( h + 0.5f );
    b[ 9 ] = (int)on_button;
    ui_core_button_add( (button_definition_data *)b );
}

// The geometry of draw_char_select: a portrait frame, the panel, and the icon to its right, centred.
typedef struct { float frame_left, panel_left, panel_right, icon_left, icon_right, title_y, first_top, row_height, step; } list_layout;

static list_layout layout( void )
{
    list_layout l;
    float s = UI_SCALE, gap = s * 4.0f, header = 2.0f * gap + ( flt_61143C + 1.0f ) * 16.0f;
    float row = UI_HEIGHT( UI_FRAME ) * s;
    float total = UI_WIDTH( UI_FRAME ) * s + gap + UI_WIDTH( UI_PANEL ) * s + gap + UI_WIDTH( UI_JOIN ) * s + gap;
    l.panel_left = ( (float)WINDOW_WIDTH - total ) * 0.5f + UI_WIDTH( UI_FRAME ) * s + gap;
    l.panel_right = l.panel_left + UI_WIDTH( UI_PANEL ) * s;
    l.frame_left = l.panel_left - gap - UI_WIDTH( UI_FRAME ) * s;
    l.icon_left = l.panel_right + gap;
    l.icon_right = l.icon_left + UI_WIDTH( UI_JOIN ) * s;
    l.title_y = ( (float)WINDOW_HEIGHT - ( ( row + gap ) * VISIBLE + header + gap + s * 16.0f ) ) * 0.5f;
    l.first_top = l.title_y + header;
    l.row_height = UI_HEIGHT( UI_PANEL ) * s;
    l.step = l.row_height + gap;
    return l;
}

static void add_buttons( void )
{
    list_layout l = layout();
    float s = UI_SCALE, bottom = (float)WINDOW_HEIGHT - flt_61143C * 16.0f - 96.0f;
    remove_buttons();
    for ( int i = 0; i < VISIBLE && list_top + i < game_count; i++ )
    {
        float top = l.first_top + l.step * (float)i;
        add_button( i, BUTTON_GAME + i, l.frame_left, top, l.panel_right - l.frame_left, l.row_height );
        add_button( VISIBLE + i, BUTTON_JOIN + i, l.icon_left, top + 2.0f * s, UI_WIDTH( UI_JOIN ) * s, UI_HEIGHT( UI_JOIN ) * s );
    }
    // Back where the character screen has its own, Refresh next to it
    add_button( 2 * VISIBLE, BUTTON_BACK, 32.0f, bottom, flt_61143C * 64.0f, flt_61143C * 16.0f );
    add_button( 2 * VISIBLE + 1, BUTTON_REFRESH, 32.0f + flt_61143C * 96.0f, bottom, flt_61143C * 96.0f, flt_61143C * 16.0f );
    buttons_added = 1;
}

static void quad( int entry, float x0, float y0, float x1, float y1 )
{
    const int *uv = UI_UV( entry );
    ui_add_vert( -1, bits( x0 ), bits( y0 ), uv[ 0 ], uv[ 1 ] );
    ui_add_vert( -1, bits( x0 ), bits( y1 ), uv[ 0 ], uv[ 3 ] );
    ui_add_vert( -1, bits( x1 ), bits( y1 ), uv[ 2 ], uv[ 3 ] );
    ui_add_vert( -1, bits( x0 ), bits( y0 ), uv[ 0 ], uv[ 1 ] );
    ui_add_vert( -1, bits( x1 ), bits( y1 ), uv[ 2 ], uv[ 3 ] );
    ui_add_vert( -1, bits( x1 ), bits( y0 ), uv[ 2 ], uv[ 1 ] );
}

static void ascii_to_text( const char *text )
{
    int n = 0;
    for ( ; text[ n ] && n < 250; n++ ) stru_60B960.text[ n ] = (unsigned char)text[ n ];
    stru_60B960.text[ n ] = 0;
    stru_60B960.length = n;
}

static void text_in_panel( int color, const list_layout *l, float y )      // the text in stru_60B960, centred in the panel
{
    float width = ui_font_pixel_strlen( &stru_60B960, flt_61144C ).m128_f32[ 0 ];
    float x = ( ( l->panel_right - l->panel_left ) - ( width - 2.0f * flt_61144C ) ) * 0.5f + l->panel_left;
    ui_font_draw_string( color, &stru_60B960.length, (_OWORD)*(unsigned int *)&x, bits( y ), bits( flt_61144C ), 1 );
}

static int button_state( int index )    { return buttons_added && buttons[ index ][ 0 ] ? ui_core_get_button_state( (button_definition_data *)buttons[ index ] ) : 0; }

static void list_set_up( void );

static void reset_vertices( int model )         // gge_model_reset_vertices, which the game has only inlined
{
    if ( (unsigned int)model < 0x400 )
    {
        *(int *)ILB_ADDR( 0x3846048 + 48 * model ) = 0;
        *(int *)ILB_ADDR( 0x3846050 + 48 * model ) = 0;
    }
}

static void draw_list( void )
{
    list_layout l = layout();
    char line[ 240 ];
    float s = UI_SCALE;
    list_set_up();
    if ( ui_model < 0 ) return;
    reset_vertices( ui_model );
    for ( int i = 0; i < VISIBLE; i++ )                     // the portraits are drawn afresh each frame too
        if ( portrait_made[ i ] ) reset_vertices( *(int *)( SLOT( i ) + 0xF8 ) );
    dword_6114AC = 0;
    text_at( "Join a game", -1, l.title_y, flt_61143C + 1.0f, WHITE );
    if ( game_count == 0 )
        text_at( "Nobody is hosting a game right now.", -1, l.first_top + l.step * 2.0f, normal(), GREY );
    for ( int i = 0; i < VISIBLE && list_top + i < game_count; i++ )
    {
        const lobby_game *game = &games[ list_top + i ];
        float top = l.first_top + l.step * (float)i, bottom = top + l.row_height;
        int lit = button_state( i ) >= 1 || button_state( VISIBLE + i ) >= 1 || cursor == list_top + i;
        quad( UI_PANEL, l.panel_left, top, l.panel_right, bottom );
        quad( UI_FRAME, l.frame_left, top, l.frame_left + UI_WIDTH( UI_FRAME ) * s, top + UI_HEIGHT( UI_FRAME ) * s );
        if ( lit ) quad( UI_PANEL_HOVER, l.panel_left, top, l.panel_right, bottom );
        quad( UI_JOIN, l.icon_left, top + 2.0f * s, l.icon_right, top + 2.0f * s + UI_HEIGHT( UI_JOIN ) * s );
        if ( button_state( VISIBLE + i ) >= 1 ) quad( UI_ICON_HOVER, l.icon_left, top + 2.0f * s, l.icon_right, top + 2.0f * s + UI_HEIGHT( UI_JOIN ) * s );
        if ( portrait_made[ i ] )
        {
            unsigned char *slot = SLOT( i );
            wchar_t number[ 16 ];
            char_slot_draw_character( i, l.frame_left, top );
            ascii_to_text( (const char *)slot + 3 );
            text_in_panel( WHITE, &l, top + 10.0f );
            // "Level <n> <race> <class>", as the character screen writes it
            safestring_create_from_id( (int)&stru_60B960, 20 );
            sub_40A0F0( number, 16, (wchar_t *)L"%i", *(int *)( slot + 0x44 ) );
            safestring_concat_str( &stru_60B960, (const unsigned short *)number );
            safestring_concat_str( &stru_60B960, (const unsigned short *)L" " );
            safestring_concat_from_id( &stru_60B960, *(int *)( slot + 0xA4 ) ? 3312 : 3310 );
            safestring_concat_str( &stru_60B960, (const unsigned short *)L" " );
            safestring_concat_from_id( &stru_60B960, character_class_definitions_60CB44[ *(int *)( slot + 0x48 ) ].char_class_string );
            text_in_panel( WHITE, &l, top + 10.0f + flt_61144C * 19.0f );
        }
        else
        {
            ascii_to_text( game->host );
            text_in_panel( WHITE, &l, top + 10.0f );
        }
        snprintf( line, sizeof line, "%s%s  -  %s", game->name, game->locked ? " [password]" : "", game->host );
        ascii_to_text( line );
        text_in_panel( game->locked ? YELLOW : GREY, &l, top + 10.0f + flt_61144C * 38.0f );
    }
    if ( list_top > 0 )
        text_at( "^", -1, l.first_top - l.step * 0.35f, normal(), GREY );
    if ( list_top + VISIBLE < game_count )
        text_at( "v", -1, l.first_top + l.step * VISIBLE, normal(), GREY );
    {
        float bottom = (float)WINDOW_HEIGHT - flt_61143C * 16.0f - 96.0f;
        text_at( "Back", 32.0f, bottom, flt_61143C, button_state( 2 * VISIBLE ) >= 1 || cursor == game_count + 1 ? YELLOW : WHITE );
        text_at( "Refresh", 32.0f + flt_61143C * 96.0f, bottom, flt_61143C, button_state( 2 * VISIBLE + 1 ) >= 1 || cursor == game_count ? YELLOW : WHITE );
    }
    gge_model_add_vertices( ui_model, dword_6114AC, (int)dword_B01350 );
}

// The list is set up on its first frame in this mode: the list is often asked for while the character
// screen is still up (a character was just picked), and that screen's exit takes down the UI and the slot
// graphics. Only once this mode's own set-up (sub_495D10, which makes WINDOW_MENU) has run do they stay.
static int list_ready;

static void list_opened( void )
{
    if ( list_top > game_count - VISIBLE ) list_top = game_count - VISIBLE;
    if ( list_top < 0 ) list_top = 0;
    list_ready = 0;
}

static void list_set_up( void )
{
    if ( list_ready || APP_MODE != MODE_NET_MENU ) return;
    if ( ui_model < 0 ) ui_model = gge_model_new( dword_5D0E98, 512 );
    make_portraits();
    add_buttons();
    list_ready = 1;
}

static void list_closed( void )
{
    remove_buttons();
    free_portraits();
    if ( ui_model >= 0 ) sub_4113E0( ui_model );
    ui_model = -1;
    list_ready = 0;
}

static void scroll_to( int game )       // keeps `game` among the five shown
{
    int top = list_top;
    if ( game < top ) top = game;
    if ( game >= top + VISIBLE ) top = game - VISIBLE + 1;
    if ( top > game_count - VISIBLE ) top = game_count - VISIBLE;
    if ( top < 0 ) top = 0;
    if ( top != list_top ) { list_top = top; if ( list_ready ) { make_portraits(); add_buttons(); } }
}

// ---------------------------------------------------------------------------------------- the loading picture
// While a joiner connects and the files go back and forth, the game's own loading screen (the church in town),
// drawn by its mode 10 functions without loading anything. Freed again before the session restores the
// start state: the session's real loading screen then shows the same picture.
static int loading_up;
void gge_gl_screenshot_next( void );    // port/gge_gl_backend.c

static void loading_picture( int percent )
{
    if ( !loading_up )
    {
        sub_492C00();
        loading_up = 1;
    }
    LOADING_PROGRESS = percent;
    // what the menu drew last stays in its models until drawn again: the pointer and the two text layers
    reset_vertices( *(int *)ILB_ADDR( 0x5E254C ) );
    reset_vertices( *(int *)ILB_ADDR( 0x5E2654 ) );
    reset_vertices( *(int *)ILB_ADDR( 0x5E25FC ) );
    gge_gl_screenshot_next();           // (only with ILB_SCREENSHOTS set: the tests see this picture)
    sub_493790();
}

static void loading_picture_done( void )
{
    if ( loading_up ) sub_492D80();
    loading_up = 0;
}

// ---------------------------------------------------------------------------------------- what the choices do
static void show( int which )
{
    if ( screen == SCREEN_LIST && which != SCREEN_LIST ) list_closed();
    screen = which;
    cursor = 0;
    if ( which == SCREEN_LIST ) list_opened();
}
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
        if ( !is_host )
        {
            loading_picture( 30 );
            loading_picture_done();             // freed before the start state comes back; the picture stays up (frozen)
        }
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
    loading_picture_done();
    connected = 0;                      // the connection was the game's; it is of no use any more
    lobby_disconnect();
    say( error );
}

static void join_chosen( void )
{
    char error[ 160 ];
    list_closed();                      // nothing of the list may be left to draw over the loading picture
    loading_picture( 0 );
    if ( !lobby_join( &games[ chosen_game ], password, error, sizeof error ) )
    {
        loading_picture_done();
        say( error );                   // wrong password, game gone, ...: still connected, back to the list afterwards
        return;
    }
    loading_picture( 10 );
    start_session( 0 );
}

// A click on the list (the game's button system calls this; see add_buttons).
static int __cdecl on_button( unsigned int id, int pressed )
{
    (void)pressed;
    if ( screen != SCREEN_LIST ) return 0;
    channel_play( 1, 1.0f );
    if ( id == BUTTON_BACK ) { leave_lobby(); show( SCREEN_TYPE ); return 0; }
    if ( id == BUTTON_REFRESH ) { refresh_list(); return 0; }
    {
        int game = list_top + (int)( id >= BUTTON_JOIN ? id - BUTTON_JOIN : id - BUTTON_GAME );
        if ( game < 0 || game >= game_count ) return 0;
        cursor = chosen_game = game;
        password[ 0 ] = 0;
        if ( games[ chosen_game ].locked ) show( SCREEN_PASSWORD );
        else join_chosen();
    }
    return 0;
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
    memset( buttons, 0, sizeof buttons );   // the UI was reset by that: no button of the list is there any more
    buttons_added = 0;
    list_ready = 0;
}

static void on_leave_mode( void )
{
    list_closed();
    loading_picture_done();
    sub_495D90();
}

static void on_wheel( int delta )       // the list scrolls a game at a time, as the character screen does
{
    if ( screen != SCREEN_LIST || game_count <= VISIBLE ) return;
    scroll_to( delta > 0 ? list_top - 1 : list_top + VISIBLE );
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
    if ( screen == SCREEN_LIST && line < game_count ) scroll_to( line );
}

static void on_mouse_move( int x, int y, int modifiers )
{
    sub_42A680( x, y );                 // what the game's own menus do with the mouse: this moves the pointer on the screen
    if ( screen == SCREEN_LIST )        // the list's panels are the game's own buttons, which follow the mouse themselves
    {
        mouse_line = -1;
        for ( int i = 0; i < 2 * VISIBLE + 2; i++ )
            if ( button_state( i ) >= 1 )
            {
                int id = buttons[ i ][ 3 ];
                cursor = id == BUTTON_BACK ? game_count + 1 : id == BUTTON_REFRESH ? game_count
                       : list_top + ( id >= BUTTON_JOIN ? id - BUTTON_JOIN : id - BUTTON_GAME );
            }
        return;
    }
    mouse_line = line_under( x, y );
    if ( mouse_line >= 0 ) move_highlight( mouse_line );
}

static void on_left_button( int down, int modifiers )
{
    if ( screen == SCREEN_LIST )
    {
        sub_42A6A0( (char)down, (char)modifiers );         // the game's button system: a click on a panel or icon calls on_button
        return;
    }
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
            draw_list();
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
    handlers[ 6 ] = (void *)on_wheel;
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
        want = WANT_NOTHING;
        sub_42BCC0();                           // the character screen's set-up: loads the slot records, then "this slot was picked"
        offer_character( slot );
        if ( reach_lobby() && lobby_host( game_name, password, error, sizeof error ) )
            hosting_while_playing = 1;
        else
            fprintf( stderr, "net menu: could not put the game back on the list; playing on alone\n" );
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
        // An empty slot. Making a character here would end in a single-player game (the game generates its
        // world and starts it), so a network game asks for an existing character instead.
        want = WANT_NOTHING;
        channel_play( 1, 1.0f );
        say( "Make a character in Single player first, then host or join with it." );    // (one line on the screen)
        APP_MODE_WANTED = MODE_NET_MENU;
        return 1;
    }
    fclose( file );
    slot = (int)( button - 99 );
    channel_play( 1, 1.0f );
    if ( want == WANT_HOST )
    {
        offer_character( slot );        // the slot records are loaded while the character screen is up
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
