// net_session.c - from "two players matched in the lobby" to "two copies of the game running in step".
// Hand-written; built with the system's C headers like net_link.c.
//
// 1. The copies exchange the files the game will load, so that both start from exactly the same data:
//    the host's character, world, settings and account file; the joiner's character. Each copy puts them
//    in a session folder of its own: slot 1 = the host's character and world, slot 2 = the joiner's
//    character. (The host's settings are used on both, because key bindings and the window size are part
//    of what the game computes from.)
// 2. Each copy then turns into a fresh run of itself (execv) in that folder, connected through the socket
//    it already has, starting a two-player game with slots 1 and 2. A fresh run is what makes the two
//    copies identical from their first step; see draft/native/native_input.c.
// 3. When that run ends, each player's own files are copied back over the real ones (the host: character
//    and world; the joiner: character), after a backup - unless the copies had fallen out of step.
//
// Wire format of step 1, after the lobby's relay has started: the host sends 'S', a 4-byte seed and its
// player's name (32 bytes), the joiner answers with its name (32 bytes); then
// each file is 'F', a 64-byte name, a 4-byte size, 64 hex characters of SHA-256, and the data; 'E' ends a
// side's files. The host sends first, then the joiner.
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "net_compat.h"

int net_link_fd( void );
void net_link_adopt( int fd );
int net_link_send( const void *data, int size );
int net_link_receive_all( void *data, int size, int seconds );
void sha256_hex( const void *first, size_t first_size, const void *second, size_t second_size, char hex[ 65 ] );

#define SAVES "native_appdata/Graybeard Games/It Lurks Below"
#define MAX_FILE ( 96u << 20 )

static char keep_list[ 4096 ];

static int fail( char *error, int error_size, const char *what, const char *detail )
{
    snprintf( error, (size_t)error_size, "%s%s%s", what, detail ? ": " : "", detail ? detail : "" );
    return 0;
}

static unsigned char *read_file( const char *path, unsigned int *size )
{
    FILE *file = fopen( path, "rb" );
    unsigned char *data = 0;
    long length;
    if ( file == 0 ) return 0;
    if ( fseek( file, 0, SEEK_END ) == 0 && ( length = ftell( file ) ) >= 0 && (unsigned long)length <= MAX_FILE
         && fseek( file, 0, SEEK_SET ) == 0 && ( data = malloc( (size_t)length + 1 ) ) != 0 )
    {
        if ( fread( data, 1, (size_t)length, file ) != (size_t)length ) { free( data ); data = 0; }
        *size = (unsigned int)length;
    }
    fclose( file );
    return data;
}

static int write_file( const char *path, const unsigned char *data, unsigned int size )
{
    FILE *file = fopen( path, "wb" );
    int ok;
    if ( file == 0 ) return 0;
    ok = fwrite( data, 1, size, file ) == size;
    return fclose( file ) == 0 && ok;
}

static void make_folders( const char *path )       // every folder on the way to `path`, which is a folder itself
{
    char partial[ 1024 ];
    snprintf( partial, sizeof partial, "%s", path );
    for ( char *p = partial + 1; ; p++ )
        if ( *p == '/' || *p == 0 )
        {
            char kept = *p;
            *p = 0;
            mkdir( partial, 0755 );
            if ( kept == 0 ) break;
            *p = kept;
        }
}

// A name another computer sent may only be one of the plain save file names: no folders, no surprises.
static int acceptable_name( const char *name )
{
    size_t length = strnlen( name, 64 );
    if ( length == 0 || length >= 64 || name[ 0 ] == '.' ) return 0;
    for ( size_t i = 0; i < length; i++ )
        if ( !( ( name[ i ] >= 'a' && name[ i ] <= 'z' ) || ( name[ i ] >= '0' && name[ i ] <= '9' ) || name[ i ] == '_' || name[ i ] == '.' ) )
            return 0;
    return 1;
}

// Send one of this copy's files under the name it has in the session, and put it in this copy's own
// session folder too. `must` 0: a missing file is skipped.
static int share( const char *real_folder, const char *real_name, const char *session_folder, const char *session_name, int must,
                  int keep, char *error, int error_size )
{
    char path[ 1200 ], header[ 1 + 64 + 4 + 64 ] = { 'F' }, digest[ 65 ];
    unsigned int size = 0;
    unsigned char *data;
    snprintf( path, sizeof path, "%s/%s", real_folder, real_name );
    data = read_file( path, &size );
    if ( data == 0 )
        return must ? fail( error, error_size, "cannot read this copy's save file", real_name ) : 1;
    sha256_hex( data, size, "", 0, digest );
    snprintf( header + 1, 64, "%s", session_name );
    memcpy( header + 65, &size, 4 );
    memcpy( header + 69, digest, 64 );
    if ( !net_link_send( header, sizeof header ) || !net_link_send( data, (int)size ) )
    {
        free( data );
        return fail( error, error_size, "the other player has left", 0 );
    }
    if ( keep )         // remember where it goes back to afterwards
        snprintf( keep_list + strlen( keep_list ), sizeof keep_list - strlen( keep_list ), "%s\t%s/%s\n", session_name, real_folder, real_name );
    snprintf( path, sizeof path, "%s/%s", session_folder, session_name );
    must = write_file( path, data, size );
    free( data );
    return must ? 1 : fail( error, error_size, "cannot write the session folder", strerror( errno ) );
}

static int receive_files( const char *session_folder, char *error, int error_size )
{
    for ( ;; )
    {
        char kind, name[ 65 ] = { 0 }, digest[ 65 ] = { 0 }, actual[ 65 ], path[ 1200 ];
        unsigned int size;
        unsigned char *data;
        if ( !net_link_receive_all( &kind, 1, 60 ) ) return fail( error, error_size, "the other player has left", 0 );
        if ( kind == 'E' ) return 1;
        if ( kind != 'F' || !net_link_receive_all( name, 64, 30 ) || !net_link_receive_all( &size, 4, 30 ) || !net_link_receive_all( digest, 64, 30 )
             || !acceptable_name( name ) || size > MAX_FILE )
            return fail( error, error_size, "the other copy sent something unexpected", 0 );
        data = malloc( size + 1 );
        if ( data == 0 || !net_link_receive_all( data, (int)size, 60 ) )
        {
            free( data );
            return fail( error, error_size, "the other player's files did not arrive", name );
        }
        sha256_hex( data, size, "", 0, actual );
        snprintf( path, sizeof path, "%s/%s", session_folder, name );
        if ( memcmp( actual, digest, 64 ) != 0 || !write_file( path, data, size ) )
        {
            free( data );
            return fail( error, error_size, "a file arrived damaged or could not be stored", name );
        }
        free( data );
    }
}

const char *net_session_player_name( void );

// Both copies know both players' names (for "... has joined"): kept in the environment, which a copy that
// restarts itself into the session passes on. Only plain printable characters are kept.
static void remember_names( const char *host, const char *joiner )
{
    char clean[ 2 ][ 33 ];
    const char *names[ 2 ] = { host, joiner };
    for ( int n = 0; n < 2; n++ )
    {
        int length = 0;
        for ( int i = 0; i < 32 && names[ n ][ i ]; i++ )
            if ( names[ n ][ i ] >= 32 && names[ n ][ i ] < 127 ) clean[ n ][ length++ ] = names[ n ][ i ];
        clean[ n ][ length ] = 0;
    }
    setenv( "ILB_NET_HOST_NAME", clean[ 0 ][ 0 ] ? clean[ 0 ] : "Player 1", 1 );
    setenv( "ILB_NET_JOINER_NAME", clean[ 1 ][ 0 ] ? clean[ 1 ] : "Player 2", 1 );
}

// Delete a folder and what is in it; make `path` stand for the existing file or folder `target`. Linux uses
// symbolic links; Windows needs a privilege for those, so there a file becomes a hard link (or a copy) and a
// folder a junction made by the system's own mklink.
static void remove_folder( const char *folder )
{
    char command[ 1300 ];
#ifdef _WIN32
    snprintf( command, sizeof command, "rmdir /s /q \"%s\" 2>nul", folder );
    for ( char *p = command; *p; p++ ) if ( *p == '/' && p > command + 12 ) *p = '\\';
#else
    snprintf( command, sizeof command, "rm -rf '%s'", folder );
#endif
    system( command );
}
static void link_here( const char *target, const char *path )
{
#ifdef _WIN32
    DWORD kind = GetFileAttributesA( target );
    if ( kind == INVALID_FILE_ATTRIBUTES ) return;
    if ( kind & FILE_ATTRIBUTE_DIRECTORY )
    {
        char command[ 2600 ];
        snprintf( command, sizeof command, "mklink /j \"%s\" \"%s\" >nul 2>nul", path, target );
        for ( char *p = command; *p; p++ ) if ( *p == '/' && p > command + 8 ) *p = '\\';
        system( command );
    }
    else if ( !CreateHardLinkA( path, target, 0 ) )
        CopyFileA( target, path, 0 );
#else
    symlink( target, path );
#endif
}

// After the lobby matched two players: exchange the files and build this copy's session folder under
// `session_root`. `slot` is the character slot this player chose. Returns 1 and the seed both will use.
int net_session_prepare( int is_host, int slot, const char *session_root, unsigned int *seed, char *error, int error_size )
{
    char folder[ 1100 ], mine[ 32 ], world[ 32 ], path[ 1200 ], here[ 900 ];
    FILE *list;
    if ( getcwd( here, sizeof here ) == 0 ) return fail( error, error_size, "no working folder", 0 );
    if ( strchr( session_root, '\'' ) == 0 && strchr( session_root, '"' ) == 0 && strstr( session_root, "/net/session_" ) != 0 )    // only ever a folder this file made
        remove_folder( session_root );
    snprintf( folder, sizeof folder, "%s/%s", session_root, SAVES );
    make_folders( folder );
    snprintf( path, sizeof path, "%s/%s", here, SAVES );
    snprintf( mine, sizeof mine, "ilbc_%d.sav", slot );
    snprintf( world, sizeof world, "ilbw_%d.sav", slot );
    keep_list[ 0 ] = 0;
    if ( is_host )
    {
        char hello[ 5 + 32 ] = { 'S' }, theirs[ 33 ] = { 0 };
        *seed = getenv( "ILB_SEED" ) ? (unsigned int)atoi( getenv( "ILB_SEED" ) ) : (unsigned int)time( 0 );
        memcpy( hello + 1, seed, 4 );
        snprintf( hello + 5, 32, "%s", net_session_player_name() );
        if ( !net_link_send( hello, sizeof hello ) || !net_link_receive_all( theirs, 32, 60 ) )
            return fail( error, error_size, "the other player has left", 0 );
        remember_names( net_session_player_name(), theirs );
        if ( !share( path, mine, folder, "ilbc_1.sav", 1, 1, error, error_size ) || !share( path, world, folder, "ilbw_1.sav", 1, 1, error, error_size )
             || !share( path, "ilb_config.txt", folder, "ilb_config.txt", 0, 0, error, error_size )
             || !share( path, "account.sav", folder, "account.sav", 0, 0, error, error_size ) )
            return 0;
        if ( !net_link_send( "E", 1 ) ) return fail( error, error_size, "the other player has left", 0 );
        if ( !receive_files( folder, error, error_size ) ) return 0;
    }
    else
    {
        char hello[ 5 + 32 + 1 ] = { 0 }, mine_name[ 32 ] = { 0 };
        if ( !net_link_receive_all( hello, 5 + 32, 60 ) || hello[ 0 ] != 'S' ) return fail( error, error_size, "the host did not start the game", 0 );
        memcpy( seed, hello + 1, 4 );
        snprintf( mine_name, sizeof mine_name, "%s", net_session_player_name() );
        if ( !net_link_send( mine_name, 32 ) ) return fail( error, error_size, "the other player has left", 0 );
        remember_names( hello + 5, mine_name );
        if ( !receive_files( folder, error, error_size ) ) return 0;
        if ( !share( path, mine, folder, "ilbc_2.sav", 1, 1, error, error_size ) ) return 0;
        if ( !net_link_send( "E", 1 ) ) return fail( error, error_size, "the other player has left", 0 );
    }
    // what the game needs next to its saves, and the list of files that go back afterwards
    snprintf( path, sizeof path, "%s/ILB.gbp", session_root );
    snprintf( folder, sizeof folder, "%s/ILB.gbp", here );
    link_here( folder, path );
    snprintf( path, sizeof path, "%s/mods", session_root );
    snprintf( folder, sizeof folder, "%s/mods", here );
    link_here( folder, path );
    snprintf( path, sizeof path, "%s/keep.txt", session_root );
    if ( ( list = fopen( path, "w" ) ) != 0 )
    {
        fputs( keep_list, list );
        fclose( list );
    }
    return 1;
}

// What a host that had to restart needs to carry on alone afterwards: the same game back on the list with
// the same character. Passed on in the environment by net_session_run.
static char host_name[ 64 ], host_password[ 64 ];
static int host_slot;
void net_session_remember_host( const char *name, const char *password, int slot )
{
    snprintf( host_name, sizeof host_name, "%s", name );
    snprintf( host_password, sizeof host_password, "%s", password ? password : "" );
    host_slot = slot;
}

// Start this program again in place of the running one, under its real name (through /proc/self/exe the
// new process would be called "exe", which hides it from anyone looking for the game by name).
//
// To the player the restart should look like a loading screen, not like the game closing and opening:
//   - the new window opens exactly where the old one was (ILB_WINDOW_X / ILB_WINDOW_Y);
//   - the old window stays on the screen, with its last picture, until the new one has something to show.
//     A window lives as long as its program's connection to the display is open, and that connection is
//     normally closed by the restart itself; here it is kept open across it, and the new run closes it
//     (ILB_OLD_CONNECTIONS) once its own loading screen is up (ILB_native_session_visible).
__attribute__(( weak )) void ILB_native_window_position( int *x, int *y ) {}     // the real one: draft/native/native_platform.c (builds with a window)
#ifdef _WIN32
// Windows cannot turn a running program into a fresh run of itself with its connection kept; there a session
// only starts in place (which is the normal way), and this fallback reports that it is not available.
static void restart( char **argv ) { (void)argv; errno = ENOSYS; }
#else
static void restart( char **argv )
{
    char program[ 1024 ], kept[ 400 ] = "", number[ 32 ];
    ssize_t length = readlink( "/proc/self/exe", program, sizeof program - 1 );
    int x = 0, y = 0;
    ILB_native_window_position( &x, &y );
    if ( x != 0 || y != 0 )
    {
        snprintf( number, sizeof number, "%d", x ); setenv( "ILB_WINDOW_X", number, 1 );
        snprintf( number, sizeof number, "%d", y ); setenv( "ILB_WINDOW_Y", number, 1 );
    }
    for ( int fd = 3; fd < 256; fd++ )              // every socket except the game's own link
    {
        int type = 0, flags;
        socklen_t size = sizeof type;
        if ( fd == net_link_fd() || getsockopt( fd, SOL_SOCKET, SO_TYPE, &type, &size ) != 0 || ( flags = fcntl( fd, F_GETFD ) ) < 0 )
            continue;
        if ( ( flags & FD_CLOEXEC ) && fcntl( fd, F_SETFD, flags & ~FD_CLOEXEC ) == 0 && strlen( kept ) < sizeof kept - 8 )
            snprintf( kept + strlen( kept ), 8, "%d ", fd );
    }
    setenv( "ILB_OLD_CONNECTIONS", kept, 1 );
    fflush( 0 );
    if ( length > 0 )
    {
        program[ length ] = 0;
        execv( program, argv );
    }
    execv( "/proc/self/exe", argv );
}
#endif

// Turn this process into the session run. Only returns if that fails.
void net_session_run( int is_host, const char *session_root, unsigned int seed, char **argv )
{
    char number[ 32 ], path[ 1200 ];
    snprintf( number, sizeof number, "%d", net_link_fd() );
    setenv( "ILB_NET_FD", number, 1 );
    setenv( "ILB_NET_ROLE", is_host ? "host" : "join", 1 );
    snprintf( number, sizeof number, "%u", seed );
    setenv( "ILB_NET_SEED", number, 1 );
    setenv( "ILB_SCRIPT", "coop", 1 );
    setenv( "ILB_SLOT", "1", 1 );
    setenv( "ILB_SLOT2", "2", 1 );
    snprintf( path, sizeof path, "%s/keep.txt", session_root );
    setenv( "ILB_NET_KEEP", path, 1 );
    setenv( "ILB_WINDOW_TITLE", is_host ? "It Lurks Below - player 1 (keyboard and mouse)" : "It Lurks Below - player 2 (controller)", 0 );
    setenv( "SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS", "1", 1 );      // a controller keeps working when the window is not in front
    unsetenv( "ILB_LOBBY_HOST" );
    unsetenv( "ILB_LOBBY_JOIN" );
    unsetenv( "ILB_REPLAY" );
    unsetenv( "ILB_TEST_KEYS" );
    unsetenv( "ILB_RECORD" );
    if ( getenv( "ILB_NET_HOME" ) == 0 && getcwd( path, sizeof path ) != 0 )
        setenv( "ILB_NET_HOME", path, 1 );      // where to go back to afterwards
    unsetenv( "ILB_AUTOHOST_NAME" );
    if ( is_host && host_name[ 0 ] )
    {
        snprintf( number, sizeof number, "%d", host_slot );
        setenv( "ILB_AUTOHOST_NAME", host_name, 1 );
        setenv( "ILB_AUTOHOST_PASSWORD", host_password, 1 );
        setenv( "ILB_AUTOHOST_SLOT", number, 1 );
    }
    if ( chdir( session_root ) != 0 )
    {
        fprintf( stderr, "net: cannot enter the session folder %s\n", session_root );
        return;
    }
    restart( argv );
    fprintf( stderr, "net: cannot restart into the session: %s\n", strerror( errno ) );
}

// At the end of a session run: this player's own files go back over the real ones, each after a backup
// (<name>.before_net). `in_step` 0 = the copies disagreed at some point: nothing is touched.
void net_session_keep( int in_step )
{
    const char *list_path = getenv( "ILB_NET_KEEP" );
    char line[ 1400 ], session[ 1200 ], backup[ 1300 ], folder[ 1100 ];
    FILE *list;
    if ( list_path == 0 || ( list = fopen( list_path, "r" ) ) == 0 )
        return;
    snprintf( folder, sizeof folder, "%s", list_path );
    *strrchr( folder, '/' ) = 0;
    while ( fgets( line, sizeof line, list ) )
    {
        char *tab = strchr( line, '\t' ), *real;
        unsigned int size = 0, old_size = 0;
        unsigned char *data, *old;
        if ( tab == 0 ) continue;
        *tab = 0;
        real = tab + 1;
        real[ strcspn( real, "\n" ) ] = 0;
        if ( !in_step )
        {
            fprintf( stderr, "net: %s is NOT kept: the two copies had fallen out of step\n", line );
            continue;
        }
        snprintf( session, sizeof session, "%s/%s/%s", folder, SAVES, line );
        data = read_file( session, &size );
        if ( data == 0 || size == 0 ) { free( data ); continue; }
        snprintf( backup, sizeof backup, "%s.before_net", real );
        old = read_file( real, &old_size );
        if ( old == 0 || write_file( backup, old, old_size ) )
            fprintf( stderr, "net: progress kept: %s%s\n", real, write_file( real, data, size ) ? "" : " - FAILED to write" );
        free( old );
        free( data );
    }
    fclose( list );
    unsetenv( "ILB_NET_KEEP" );
}

// Start-up without menus, for tests and scripts:
//   ILB_LOBBY_HOST=<game name>   offer a game and wait for a player
//   ILB_LOBBY_JOIN=<game name>   join the game of that name
// with ILB_LOBBY_PASSWORD (optional), ILB_SLOT (this player's character slot, default 1) and ILB_LOBBY
// (server address, default 127.0.0.1:27960). Does not return when it works.
#include "net_lobby.h"

const char *net_session_lobby_address( void )
{
    static char address[ 128 ];
    FILE *file;
    if ( getenv( "ILB_LOBBY" ) ) return getenv( "ILB_LOBBY" );
    if ( address[ 0 ] == 0 && ( file = fopen( "lobby.txt", "r" ) ) != 0 )
    {
        if ( fgets( address, sizeof address, file ) ) address[ strcspn( address, "\r\n " ) ] = 0;
        fclose( file );
    }
    return address[ 0 ] ? address : "127.0.0.1:27960";
}

const char *net_session_player_name( void )
{
    const char *name = getenv( "ILB_PLAYER" ) ? getenv( "ILB_PLAYER" ) : getenv( "USER" );
    if ( name == 0 ) name = getenv( "USERNAME" );       // Windows
    return name && name[ 0 ] ? name : "Player";
}

// Where this copy's session folder goes: next to the run folder, one per role so that two copies on one
// machine do not share it.
void net_session_root( int is_host, char *root, int size )
{
    char here[ 900 ];
    if ( getcwd( here, sizeof here ) == 0 ) strcpy( here, "." );
    snprintf( root, (size_t)size, "%s/../net/session_%s", here, is_host ? "host" : "join" );
}

void net_session_from_environment( char **argv )
{
    const char *hosting = getenv( "ILB_LOBBY_HOST" ), *joining = getenv( "ILB_LOBBY_JOIN" ), *password = getenv( "ILB_LOBBY_PASSWORD" );
    int slot = getenv( "ILB_SLOT" ) ? atoi( getenv( "ILB_SLOT" ) ) : 1;
    char error[ 200 ] = "", root[ 1024 ], other[ 100 ] = "";
    unsigned int seed = 0;
    if ( getenv( "ILB_NET_FD" ) || ( hosting == 0 && joining == 0 ) )
        return;
    net_session_root( hosting != 0, root, sizeof root );
    if ( !lobby_connect( net_session_lobby_address(), net_session_player_name(), error, sizeof error ) )
        goto failed;
    if ( hosting )
    {
        int state;
        if ( !lobby_host( hosting, password, error, sizeof error ) ) goto failed;
        fprintf( stderr, "net: hosting \"%s\"%s, waiting for a player\n", hosting, password && password[ 0 ] ? " (password protected)" : "" );
        while ( ( state = lobby_wait( other, sizeof other ) ) == 0 ) usleep( 200000 );
        if ( state < 0 ) { snprintf( error, sizeof error, "the game server is gone" ); goto failed; }
        fprintf( stderr, "net: %s joined\n", other );
    }
    else
    {
        lobby_game games[ 64 ];
        int count = lobby_list( games, 64, error, sizeof error ), found = -1;
        for ( int i = 0; i < count; i++ )
            if ( strcmp( games[ i ].name, joining ) == 0 ) found = i;
        if ( count < 0 ) goto failed;
        if ( found < 0 ) { snprintf( error, sizeof error, "no game called \"%s\" on the server (%d on offer)", joining, count ); goto failed; }
        if ( !lobby_join( &games[ found ], password, error, sizeof error ) ) goto failed;
        fprintf( stderr, "net: joined \"%s\" hosted by %s\n", joining, games[ found ].host );
    }
    if ( !net_session_prepare( hosting != 0, slot, root, &seed, error, sizeof error ) ) goto failed;
    net_session_run( hosting != 0, root, seed, argv );
    return;
failed:
    fprintf( stderr, "net: %s\n", error );
    exit( 1 );
}

// The other player has left a session that is otherwise fine: keep this player's progress and restart as
// an ordinary game - a host with its character loaded and its game back on the list (ILB_AUTOHOST_*, picked
// up by port/mod_net_menu.c), a joiner at the title menu. Only returns if the restart fails.
void net_session_continue( int in_step, char **argv )
{
    const char *home = getenv( "ILB_NET_HOME" );
    if ( getenv( "ILB_NET_FD" ) == 0 || home == 0 )
        return;                                 // not a session that came from the lobby
    net_session_keep( in_step );
    if ( net_link_fd() >= 0 ) net_close( net_link_fd() );
    unsetenv( "ILB_NET_FD" );
    unsetenv( "ILB_NET_ROLE" );
    unsetenv( "ILB_NET_SEED" );
    unsetenv( "ILB_NET_KEEP" );
    unsetenv( "ILB_SCRIPT" );
    unsetenv( "ILB_SLOT" );
    unsetenv( "ILB_SLOT2" );
    unsetenv( "ILB_WINDOW_TITLE" );
    if ( chdir( home ) != 0 )
        return;
    unsetenv( "ILB_NET_HOME" );
    restart( argv );
}

// The same two steps without restarting the program (the session begins and ends inside the running game;
// draft/native/native_input.c, ILB_input_net_begin): enter the session folder, and afterwards keep the
// progress, close the connection and go back.
void net_session_enter( const char *session_root )
{
    char path[ 1200 ];
    if ( getcwd( path, sizeof path ) != 0 )
        setenv( "ILB_NET_HOME", path, 1 );
    snprintf( path, sizeof path, "%s/keep.txt", session_root );
    setenv( "ILB_NET_KEEP", path, 1 );
    if ( chdir( session_root ) != 0 )
        fprintf( stderr, "net: cannot enter the session folder %s\n", session_root );
}

void net_session_leave( int in_step )
{
    const char *home = getenv( "ILB_NET_HOME" );
    net_session_keep( in_step );
    if ( net_link_fd() >= 0 ) net_close( net_link_fd() );
    net_link_adopt( -1 );
    if ( home && chdir( home ) != 0 )
        fprintf( stderr, "net: cannot go back to %s\n", home );
    unsetenv( "ILB_NET_HOME" );
    unsetenv( "ILB_NET_KEEP" );
}

// Called by a restarted run when its own picture is on the screen: let the previous run's window go.
void net_session_drop_old_connections( void )
{
    const char *kept = getenv( "ILB_OLD_CONNECTIONS" );
    while ( kept && *kept )
    {
        int fd = atoi( kept );
#ifndef _WIN32
        if ( fd > 2 && fd != net_link_fd() ) close( fd );
#endif
        while ( *kept && *kept != ' ' ) kept++;
        while ( *kept == ' ' ) kept++;
    }
    unsetenv( "ILB_OLD_CONNECTIONS" );
}
