// Crash recorder for the native builds (hand-written). The game code is compiled with
// -finstrument-functions, so every function entry lands here and is kept in a ring. When the program dies
// on a signal, the ring is printed (most recent first) before the usual report: for crashes that destroy the
// stack - a jump to a bad address, a smashed return address - it is the only record of where execution was.
// tools/native_trace.py turns the addresses into function names.
#define NO_TRACE __attribute__(( no_instrument_function ))
#define RING 512

typedef struct { void ( *handler )( int ); unsigned long mask[ 32 ]; int flags; void ( *restorer )( void ); } trace_sigaction;
int sigaction( int signal, const void *action, void *old_action );
long write( int fd, const void *data, unsigned long size );

static void *ring[ RING ];
static unsigned int ring_at;
static trace_sigaction previous[ 32 ];

NO_TRACE void __cyg_profile_func_enter( void *function, void *call_site ) { ring[ ring_at++ % RING ] = function; }
NO_TRACE void __cyg_profile_func_exit( void *function, void *call_site ) { }

// The ring for a crash report made elsewhere (port/windows/windows_shim.c): the i-th most recent entry, 0 at the end.
NO_TRACE void *ILB_native_trace_entry( unsigned int i ) { return i < RING && i < ring_at ? ring[ ( ring_at - 1 - i ) % RING ] : 0; }

NO_TRACE static void put( const char *text )
{
    unsigned long n = 0;
    while ( text[ n ] ) n++;
    write( 2, text, n );
}

NO_TRACE static void put_hex( unsigned int value )
{
    char text[ 10 ];
    for ( int i = 0; i < 8; i++ ) text[ i ] = "0123456789abcdef"[ ( value >> ( 28 - 4 * i ) ) & 15 ];
    text[ 8 ] = ' ';
    text[ 9 ] = 0;
    put( text );
}

NO_TRACE static void on_signal( int signal )
{
    static int printed;
    if ( !printed )
    {
        printed = 1;
        put( "native: TRACE signal " ); put_hex( signal ); put( "entered:" );
        for ( unsigned int i = 0; i < RING && i < ring_at; i++ ) { put( " " ); put_hex( (unsigned int)ring[ ( ring_at - 1 - i ) % RING ] ); }
        put( "\n" );
    }
    // hand over to whoever was there before (AddressSanitizer in the debug builds), else die by the signal
    if ( signal < 32 && previous[ signal ].handler && previous[ signal ].handler != (void ( * )( int ))1 )
        sigaction( signal, &previous[ signal ], 0 );
    else
    {
        trace_sigaction dfl = { 0 };
        sigaction( signal, &dfl, 0 );
    }
}

NO_TRACE void ILB_native_trace_init( void )
{
    static const int signals[] = { 11 /*SEGV*/, 4 /*ILL*/, 7 /*BUS*/, 5 /*TRAP*/, 6 /*ABRT*/, 8 /*FPE*/ };
    for ( unsigned int i = 0; i < sizeof( signals ) / sizeof( signals[ 0 ] ); i++ )
    {
        trace_sigaction action = { 0 };
        action.handler = on_signal;
        sigaction( signals[ i ], &action, &previous[ signals[ i ] ] );
    }
}
