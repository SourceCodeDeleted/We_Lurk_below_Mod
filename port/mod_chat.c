// mod_chat.c - typed chat between the two players of a network game (hand-written, NOT part of the original).
//
// In play, Enter opens a line at the game's messages ("> text_"). While it is open, key presses and typed
// text go to the line instead of the game; key releases still reach the game, so a key that was held down
// (walking, say) is let go as usual. Backspace deletes, Escape closes the line, Enter sends it. A sent line
// shows on both screens as "<character name>: text" among the game's own messages and goes away after about
// ten seconds; one line may be sent every half second at most.
//
// Chat never touches what the two copies compute together: it travels beside the game's input as its own
// message ('M', see draft/native/native_input.c) and is written only into the message list each copy shows
// its own player - picture, not game state.
#include "ILB_draft_decls.h"

typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );
int snprintf( char *, unsigned int, const char *, ... );
void *memmove( void *, const void *, unsigned int );
unsigned int strlen( const char * );

unsigned int net_link_ms( void );                       // port/net/net_link.c
const char *net_session_player_name( void );            // port/net/net_session.c
int ILB_input_net_guest( void );                        // draft/native/native_input.c
int ILB_input_net_send_chat( const char *text, int length );

#define APP_MODE            ( *(int *)ILB_ADDR( 0x3A73CC0 ) )
#define MODE_PLAY           11

// The game's message list, one per player (client): 8 entries, then how many are in use
// (ui_message_add_str, 0x5610F0, and ui_process_messages show the layout).
#define MESSAGE_LIST( c )   ( (unsigned char *)ILB_ADDR( 0xE8B7D0 + 16676 * ( c ) ) )
#define MESSAGE_COUNT( c )  ( *(int *)ILB_ADDR( 0xE8F8F0 + 16676 * ( c ) ) )
#define MESSAGE_SIZE        2084
#define MAX_MESSAGES        8
#define TEXT_LENGTH( e )    ( *(int *)( e ) )                                   // safestring: length, then UTF-16
#define TEXT( e )           ( (unsigned short *)( ( e ) + 4 ) )
#define COLOUR( e )         ( *(int *)( ( e ) + 2056 ) )
#define TIMER( e )          ( *(int *)( ( e ) + 2060 ) )                        // counted down while shown; gone at 0
#define SHOWN_STEPS         600                                                 // about ten seconds
#define CHAT_COLOUR         -256                                                // as the game's "Day N" notice

#define MAX_TYPED           60
#define MIN_GAP_MS          500

static int typing;
static char typed[ MAX_TYPED + 1 ];
static unsigned int last_sent_ms;
static int ever_sent;

static int own_client( void ) { return ILB_input_net_guest() ? 1 : 0; }        // the host is player 1, the joiner player 2

static void set_text( unsigned char *entry, const char *text )
{
    int n = 0;
    for ( ; text[ n ] && n < 1023; n++ ) TEXT( entry )[ n ] = (unsigned char)text[ n ];
    TEXT( entry )[ n ] = 0;
    TEXT_LENGTH( entry ) = n;
}

static int is_typing_line( unsigned char *entry )
{
    return TEXT_LENGTH( entry ) >= 2 && TEXT( entry )[ 0 ] == '>' && TEXT( entry )[ 1 ] == ' ';
}

// A new entry at the end of this player's list; when the list is full the oldest gives way.
static unsigned char *new_entry( void )
{
    int client = own_client(), count = MESSAGE_COUNT( client );
    unsigned char *list = MESSAGE_LIST( client ), *entry;
    if ( count < 0 || count > MAX_MESSAGES ) return 0;
    if ( count == MAX_MESSAGES )
    {
        memmove( list, list + MESSAGE_SIZE, MESSAGE_SIZE * ( MAX_MESSAGES - 1 ) );
        count--;
    }
    entry = list + MESSAGE_SIZE * count;
    for ( int i = 0; i < MESSAGE_SIZE; i++ ) entry[ i ] = 0;
    *(int *)( entry + 2052 ) = -1;
    COLOUR( entry ) = CHAT_COLOUR;
    TIMER( entry ) = SHOWN_STEPS;
    *(float *)( entry + 2064 ) = 2.0f;
    MESSAGE_COUNT( client ) = count + 1;
    return entry;
}

static unsigned char *find_typing_line( void )
{
    int client = own_client(), count = MESSAGE_COUNT( client );
    for ( int i = 0; i < count && i < MAX_MESSAGES; i++ )
        if ( is_typing_line( MESSAGE_LIST( client ) + MESSAGE_SIZE * i ) )
            return MESSAGE_LIST( client ) + MESSAGE_SIZE * i;
    return 0;
}

static void show( const char *line )
{
    unsigned char *entry = new_entry();
    if ( entry ) set_text( entry, line );
}

static void close_line( void )
{
    unsigned char *entry = find_typing_line();
    if ( entry ) { set_text( entry, " " ); TIMER( entry ) = 1; }    // gone on the game's next count
    typing = 0;
    typed[ 0 ] = 0;
}

static void send_line( void )
{
    char line[ 120 ];
    unsigned int now = net_link_ms();
    if ( typed[ 0 ] == 0 ) { close_line(); return; }
    if ( ever_sent && now - last_sent_ms < MIN_GAP_MS ) return;    // too soon: the line stays, Enter again sends it
    snprintf( line, sizeof line, "%.32s: %s", net_session_player_name(), typed );
    if ( !ILB_input_net_send_chat( line, (int)strlen( line ) ) )
        fprintf( stderr, "chat: could not send\n" );
    last_sent_ms = now;
    ever_sent = 1;
    close_line();
    show( line );
}

// One of this copy's own input events, before it is sent to the game. Returns 1 if chat took it.
int ILB_chat_event( const unsigned char *event )
{
    unsigned int type = *(const unsigned int *)event;
    if ( APP_MODE != MODE_PLAY ) { if ( typing ) close_line(); return 0; }
    if ( !typing )
    {
        // SDL key event: keysym.sym at 20, repeat at 13
        if ( type == 0x300 && event[ 13 ] == 0 && ( *(const int *)( event + 20 ) == 13 || *(const int *)( event + 20 ) == 0x40000058 ) )
        {
            typing = 1;
            typed[ 0 ] = 0;
            return 1;
        }
        return 0;
    }
    if ( type == 0x300 )                                    // a key pressed while typing
    {
        int sym = *(const int *)( event + 20 ), length = (int)strlen( typed );
        if ( sym == 13 || sym == 0x40000058 ) send_line();
        else if ( sym == 27 ) close_line();
        else if ( sym == 8 && length > 0 ) typed[ length - 1 ] = 0;
        return 1;
    }
    if ( type == 0x303 )                                    // typed text (UTF-8, at 12): plain characters only
    {
        int length = (int)strlen( typed );
        for ( const unsigned char *c = event + 12; *c && c < event + 12 + 32 && length < MAX_TYPED; c++ )
            if ( *c >= 32 && *c < 127 ) { typed[ length++ ] = (char)*c; typed[ length ] = 0; }
        return 1;
    }
    return type == 0x302;                                   // text being composed (input methods): ignored
}

// Every pass of the main loop during a session: the line being typed stays on the screen and shows the text.
void ILB_chat_frame( void )
{
    char line[ MAX_TYPED + 8 ];
    unsigned char *entry;
    if ( !typing ) return;
    if ( APP_MODE != MODE_PLAY ) { close_line(); return; }
    entry = find_typing_line();
    if ( entry == 0 && ( entry = new_entry() ) == 0 ) return;
    snprintf( line, sizeof line, "> %s_", typed );
    set_text( entry, line );
    TIMER( entry ) = SHOWN_STEPS;
}

// A line from the other player.
void ILB_chat_received( const char *text, int length )
{
    char line[ 121 ];
    int n = 0;
    for ( int i = 0; i < length && n < 120; i++ )
        if ( text[ i ] >= 32 && text[ i ] < 127 ) line[ n++ ] = text[ i ];
    line[ n ] = 0;
    if ( n == 0 ) return;
    fprintf( stderr, "chat: %s\n", line );
    show( line );
}

// The session is over: nothing half-typed stays behind.
void ILB_chat_reset( void )
{
    typing = 0;
    typed[ 0 ] = 0;
}
