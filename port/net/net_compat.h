// net_compat.h - the few system calls the network files need, spelled once for Linux and once for Windows
// (hand-written). Sockets are plain ints on both; net_start() must run before the first socket call.
#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <direct.h>
#include <stdlib.h>
#include <string.h>

typedef int socklen_t;
#define MSG_NOSIGNAL        0
#define usleep( us )        Sleep( ( us ) / 1000 )
#define mkdir( path, mode ) _mkdir( path )
#define getcwd              _getcwd
#define chdir               _chdir
#define net_close( s )      closesocket( s )

static inline void net_start( void )
{
    static int started;
    WSADATA data;
    if ( !started ) { WSAStartup( MAKEWORD( 2, 2 ), &data ); started = 1; }
}
static inline int net_try_again( void )     // the last socket call failed only because nothing was ready
{
    int error = WSAGetLastError();
    return error == WSAEWOULDBLOCK || error == WSAEINTR || error == WSAETIMEDOUT;
}
static inline int net_interrupted( void )   { return WSAGetLastError() == WSAEINTR; }
static inline int net_receive_now( int s, void *data, int size )     // whatever has arrived, without waiting
{
    u_long on = 1, off = 0;
    int got, error;
    ioctlsocket( s, FIONBIO, &on );
    got = recv( s, data, size, 0 );
    error = WSAGetLastError();
    ioctlsocket( s, FIONBIO, &off );
    WSASetLastError( error );
    return got;
}
static inline void net_timeout( int s, int option, int seconds )
{
    DWORD limit = (DWORD)seconds * 1000;
    setsockopt( s, SOL_SOCKET, option, (const char *)&limit, sizeof limit );
}
static inline int setenv( const char *name, const char *value, int replace )
{
    if ( !replace && getenv( name ) ) return 0;
    return _putenv_s( name, value );
}
static inline int unsetenv( const char *name )  { return _putenv_s( name, "" ); }
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define net_close( s )      close( s )
static inline void net_start( void ) {}
static inline int net_try_again( void )     { return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR; }
static inline int net_interrupted( void )   { return errno == EINTR; }
static inline int net_receive_now( int s, void *data, int size )     // whatever has arrived, without waiting
{
    return (int)recv( s, data, (size_t)size, MSG_DONTWAIT );
}
static inline void net_timeout( int s, int option, int seconds )
{
    struct timeval limit = { seconds, 0 };
    setsockopt( s, SOL_SOCKET, option, &limit, sizeof limit );
}
#endif
