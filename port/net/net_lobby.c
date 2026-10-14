// net_lobby.c - the game's side of the lobby protocol (server/src/protocol.rs): say hello, host a game or
// list and join one. Hand-written; built with the system's C headers like net_link.c.
//
// All calls wait for the server's answer (a few seconds at most) except lobby_wait, which is polled while
// a "waiting for a player" screen is up. After lobby_wait or lobby_join succeeds, the same connection
// carries the two game copies' own data and nothing in this file touches it again.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "net_compat.h"

int net_link_connect( const char *host, int port, int seconds );
void net_link_adopt( int fd );
int net_link_fd( void );
int net_link_send( const void *data, int size );
int net_link_receive( void *data, int size, int wait );
int net_link_receive_all( void *data, int size, int seconds );
void sha256_hex( const void *first, size_t first_size, const void *second, size_t second_size, char hex[ 65 ] );
int sha256_file_hex( const char *path, char hex[ 65 ] );

#include "net_lobby.h"

static char nonce[ 40 ];
static char pending[ 2048 ];        // what has arrived of the next line (only lobby_wait leaves any)
static char character[ 481 ];       // the host's character, sent with HOST (lobby_set_character)
static int pending_size;
static time_t last_ping;

static int say( const char *line )
{
    return net_link_send( line, (int)strlen( line ) ) && net_link_send( "\n", 1 );
}

// One line from the server, without its newline. Returns 1, 0 for "nothing yet" (only with wait 0), -1 lost.
static int hear( char *line, int size, int wait )
{
    for ( ;; )
    {
        char *end = memchr( pending, '\n', (size_t)pending_size );
        if ( end )
        {
            int length = (int)( end - pending );
            if ( length >= size ) return -1;
            memcpy( line, pending, (size_t)length );
            line[ length ] = 0;
            memmove( pending, end + 1, (size_t)( pending_size - length - 1 ) );
            pending_size -= length + 1;
            return 1;
        }
        if ( pending_size >= (int)sizeof pending ) return -1;
        if ( wait )
        {
            if ( !net_link_receive_all( pending + pending_size, 1, 15 ) ) return -1;
            pending_size++;
        }
        else
        {
            int got = net_link_receive( pending + pending_size, (int)sizeof pending - pending_size, 0 );
            if ( got < 0 ) return -1;
            if ( got == 0 ) return 0;
            pending_size += got;
        }
    }
}

static void copy_reason( const char *answer, char *error, int error_size )
{
    const char *tab = strchr( answer, '\t' );
    snprintf( error, (size_t)error_size, "%s", tab ? tab + 1 : answer[ 0 ] ? answer : "the server did not answer" );
}

// Who this copy is: the first 16 hex characters of the hash of its own program file, so that only copies
// of exactly the same build are matched.
const char *lobby_build_id( void )
{
    static char id[ 65 ];
#ifdef _WIN32
    char program[ 1024 ];
    DWORD length = GetModuleFileNameA( 0, program, sizeof program );
    if ( id[ 0 ] == 0 && ( length == 0 || length >= sizeof program || !sha256_file_hex( program, id ) ) )
#else
    if ( id[ 0 ] == 0 && !sha256_file_hex( "/proc/self/exe", id ) )
#endif
        strcpy( id, "0000000000000000" );
    id[ 16 ] = 0;
    return id;
}

// Connect to the lobby at "address[:port]" and introduce this copy. Returns 1, or 0 with a reason in `error`.
int lobby_connect( const char *address, const char *player, char *error, int error_size )
{
    char host[ 128 ], line[ 600 ];
    int i = 0, fd;
    for ( ; address[ i ] && address[ i ] != ':' && i < 127; i++ ) host[ i ] = address[ i ];
    host[ i ] = 0;
    fd = net_link_connect( host, address[ i ] == ':' ? atoi( address + i + 1 ) : 27960, 6 );
    if ( fd < 0 )
    {
        snprintf( error, (size_t)error_size, "cannot reach the game server at %s", address );
        return 0;
    }
    net_link_adopt( fd );
    pending_size = 0;
    snprintf( line, sizeof line, "HELLO\t1\t%s\t%.32s", lobby_build_id(), player );
    if ( !say( line ) || hear( line, sizeof line, 1 ) != 1 || strncmp( line, "OK\t", 3 ) != 0 || strlen( line + 3 ) >= sizeof nonce )
    {
        copy_reason( line, error, error_size );
        return 0;
    }
    strcpy( nonce, line + 3 );
    last_ping = time( 0 );
    return 1;
}

void lobby_disconnect( void )
{
    if ( net_link_fd() >= 0 ) net_close( net_link_fd() );
    net_link_adopt( -1 );
}

// The host's character as the lobby list shows it to others: lower-case hex, at most 480 characters (the
// server's limit). Kept until changed; 0 or "" sends none.
void lobby_set_character( const char *hex )
{
    snprintf( character, sizeof character, "%s", hex ? hex : "" );
}

// Offer a game. An empty password means anyone may join. Returns 1, or 0 with a reason.
int lobby_host( const char *name, const char *password, char *error, int error_size )
{
    char line[ 1200 ], salt[ 65 ] = "-", verifier[ 65 ] = "-";
    int with_character = character[ 0 ] != 0;
    if ( password && password[ 0 ] )
    {
        // the salt only has to differ from game to game; the time and this copy's challenge do that
        snprintf( line, sizeof line, "%ld%s%s", (long)time( 0 ), nonce, name );
        sha256_hex( line, strlen( line ), "", 0, salt );
        salt[ 16 ] = 0;
        sha256_hex( salt, 16, password, strlen( password ), verifier );
    }
    for ( ;; )
    {
        char request[ 1200 ];
        snprintf( request, sizeof request, "HOST\t%.32s\t%s\t%s%s%s", name, salt, verifier, with_character ? "\t" : "", with_character ? character : "" );
        if ( !say( request ) || hear( line, sizeof line, 1 ) != 1 )
        {
            copy_reason( "", error, error_size );
            return 0;
        }
        if ( strncmp( line, "HOSTING\t", 8 ) == 0 )
            return 1;
        // a server from before the character field: host without it
        if ( with_character && strncmp( line, "ERROR\t", 6 ) == 0 && ( strstr( line, "unknown request" ) || strstr( line, "character" ) ) )
        {
            with_character = 0;
            continue;
        }
        copy_reason( line, error, error_size );
        return 0;
    }
}

// While hosting: has someone joined? 0 not yet, 1 yes (their name is filled in and the connection now
// belongs to the game), -1 the server is gone. Call it a few times a second.
int lobby_wait( char *joiner, int joiner_size )
{
    char line[ 1200 ];
    int heard;
    if ( time( 0 ) - last_ping >= 20 )
    {
        last_ping = time( 0 );
        if ( !say( "PING" ) ) return -1;
    }
    while ( ( heard = hear( line, sizeof line, 0 ) ) == 1 )
        if ( strncmp( line, "JOINED\t", 7 ) == 0 )
        {
            snprintf( joiner, (size_t)joiner_size, "%s", line + 7 );
            return say( "READY" ) ? 1 : -1;
        }
    return heard;
}

// The games on offer for this build. Returns how many were put in `games`, or -1 with a reason.
int lobby_list( lobby_game *games, int max, char *error, int error_size )
{
    char line[ 1200 ];
    int count = 0;
    if ( !say( "LIST" ) ) { snprintf( error, (size_t)error_size, "the game server is gone" ); return -1; }
    for ( ;; )
    {
        lobby_game game = { 0 };
        int fields;
        if ( hear( line, sizeof line, 1 ) != 1 ) { snprintf( error, (size_t)error_size, "the game server is gone" ); return -1; }
        if ( strcmp( line, "END" ) == 0 ) return count;
        // the seventh field, the host's character, is missing from older servers
        fields = sscanf( line, "GAME\t%d\t%99[^\t]\t%d\t%99[^\t]\t%d\t%19s\t%480s", &game.id, game.name, &game.locked, game.host, &game.age, game.salt, game.character );
        if ( fields < 7 || strcmp( game.character, "-" ) == 0 ) game.character[ 0 ] = 0;
        if ( fields >= 6 && count < max )
            games[ count++ ] = game;
    }
}

// Join a game from the list. Returns 1 (the connection now belongs to the game), or 0 with a reason.
int lobby_join( const lobby_game *game, const char *password, char *error, int error_size )
{
    char line[ 1200 ], verifier[ 65 ], proof[ 65 ] = "-";
    if ( game->locked )
    {
        sha256_hex( game->salt, strlen( game->salt ), password ? password : "", password ? strlen( password ) : 0, verifier );
        sha256_hex( nonce, strlen( nonce ), verifier, 64, proof );
    }
    snprintf( line, sizeof line, "JOIN\t%d\t%s", game->id, proof );
    if ( !say( line ) || hear( line, sizeof line, 1 ) != 1 || strcmp( line, "OK" ) != 0 )
    {
        copy_reason( line, error, error_size );
        return 0;
    }
    return 1;
}
