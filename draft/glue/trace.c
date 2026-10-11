// Debugging aid for the mechanical draft, used only in a trace build (DRAFT_TRACE=1 tools/draft_symbols.sh):
// every draft function then calls __cyg_profile_func_enter on entry. Each call of the game's assert handler
// is written at once to ILB_trace.txt (the game's own debug log only exists after the log buffer is
// allocated and only reaches the disk at exit), together with the functions entered just before it.
// tools/draft_trace.py turns the addresses into names.
#include "ILB_draft_decls.h"

typedef struct _iobuf FILE;
FILE *fopen( const char *, const char * );
size_t fwrite( const void *, size_t, size_t, FILE * );
int fflush( FILE * );

#define NO_TRACE __attribute__(( no_instrument_function ))
#define RING 64

static void *ring[ RING ];
static unsigned int ring_at, entered;
static FILE *out;

NO_TRACE static void put( const char *text )
{
    fwrite( text, 1, strlen( text ), out );
}

NO_TRACE static void put_hex( unsigned int value )
{
    char text[ 9 ];
    for ( int i = 0; i < 8; i++ )
        text[ i ] = "0123456789abcdef"[ ( value >> ( 28 - 4 * i ) ) & 15 ];
    text[ 8 ] = 0;
    put( text );
}

// ---- watched functions: ILB_watch.txt next to the exe lists addresses (hex, one per line; written by
// tools/draft_watch.py from function names). Each entry of a watched function is logged with its call site
// and its first four stack arguments, up to a limit per function.
#define MAX_WATCH 32
static void *watched[ MAX_WATCH ];
static unsigned int watch_count[ MAX_WATCH ], num_watched;
size_t fread( void *, size_t, size_t, FILE * );
int fclose( FILE * );
NO_TRACE static void put( const char *text );
NO_TRACE static void put_hex( unsigned int value );

NO_TRACE static void load_watch_list( void )
{
    static char text[ 1024 ];
    FILE *file = fopen( "ILB_watch.txt", "r" );
    if ( file == 0 )
        return;
    size_t size = fread( text, 1, sizeof( text ) - 1, file );
    fclose( file );
    unsigned int value = 0, digits = 0;
    for ( size_t i = 0; i <= size; i++ )
    {
        char c = i < size ? text[ i ] : '\n';
        int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        if ( digit >= 0 ) { value = value * 16 + digit; digits++; continue; }
        if ( digits != 0 && num_watched < MAX_WATCH )
            watched[ num_watched++ ] = (void *)value;
        value = digits = 0;
    }
}

NO_TRACE void __cyg_profile_func_enter( void *function, void *call_site )
{
    ring[ ring_at++ % RING ] = function;
    entered++;
    for ( unsigned int i = 0; i < num_watched; i++ )
    {
        if ( watched[ i ] != function || watch_count[ i ]++ >= 40 )
            continue;
        if ( out == 0 )
            out = fopen( "ILB_trace.txt", "w" );
        if ( out == 0 )
            break;
        // this function's frame -> the watched function's frame -> its stack arguments
        const unsigned int *frame = *(const unsigned int **)__builtin_frame_address( 0 );
        put( "ENTER " ); put_hex( (unsigned int)function ); put( " from " ); put_hex( frame[ 1 ] ); put( " args" );
        for ( unsigned int a = 0; a < 4; a++ ) { put( " " ); put_hex( frame[ 2 + a ] ); }
        put( "\n" );
        fflush( out );
    }
    if ( function != (void *)assert_failed )
        return;
    if ( out == 0 )
        out = fopen( "ILB_trace.txt", "w" );
    if ( out == 0 )
        return;
    // ecx and edx (the expression and the file) are gone by now; the call site identifies the assert
    put( "ASSERT from " ); put_hex( (unsigned int)call_site );
    put( " after " ); put_hex( entered ); put( " calls; entered before it:" );
    for ( unsigned int i = 1; i < RING && i < ring_at; i++ )
    {
        put( " " );
        put_hex( (unsigned int)ring[ ( ring_at - 1 - i ) % RING ] );
    }
    put( "\n" );
    fflush( out );
}

NO_TRACE void __cyg_profile_func_exit( void *function, void *call_site )
{
}

// ---- Direct3D device creation, logged: the draft only checks the result against S_OK
HRESULT __stdcall real_D3D11CreateDeviceAndSwapChain( IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL *, UINT, UINT,
    const DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D11Device **, D3D_FEATURE_LEVEL *, ID3D11DeviceContext ** )
    __asm__( "_D3D11CreateDeviceAndSwapChain@48" );
int __stdcall IsWindow( void * );
int __stdcall IsWindowVisible( void * );
void *__stdcall GetForegroundWindow( void );

NO_TRACE HRESULT __stdcall ILB_trace_D3D11CreateDeviceAndSwapChain( IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver, HMODULE software, UINT flags,
    const D3D_FEATURE_LEVEL *levels, UINT num_levels, UINT sdk, const DXGI_SWAP_CHAIN_DESC *desc, IDXGISwapChain **swap_chain,
    ID3D11Device **device, D3D_FEATURE_LEVEL *level, ID3D11DeviceContext **context )
{
    HRESULT hr = real_D3D11CreateDeviceAndSwapChain( adapter, driver, software, flags, levels, num_levels, sdk, desc, swap_chain, device, level, context );
    if ( out == 0 )
        out = fopen( "ILB_trace.txt", "w" );
    if ( out != 0 )
    {
        put( "D3D11CreateDeviceAndSwapChain hr=" ); put_hex( (unsigned int)hr );
        put( " driver=" ); put_hex( driver ); put( " flags=" ); put_hex( flags ); put( " sdk=" ); put_hex( sdk );
        put( " sizeof(desc)=" ); put_hex( sizeof( *desc ) );
        put( "\n  desc:" );
        for ( unsigned int i = 0; i < sizeof( *desc ) / 4; i++ ) { put( " " ); put_hex( ( (const unsigned int *)desc )[ i ] ); }
        put( "\n  window=" ); put_hex( (unsigned int)desc->OutputWindow );
        put( " is_window=" ); put_hex( IsWindow( desc->OutputWindow ) );
        put( " visible=" ); put_hex( IsWindowVisible( desc->OutputWindow ) );
        put( " foreground=" ); put_hex( (unsigned int)GetForegroundWindow() );
        put( "\n" );
        fflush( out );
    }
    return hr;
}

// ---- crashes: written from a first-chance exception handler, since a run started from Steam leaves no
// debugger output behind
void *__stdcall AddVectoredExceptionHandler( unsigned int first, int ( __stdcall *handler )( void * ) );
extern char __ImageBase;
static int crashes;

NO_TRACE static int __stdcall on_exception( void *pointers )
{
    unsigned int *record = ( (unsigned int **)pointers )[ 0 ];     // EXCEPTION_RECORD: code, flags, next, address, n, info[]
    unsigned int *context = ( (unsigned int **)pointers )[ 1 ];    // x86 CONTEXT, fields by offset
    unsigned int code = record[ 0 ];
    if ( ( code >> 28 ) != 0xC || crashes++ >= 3 )                  // errors only (not debug output or C++ throws)
        return 0;                                                   // EXCEPTION_CONTINUE_SEARCH
    if ( out == 0 )
        out = fopen( "ILB_trace.txt", "w" );
    if ( out == 0 )
        return 0;
    put( "CRASH code=" ); put_hex( code ); put( " at=" ); put_hex( record[ 3 ] );
    put( " access=" ); put_hex( record[ 5 ] ); put( " address=" ); put_hex( record[ 6 ] );
    put( " image=" ); put_hex( (unsigned int)&__ImageBase );
    put( "\n  eax=" ); put_hex( context[ 0xB0 / 4 ] ); put( " ebx=" ); put_hex( context[ 0xA4 / 4 ] );
    put( " ecx=" ); put_hex( context[ 0xAC / 4 ] ); put( " edx=" ); put_hex( context[ 0xA8 / 4 ] );
    put( " esi=" ); put_hex( context[ 0xA0 / 4 ] ); put( " edi=" ); put_hex( context[ 0x9C / 4 ] );
    put( " ebp=" ); put_hex( context[ 0xB4 / 4 ] ); put( " esp=" ); put_hex( context[ 0xC4 / 4 ] );
    put( "\n  entered:" );
    for ( unsigned int i = 0; i < RING && i < ring_at; i++ ) { put( " " ); put_hex( (unsigned int)ring[ ( ring_at - 1 - i ) % RING ] ); }
    // the chain of saved frame pointers: the program is compiled without optimisation, so every one of its
    // functions keeps one (a DLL in between may break the chain)
    put( "\n  frames:" );
    const unsigned int *frame = (const unsigned int *)context[ 0xB4 / 4 ];
    unsigned int low = context[ 0xC4 / 4 ];
    for ( unsigned int i = 0; i < 40 && (unsigned int)frame >= low && (unsigned int)frame < low + 0x100000 && ( (unsigned int)frame & 3 ) == 0; i++ )
    {
        put( " " ); put_hex( frame[ 1 ] );
        if ( frame[ 0 ] <= (unsigned int)frame )
            break;
        frame = (const unsigned int *)frame[ 0 ];
    }
    put( "\n  stack:" );
    const unsigned int *stack = (const unsigned int *)context[ 0xC4 / 4 ];
    for ( unsigned int i = 0; i < 400; i++ ) { put( " " ); put_hex( stack[ i ] ); }
    put( "\n" );
    fflush( out );
    return 0;
}

// ---- hangs: a second thread looks at the main thread every few seconds and writes where it is whenever
// no frame has been presented in between... the program has no cheap "frame" signal here, so it simply
// writes a sample each time, up to a limit; a run that hangs shows the same place again and again.
void *__stdcall CreateThread( void *, unsigned int, unsigned int ( __stdcall *start )( void * ), void *, unsigned int, unsigned int * );
void *__stdcall GetCurrentProcess( void );
void *__stdcall GetCurrentThread( void );
int __stdcall DuplicateHandle( void *, void *, void *, void **, unsigned int, int, unsigned int );
unsigned int __stdcall SuspendThread( void * );
unsigned int __stdcall ResumeThread( void * );
int __stdcall GetThreadContext( void *, void * );
int __stdcall IsBadReadPtr( const void *, unsigned int );
void __stdcall Sleep( unsigned int );
static void *main_thread;

NO_TRACE static unsigned int __stdcall sampler( void *unused )
{
    static unsigned int context[ 256 ] __attribute__(( aligned( 16 ) ));
    unsigned int last_entered = 0;
    for ( unsigned int samples = 0; samples < 400; )
    {
        Sleep( 3000 );
        SuspendThread( main_thread );
        context[ 0 ] = 0x10007;         // CONTEXT_FULL
        if ( GetThreadContext( main_thread, context ) && out != 0 )
        {
            put( "SAMPLE calls=" ); put_hex( entered - last_entered ); put( " at=" ); put_hex( context[ 0xB8 / 4 ] );
            put( " frames:" );
            const unsigned int *frame = (const unsigned int *)context[ 0xB4 / 4 ];
            unsigned int low = context[ 0xC4 / 4 ];
            for ( unsigned int i = 0; i < 24 && (unsigned int)frame >= low && (unsigned int)frame < low + 0x100000
                  && ( (unsigned int)frame & 3 ) == 0 && !IsBadReadPtr( frame, 8 ); i++ )
            {
                put( " " ); put_hex( frame[ 1 ] );
                if ( frame[ 0 ] <= (unsigned int)frame )
                    break;
                frame = (const unsigned int *)frame[ 0 ];
            }
            put( " entered:" );
            for ( unsigned int i = 0; i < 12 && i < ring_at; i++ ) { put( " " ); put_hex( (unsigned int)ring[ ( ring_at - 1 - i ) % RING ] ); }
            put( "\n" );
            fflush( out );
            samples++;
        }
        last_entered = entered;
        ResumeThread( main_thread );
    }
    return 0;
}

NO_TRACE void ILB_trace_init( void )
{
    if ( out == 0 )
        out = fopen( "ILB_trace.txt", "w" );
    DuplicateHandle( GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &main_thread, 0, 0, 2 );
    CreateThread( 0, 0, sampler, 0, 0, 0 );
    load_watch_list();
    AddVectoredExceptionHandler( 1, on_exception );
}
