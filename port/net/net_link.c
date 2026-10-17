// net_link.c - one TCP connection between two copies of the game (hand-written; built on its own with the
// system's C headers, because the game's own sources are compiled without them).
//
// Deliberately small: connect or accept once, then send whole buffers and receive whatever has arrived
// without waiting. Everything about what the bytes mean is in draft/native/native_input.c.
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "net_compat.h"

static int link_socket = -1;

static void tune( int s )
{
    int one = 1;
    setsockopt( s, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof one );    // a tick's input must not wait for more data
}

// Wait for the other player to connect to `port`. Returns 1 when connected.
int net_link_host( int port )
{
    struct sockaddr_in address = { 0 };
    int one = 1, listener;
    net_start();
    listener = socket( AF_INET, SOCK_STREAM, 0 );
    if ( listener < 0 ) return 0;
    setsockopt( listener, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof one );
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl( INADDR_ANY );
    address.sin_port = htons( (unsigned short)port );
    if ( bind( listener, (struct sockaddr *)&address, sizeof address ) != 0 || listen( listener, 1 ) != 0 )
    {
        fprintf( stderr, "net: cannot listen on port %d: %s\n", port, strerror( errno ) );
        net_close( listener );
        return 0;
    }
    fprintf( stderr, "net: waiting for the other player on port %d\n", port );
    link_socket = accept( listener, 0, 0 );
    net_close( listener );
    if ( link_socket < 0 ) return 0;
    tune( link_socket );
    return 1;
}

// Connect to a hosting copy at `host`:`port`, trying for about `seconds`. Returns 1 when connected.
int net_link_join( const char *host, int port, int seconds )
{
    struct addrinfo hints = { 0 }, *found = 0;
    char service[ 16 ];
    net_start();
    snprintf( service, sizeof service, "%d", port );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ( getaddrinfo( host, service, &hints, &found ) != 0 || found == 0 )
    {
        fprintf( stderr, "net: cannot find %s\n", host );
        return 0;
    }
    for ( int attempt = 0; attempt < seconds * 4 && link_socket < 0; attempt++ )
    {
        int s = socket( AF_INET, SOCK_STREAM, 0 );
        if ( s >= 0 && connect( s, found->ai_addr, (int)found->ai_addrlen ) == 0 )
            link_socket = s;
        else
        {
            if ( s >= 0 ) net_close( s );
            usleep( 250000 );
        }
    }
    freeaddrinfo( found );
    if ( link_socket < 0 )
    {
        fprintf( stderr, "net: nobody is hosting at %s:%d\n", host, port );
        return 0;
    }
    tune( link_socket );
    return 1;
}

// Send all of it. Returns 0 if the connection is gone.
int net_link_send( const void *data, int size )
{
    const char *p = data;
    while ( size > 0 )
    {
        int sent = (int)send( link_socket, p, size, MSG_NOSIGNAL );
        if ( sent <= 0 )
        {
            if ( sent < 0 && net_try_again() ) { usleep( 1000 ); continue; }
            return 0;
        }
        p += sent;
        size -= sent;
    }
    return 1;
}

// Milliseconds of real time (the game's own clock is a step counter); only differences between two readings mean anything.
unsigned int net_link_ms( void )
{
#ifdef _WIN32
    return (unsigned int)GetTickCount();
#else
    struct timeval now;
    gettimeofday( &now, 0 );
    return (unsigned int)( now.tv_sec * 1000u + now.tv_usec / 1000 );
#endif
}

// Receive up to `size` bytes. With `wait` 0 it returns at once: the number of bytes, 0 for nothing yet,
// -1 if the connection is gone.
int net_link_receive( void *data, int size, int wait )
{
    int got = wait ? (int)recv( link_socket, data, size, 0 ) : net_receive_now( link_socket, data, size );
    if ( got > 0 ) return got;
    if ( got < 0 && net_try_again() ) return 0;
    return -1;
}

// The lobby (net_lobby.c) makes the connection and the game session then carries on over the same socket,
// in the same process or in the one it turns into.
int net_link_fd( void )             { return link_socket; }
void net_link_adopt( int fd )       { link_socket = fd; if ( fd >= 0 ) tune( fd ); }

// Connect to `host`:`port` once, waiting at most `seconds`. Returns the socket, or -1.
int net_link_connect( const char *host, int port, int seconds )
{
    struct addrinfo hints = { 0 }, *found = 0;
    char service[ 16 ];
    int s;
    net_start();
    snprintf( service, sizeof service, "%d", port );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ( getaddrinfo( host, service, &hints, &found ) != 0 || found == 0 )
        return -1;
    s = socket( AF_INET, SOCK_STREAM, 0 );
    if ( s >= 0 )
    {
        net_timeout( s, SO_SNDTIMEO, seconds );     // bounds the connect as well (on Linux)
        if ( connect( s, found->ai_addr, (int)found->ai_addrlen ) != 0 ) { net_close( s ); s = -1; }
    }
    freeaddrinfo( found );
    if ( s >= 0 ) tune( s );
    return s;
}

// Receive exactly `size` bytes, waiting at most `seconds` for each piece. Returns 1, or 0 if it failed.
int net_link_receive_all( void *data, int size, int seconds )
{
    char *p = data;
    net_timeout( link_socket, SO_RCVTIMEO, seconds );
    while ( size > 0 )
    {
        int got = (int)recv( link_socket, p, size, 0 );
        if ( got <= 0 )
        {
            if ( got < 0 && net_interrupted() ) continue;
            return 0;
        }
        p += got;
        size -= got;
    }
    net_timeout( link_socket, SO_RCVTIMEO, 0 );
    return 1;
}
