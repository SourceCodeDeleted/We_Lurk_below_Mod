// The native Linux debug build's stand-in for Windows, SDL2, FMOD and Direct3D (hand-written).
// Everything not defined here is a do-nothing stub from draft/build/native/generated_stubs.c (weak, so these
// definitions take precedence). No window, no sound, no drawing: the point of this build is to run the
// game's own logic under AddressSanitizer and a debugger. Prototypes must match ILB_draft_decls.h.
#include "ILB_draft_decls.h"

typedef struct _IO_FILE FILE;
extern FILE *stderr;
FILE *fopen( const char *, const char * );
int fclose( FILE * );
size_t fread( void *, size_t, size_t, FILE * );
int fseek( FILE *, long, int );
long ftell( FILE * );
int fprintf( FILE *, const char *, ... );
int snprintf( char *, size_t, const char *, ... );
int vsnprintf( char *, size_t, const char *, va_list );
void *calloc( size_t, size_t );
int strcasecmp( const char *, const char * );
long long atoll( const char * );
long time( long * );
double sin( double ); double cos( double ); double sqrt( double );

// ---------------------------------------------------------------- frame script (draft/native/main.c)
void ILB_native_frame( unsigned int frame );
static unsigned int frames;

// ---------------------------------------------------------------- 16-bit wide strings
size_t wcslen( const wchar_t *s )                       { size_t n = 0; while ( s[ n ] ) n++; return n; }
wchar_t *wcscpy( wchar_t *d, const wchar_t *s )         { wchar_t *r = d; while ( ( *d++ = *s++ ) != 0 ) ; return r; }
int wcscmp( const wchar_t *a, const wchar_t *b )        { while ( *a && *a == *b ) { a++; b++; } return (int)*a - (int)*b; }

static void narrow( char *out, size_t size, const wchar_t *text )
{
    size_t i = 0;
    for ( ; text && text[ i ] && i + 1 < size; i++ ) out[ i ] = text[ i ] < 128 ? (char)text[ i ] : '?';
    out[ i ] = 0;
}

// ---------------------------------------------------------------- C runtime specials
int __cdecl _stdio_common_vsprintf( unsigned __int64 Options, char *Buffer, size_t BufferCount, const char *Format, _locale_t Locale, va_list ArgList )
{
    int n = vsnprintf( Buffer, BufferCount, Format, ArgList );
    return n < 0 || (size_t)n >= BufferCount ? -1 : n;
}

// Microsoft's legacy wide printf: %s is a wide string, %S a narrow one. Numbers are formatted by the C
// library through a narrow copy of the conversion.
int __cdecl _stdio_common_vswprintf( unsigned __int64 Options, wchar_t *Buffer, size_t BufferCount, const wchar_t *Format, _locale_t Locale, va_list ArgList )
{
    size_t out = 0;
#define EMIT( c ) do { if ( out + 1 < BufferCount ) Buffer[ out ] = ( c ); out++; } while ( 0 )
    for ( const wchar_t *f = Format; *f; f++ )
    {
        if ( *f != '%' ) { EMIT( *f ); continue; }
        char spec[ 32 ], number[ 128 ];
        int n = 0, is_long_long = 0;
        spec[ n++ ] = '%';
        f++;
        while ( *f && n < 24 && ( ( *f >= '0' && *f <= '9' ) || *f == '-' || *f == '+' || *f == ' ' || *f == '#' || *f == '.' ) ) spec[ n++ ] = (char)*f++;
        while ( *f == 'l' || *f == 'h' || *f == 'I' || *f == 'w' ) { if ( *f == 'I' && f[ 1 ] == '6' ) { is_long_long = 1; f += 2; } f++; }
        if ( *f == 0 ) break;
        if ( *f == '%' ) { EMIT( '%' ); continue; }
        if ( *f == 's' ) { const wchar_t *s = va_arg( ArgList, const wchar_t * ); for ( ; s && *s; s++ ) EMIT( *s ); continue; }
        if ( *f == 'S' ) { const char *s = va_arg( ArgList, const char * ); for ( ; s && *s; s++ ) EMIT( (unsigned char)*s ); continue; }
        if ( *f == 'c' ) { EMIT( (wchar_t)va_arg( ArgList, int ) ); continue; }
        number[ 0 ] = 0;
        if ( *f == 'f' || *f == 'g' || *f == 'e' ) { spec[ n++ ] = (char)*f; spec[ n ] = 0; snprintf( number, sizeof( number ), spec, va_arg( ArgList, double ) ); }
        else if ( is_long_long ) { spec[ n++ ] = 'l'; spec[ n++ ] = 'l'; spec[ n++ ] = (char)*f; spec[ n ] = 0; snprintf( number, sizeof( number ), spec, va_arg( ArgList, long long ) ); }
        else { spec[ n++ ] = (char)*f; spec[ n ] = 0; snprintf( number, sizeof( number ), spec, va_arg( ArgList, int ) ); }
        for ( const char *s = number; *s; s++ ) EMIT( (unsigned char)*s );
    }
#undef EMIT
    if ( BufferCount ) Buffer[ out < BufferCount ? out : BufferCount - 1 ] = 0;
    return out < BufferCount ? (int)out : -1;
}

__int64 __cdecl atoi64( const char *String )                        { return atoll( String ); }
__time64_t __cdecl time64( __time64_t *Time )                       { __time64_t t = time( 0 ); if ( Time ) *Time = t; return t; }
int __cdecl stricmp( const char *String1, const char *String2 )     { return strcasecmp( String1, String2 ); }
double libm_sse2_sin_precise( double x )                            { return sin( x ); }
double libm_sse2_cos_precise( double x )                            { return cos( x ); }
double libm_sse2_sqrt_precise( double x )                           { return sqrt( x ); }
void *__cdecl sub_582D30( unsigned int size )                       { return calloc( 1, size ); }      // operator new
void __cdecl j_j_free( void *block )                                { free( block ); }
int __cdecl j___ftol2_sse_excpt_0( double x )                       { return (int)x; }
#ifndef _WIN32      // the Windows C runtime has its own
void __cdecl __report_rangecheckfailure( void )                      { fprintf( stderr, "native: range check failure\n" ); __builtin_trap(); }
#endif

// the per-user data folder; the game appends its own sub-folder and file names
wchar_t *__cdecl wgetenv( const wchar_t *VarName )
{
    static wchar_t folder[] = L"native_appdata";
    return folder;
}

// ---------------------------------------------------------------- Windows
// Files: the game's settings, account and save files, through the C library. Paths arrive as Windows wide
// strings ("native_appdata\\Graybeard Games\\It Lurks Below\\ilbc_1.sav") relative to the run folder.
int mkdir( const char *, unsigned int );
int remove( const char * );
int rename( const char *, const char * );
size_t fwrite( const void *, size_t, size_t, FILE * );

static void file_path( char *path, size_t size, const wchar_t *name, int make_folders )
{
    size_t i = 0;
    for ( ; name && name[ i ] && i + 1 < size; i++ )
        path[ i ] = name[ i ] == '\\' ? '/' : name[ i ] < 128 ? (char)name[ i ] : '_';
    path[ i ] = 0;
    for ( size_t k = 1; make_folders && k < i; k++ )
        if ( path[ k ] == '/' ) { path[ k ] = 0; mkdir( path, 0755 ); path[ k ] = '/'; }
}

HANDLE __stdcall CreateFileW( LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile )
{
    char path[ 1024 ];
    int writing = ( dwDesiredAccess & 0x40000000 ) != 0;           // GENERIC_WRITE
    file_path( path, sizeof( path ), lpFileName, writing );
    // dispositions: 2 create always, 3 open existing, 4 open always
    FILE *f = fopen( path, !writing ? "rb" : dwCreationDisposition == 2 ? "wb" : "r+b" );
    if ( f == 0 && writing && dwCreationDisposition == 4 )
        f = fopen( path, "w+b" );
    return f ? (HANDLE)f : (HANDLE)-1;                              // INVALID_HANDLE_VALUE
}

BOOL __stdcall CloseHandle( HANDLE hObject )
{
    if ( hObject != 0 && hObject != (HANDLE)-1 ) fclose( (FILE *)hObject );
    return 1;
}

DWORD __stdcall GetFileSize( HANDLE hFile, LPDWORD lpFileSizeHigh )
{
    FILE *f = (FILE *)hFile;
    long at = ftell( f ), size;
    fseek( f, 0, 2 );
    size = ftell( f );
    fseek( f, at, 0 );
    if ( lpFileSizeHigh ) *lpFileSizeHigh = 0;
    return (DWORD)size;
}

BOOL __stdcall ReadFile( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped )
{
    size_t n = fread( lpBuffer, 1, nNumberOfBytesToRead, (FILE *)hFile );
    if ( lpNumberOfBytesRead ) *lpNumberOfBytesRead = n;
    return 1;
}

BOOL __stdcall WriteFile( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped )
{
    size_t n = fwrite( lpBuffer, 1, nNumberOfBytesToWrite, (FILE *)hFile );
    if ( lpNumberOfBytesWritten ) *lpNumberOfBytesWritten = n;
    return n == nNumberOfBytesToWrite;
}

BOOL __stdcall CreateDirectoryW( LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
    char path[ 1024 ];
    file_path( path, sizeof( path ), lpPathName, 1 );
    return mkdir( path, 0755 ) == 0;
}

BOOL __stdcall DeleteFileW( LPCWSTR lpFileName )
{
    char path[ 1024 ];
    file_path( path, sizeof( path ), lpFileName, 0 );
    return remove( path ) == 0;
}

BOOL __stdcall MoveFileW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName )
{
    char from[ 1024 ], to[ 1024 ];
    file_path( from, sizeof( from ), lpExistingFileName, 0 );
    file_path( to, sizeof( to ), lpNewFileName, 1 );
    return rename( from, to ) == 0;
}

BOOL __stdcall CopyFileW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists )
{
    char from[ 1024 ], to[ 1024 ], block[ 65536 ];
    size_t n;
    file_path( from, sizeof( from ), lpExistingFileName, 0 );
    file_path( to, sizeof( to ), lpNewFileName, 1 );
    FILE *in = fopen( from, "rb" ), *out_file = in ? fopen( to, "wb" ) : 0;
    if ( out_file == 0 ) { if ( in ) fclose( in ); return 0; }
    while ( ( n = fread( block, 1, sizeof( block ), in ) ) > 0 ) fwrite( block, 1, n, out_file );
    fclose( in );
    fclose( out_file );
    return 1;
}

DWORD __stdcall GetCurrentDirectoryW( DWORD nBufferLength, LPWSTR lpBuffer )
{
    if ( nBufferLength < 2 ) return 0;
    lpBuffer[ 0 ] = '.';
    lpBuffer[ 1 ] = 0;
    return 1;
}

// no install path in the registry: the game falls back to the current directory
LSTATUS __stdcall RegOpenKeyExW( HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult ) { return 2; }

int __stdcall MessageBoxW( HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType )
{
    char caption[ 256 ], text[ 2048 ];
    narrow( caption, sizeof( caption ), lpCaption );
    narrow( text, sizeof( text ), lpText );
    fprintf( stderr, "native: MessageBox [%s] %s\n", caption, text );
    return 1;
}

HWND __stdcall GetActiveWindow()                    { return (HWND)0x1000; }
HDC __stdcall GetDC( HWND hWnd )                    { return (HDC)0x2000; }
HDC __stdcall CreateCompatibleDC( HDC hdc )         { return (HDC)0x3000; }
HBITMAP __stdcall CreateDIBSection( HDC hdc, const BITMAPINFO *pbmi, UINT usage, void **ppvBits, HANDLE hSection, DWORD offset )
{
    *ppvBits = calloc( 1, 64 << 20 );       // the software surface; the game clears 3 MB of it
    return (HBITMAP)0x4000;
}

// ---------------------------------------------------------------- SDL2
static unsigned int clock_ms;
DWORD __stdcall GetTickCount()                      { return clock_ms; }

static void unix_path( char *path, size_t size, const char *file )
{
    size_t i = 0;
    for ( ; *file && i + 1 < size; file++ )
        path[ i++ ] = *file == '\\' ? '/' : *file;      // the game builds Windows paths
    path[ i ] = 0;
}

#ifdef ILB_WINDOW
// ---- window build: the real SDL2 is linked. Two calls are intercepted by the linker (--wrap): file paths
// need their separators changed, and the event poll is where the frame hook and the OpenGL context live.
int __cdecl __real_SDL_RWFromFile( _DWORD file, _DWORD mode );
int __cdecl __real_SDL_PollEvent( _DWORD event );
int __cdecl __real_SDL_CreateWindow( _DWORD title, _DWORD x, _DWORD y, _DWORD w, _DWORD h, _DWORD flags );
void *SDL_GL_CreateContext( void *window );
void SDL_GL_SwapWindow( void *window );
int SDL_GL_SetSwapInterval( int interval );
const char *SDL_GetError( void );
unsigned int __real_SDL_GetTicks( void );

void *ILB_window;
int ILB_cheat_key( int key, int pressed );
int ILB_input_active( void );
int ILB_input_poll( int ( *real_poll )( void * ), void *event );
const unsigned char *ILB_input_keys( void );
int ILB_input_mouse( int *x, int *y );
unsigned int ILB_input_seed( unsigned int wanted );
int ILB_mod_local_coop_key( int key, int pressed );
int gge_gl_load( void );
int gge_gl_build_programs( void );

int __cdecl __wrap_SDL_RWFromFile( _DWORD file, _DWORD mode )
{
    char path[ 1024 ];
    unix_path( path, sizeof( path ), (const char *)file );
    return __real_SDL_RWFromFile( (_DWORD)path, mode );
}

int __cdecl __wrap_SDL_CreateWindow( _DWORD title, _DWORD x, _DWORD y, _DWORD w, _DWORD h, _DWORD flags )
{
    // ILB_WINDOW_TITLE / ILB_WINDOW_X / ILB_WINDOW_Y: for two copies on one screen (tools/net_game.sh), each
    // says in its title bar which player it is and sits where the script puts it
    if ( getenv( "ILB_WINDOW_TITLE" ) ) title = (_DWORD)getenv( "ILB_WINDOW_TITLE" );
    if ( getenv( "ILB_WINDOW_X" ) ) x = atoi( getenv( "ILB_WINDOW_X" ) );
    if ( getenv( "ILB_WINDOW_Y" ) ) y = atoi( getenv( "ILB_WINDOW_Y" ) );
    ILB_window = (void *)__real_SDL_CreateWindow( title, x, y, w, h, flags | 2 );      // SDL_WINDOW_OPENGL
    if ( ILB_window == 0 || SDL_GL_CreateContext( ILB_window ) == 0 )
        fprintf( stderr, "native: no OpenGL window: %s\n", SDL_GetError() );
    else
    {
        SDL_GL_SetSwapInterval( 1 );
        if ( gge_gl_load() )
            gge_gl_build_programs();
    }
    return (int)ILB_window;
}

// The game paints its window black through SDL's software "window surface" in a few places. On a window
// that OpenGL draws to, that path fights with the OpenGL context (it crashed inside the graphics driver), and
// it is not needed: the renderer clears the window. The three calls do nothing here.
// ... and stays there: the game centres its window when it applies the display settings.
void __real_SDL_SetWindowPosition( _DWORD window, _DWORD x, _DWORD y );
void __cdecl __wrap_SDL_SetWindowPosition( _DWORD window, _DWORD x, _DWORD y )
{
    if ( getenv( "ILB_WINDOW_X" ) ) x = atoi( getenv( "ILB_WINDOW_X" ) );
    if ( getenv( "ILB_WINDOW_Y" ) ) y = atoi( getenv( "ILB_WINDOW_Y" ) );
    __real_SDL_SetWindowPosition( window, x, y );
}

void ILB_native_window_position( int *x, int *y )       // for a copy about to restart itself (port/net/net_session.c)
{
    if ( ILB_window ) SDL_GetWindowPosition( (_DWORD)ILB_window, (_DWORD)x, (_DWORD)y );
}

// A run that was started to go straight into a game (a network session, or a host carrying on alone after
// one) passes through the title menu and the character screen on its way, driven by a script. The player
// should not see those flash by: until the loading screen is reached the window stays black, and at that
// moment the previous run's window, kept up behind this one, is finally let go.
void net_session_drop_old_connections( void );
//
// A host whose game is being switched into a two-player session inside the running program (or back) passes
// through the same screens; its window simply keeps the last picture of the game until the game is back.
static int frozen;
void ILB_native_freeze_picture( int on ) { frozen = on; }

int ILB_native_hide_picture( void )         // 0 show, 1 black, 2 leave what is on the screen
{
    static int visible;
    static unsigned int steps_in_game;
    int app_mode = *(int *)ILB_ADDR( 0x3A73CC0 );
    if ( frozen == 2 )                      // a joiner: until its loading screen
    {
        if ( app_mode != 10 && app_mode != 11 )
            return 2;
        frozen = 0;
    }
    if ( frozen )                           // a host: until its game is back
    {
        steps_in_game = app_mode == 11 ? steps_in_game + 1 : 0;
        if ( steps_in_game < 3 )            // the first pictures of the game are still being set up
            return 2;
        frozen = 0;
        steps_in_game = 0;
    }
    if ( visible || ( getenv( "ILB_NET_FD" ) == 0 && getenv( "ILB_AUTOHOST_NAME" ) == 0 ) )
        return 0;
    if ( app_mode == 10 || app_mode == 11 )             // loading, or in the game
    {
        visible = 1;
        net_session_drop_old_connections();
        return 0;
    }
    return 1;
}

int __cdecl __wrap_SDL_GetWindowSurface( _DWORD window )    { static int surface[ 64 ]; return (int)surface; }
int __cdecl __wrap_SDL_FillRect( _DWORD surface, _DWORD rect, _DWORD color ) { return 0; }
int __cdecl __wrap_SDL_UpdateWindowSurface( _DWORD window ) { return 0; }

// While input is being recorded or replayed, the keyboard and mouse state the game asks for is the one
// rebuilt from the events it has been given (native_input.c).
int __real_SDL_GetMouseState( _DWORD x, _DWORD y );
int __real_SDL_GetKeyboardState( _DWORD numkeys );
int __cdecl __wrap_SDL_GetMouseState( _DWORD x, _DWORD y )
{
    return ILB_input_active() ? ILB_input_mouse( (int *)x, (int *)y ) : __real_SDL_GetMouseState( x, y );
}
int __cdecl __wrap_SDL_GetKeyboardState( _DWORD numkeys )
{
    if ( !ILB_input_active() )
        return __real_SDL_GetKeyboardState( numkeys );
    if ( numkeys ) *(int *)numkeys = 512;
    return (int)ILB_input_keys();
}

// The controller: the game's questions about the device go through the recording (native_input.c), so a
// replay - and later a second player on another machine - needs no controller here. One wrapper per SDL
// function the game uses; the first argument of ILB_input_query only has to be a different number each.
int ILB_input_query( int kind, int real_value, const char *real_text, const char **text );
int ILB_input_pad_axis( int axis );
int ILB_input_replaying( void );
int ILB_input_net_guest( void );
void ILB_mod_net_menu_background( void );
int ILB_input_net_session( void );
int __real_SDL_NumJoysticks( void );
int __real_SDL_IsGameController( _DWORD index );
int __real_SDL_GameControllerOpen( _DWORD index );
int __real_SDL_GameControllerGetJoystick( _DWORD controller );
int __real_SDL_JoystickInstanceID( _DWORD joystick );
int __real_SDL_GameControllerGetVendor( _DWORD controller );
int __real_SDL_GameControllerName( _DWORD controller );
int __real_SDL_GameControllerGetAxis( _DWORD controller, _DWORD axis );
int __real_SDL_GameControllerClose( _DWORD controller );
#define LIVE ( !ILB_input_replaying() )
int __wrap_SDL_NumJoysticks( void )                             { return ILB_input_query( 1, LIVE ? __real_SDL_NumJoysticks() : 0, 0, 0 ); }
int __cdecl __wrap_SDL_IsGameController( _DWORD index )         { return ILB_input_query( 2, LIVE ? __real_SDL_IsGameController( index ) : 0, 0, 0 ); }
int __cdecl __wrap_SDL_GameControllerOpen( _DWORD index )
{
    int real = LIVE ? __real_SDL_GameControllerOpen( index ) : 0;
    int opened = ILB_input_query( 3, real != 0, 0, 0 );
    // any copy of a network game: its player may pick up the controller at any time, and SDL reports a controller's
    // buttons and sticks only once it has been opened
    if ( ILB_input_net_session() )
    {
        int found = __real_SDL_NumJoysticks(), mine = 0;
        for ( int i = 0; i < found && mine == 0; i++ )
            if ( __real_SDL_IsGameController( i ) )
                mine = __real_SDL_GameControllerOpen( i );
        fprintf( stderr, "native: controllers on this copy: %d found%s%s\n", found, mine ? ", using " : "",
                 mine ? (const char *)__real_SDL_GameControllerName( mine ) : "" );
    }
    return LIVE ? real : opened;                                // replaying: 1 stands for "a controller"
}
int __cdecl __wrap_SDL_GameControllerGetJoystick( _DWORD controller )   { return LIVE ? __real_SDL_GameControllerGetJoystick( controller ) : 1; }
int __cdecl __wrap_SDL_JoystickInstanceID( _DWORD joystick )    { return ILB_input_query( 4, LIVE ? __real_SDL_JoystickInstanceID( joystick ) : 0, 0, 0 ); }
int __cdecl __wrap_SDL_GameControllerGetVendor( _DWORD controller ) { return ILB_input_query( 5, LIVE ? __real_SDL_GameControllerGetVendor( controller ) : 0, 0, 0 ); }
int __cdecl __wrap_SDL_GameControllerName( _DWORD controller )
{
    const char *name;
    ILB_input_query( 6, 0, LIVE ? (const char *)__real_SDL_GameControllerName( controller ) : "", &name );
    return (int)name;
}
int __cdecl __wrap_SDL_GameControllerGetAxis( _DWORD controller, _DWORD axis )
{
    return ILB_input_active() ? ILB_input_pad_axis( axis ) : __real_SDL_GameControllerGetAxis( controller, axis );
}
int __cdecl __wrap_SDL_GameControllerClose( _DWORD controller )  { return LIVE ? __real_SDL_GameControllerClose( controller ) : 0; }
#undef LIVE

int __cdecl __wrap_SDL_PollEvent( _DWORD event )
{
    int pending = ILB_input_poll( (int ( * )( void * ))__real_SDL_PollEvent, (void *)event );   // records or replays (native_input.c)
#ifdef ILB_CHEATS
    // testing shortcuts (port/cheats.c): SDL_KEYDOWN is 0x300 and SDL_KEYUP 0x301, the key's code is the
    // sixth 32-bit field. A key the shortcuts take is not passed on to the game.
    while ( pending && ( ( (const unsigned int *)event )[ 0 ] == 0x300 || ( (const unsigned int *)event )[ 0 ] == 0x301 )
            && ILB_cheat_key( ( (const int *)event )[ 5 ],
                              ( (const unsigned int *)event )[ 0 ] == 0x300 && ( (const unsigned char *)event )[ 13 ] == 0 ) )
        pending = ILB_input_poll( (int ( * )( void * ))__real_SDL_PollEvent, (void *)event );
#endif
#ifdef ILB_MOD_LOCAL_COOP
    // port/mod_local_coop.c: F5 on the title menu opens the game's own Local Multiplayer set-up screen
    while ( pending && ( ( (const unsigned int *)event )[ 0 ] == 0x300 || ( (const unsigned int *)event )[ 0 ] == 0x301 )
            && ILB_mod_local_coop_key( ( (const int *)event )[ 5 ],
                                       ( (const unsigned int *)event )[ 0 ] == 0x300 && ( (const unsigned char *)event )[ 13 ] == 0 ) )
        pending = ILB_input_poll( (int ( * )( void * ))__real_SDL_PollEvent, (void *)event );
#endif
#ifdef ILB_MOD_NET_MENU
    if ( !pending )
        ILB_mod_net_menu_background();      // port/mod_net_menu.c: a host playing alone watches for a joiner
#endif
    if ( !pending )
        ILB_native_frame( frames++ );       // the queue is empty once per frame
    return pending;
}

#else
// ---- headless build: SDL stand-ins. No window, no events; time moves on by a 60th of a second per frame.
int __cdecl SDL_Init( _DWORD flags )                { return 0; }
int __cdecl SDL_CreateWindow( _DWORD title, _DWORD x, _DWORD y, _DWORD w, _DWORD h, _DWORD flags ) { return 0x5000; }
int __cdecl SDL_GetWindowSurface( _DWORD window )   { return (int)calloc( 1, 256 ); }
int __cdecl SDL_memset( _DWORD dest, _DWORD value, _DWORD size ) { return (int)memset( (void *)dest, value, size ); }
int __cdecl SDL_GetTicks()                          { return clock_ms; }

// SDL_DisplayMode: format, w, h, refresh_rate, driverdata
static int display_mode( _DWORD mode )
{
    int *m = (int *)mode;
    m[ 0 ] = 0; m[ 1 ] = 1920; m[ 2 ] = 1080; m[ 3 ] = 60; m[ 4 ] = 0;
    return 0;
}
int __cdecl SDL_GetDesktopDisplayMode( _DWORD display, _DWORD mode )    { return display_mode( mode ); }
int __cdecl SDL_GetCurrentDisplayMode( _DWORD display, _DWORD mode )    { return display_mode( mode ); }

// Scripted input for the headless build: ILB_FIRE="frame x y" holds the left mouse button down at (x, y)
// from that frame on, as a player aiming a wand would. Events have SDL2's layout: type, time, window,
// device, then for a button: button (byte), state, clicks, padding, x, y; for motion the same x, y places.
static int fire_frame = -1, fire_x, fire_y, fire_sent;
int sscanf( const char *, const char *, ... );
char *getenv( const char * );

int __cdecl SDL_PollEvent( _DWORD event )
{
    unsigned int *e = (unsigned int *)event;
    if ( fire_frame == -1 )
    {
        const char *plan = getenv( "ILB_FIRE" );
        fire_frame = -2;
        if ( plan ) sscanf( plan, "%d %d %d", &fire_frame, &fire_x, &fire_y );
    }
    if ( fire_frame >= 0 && (int)frames >= fire_frame && fire_sent < 2 )
    {
        memset( e, 0, 56 );
        e[ 0 ] = fire_sent == 0 ? 0x400 : 0x401;        // SDL_MOUSEMOTION, then SDL_MOUSEBUTTONDOWN
        e[ 5 ] = fire_x;
        e[ 6 ] = fire_y;
        if ( fire_sent == 1 ) { ( (unsigned char *)e )[ 16 ] = 1; ( (unsigned char *)e )[ 17 ] = 1; ( (unsigned char *)e )[ 18 ] = 1; }
        fire_sent++;
        return 1;
    }
    if ( ILB_input_poll( 0, e ) )           // a recording being replayed (native_input.c)
        return 1;
    clock_ms += 16;
    ILB_native_frame( frames++ );
    return 0;
}

int __cdecl SDL_GetMouseState( _DWORD x, _DWORD y )
{
    int held = fire_frame >= 0 && (int)frames >= fire_frame;
    if ( ILB_input_active() )
        return ILB_input_mouse( (int *)x, (int *)y );
    if ( x ) *(int *)x = held ? fire_x : 0;
    if ( y ) *(int *)y = held ? fire_y : 0;
    return held ? 1 : 0;        // SDL_BUTTON_LMASK
}

int __cdecl SDL_GetKeyboardState( _DWORD numkeys )
{
    if ( numkeys ) *(int *)numkeys = 512;
    return (int)ILB_input_keys();
}

// files (the pack file is read through these)
int __cdecl SDL_RWFromFile( _DWORD file, _DWORD mode )
{
    char path[ 1024 ];
    unix_path( path, sizeof( path ), (const char *)file );
    FILE *f = fopen( path, (const char *)mode );
    if ( f == 0 )
        fprintf( stderr, "native: cannot open %s\n", path );
    return (int)f;
}
int __cdecl SDL_RWsize( _DWORD context )
{
    FILE *f = (FILE *)context;
    long at = ftell( f ), size;
    fseek( f, 0, 2 );
    size = ftell( f );
    fseek( f, at, 0 );
    return (int)size;
}
int __cdecl SDL_RWread( _DWORD context, _DWORD ptr, _DWORD size, _DWORD maxnum ) { return (int)fread( (void *)ptr, size, maxnum, (FILE *)context ); }
int __cdecl SDL_RWclose( _DWORD context )               { return fclose( (FILE *)context ); }

int __cdecl SDL_ShowSimpleMessageBox( _DWORD flags, _DWORD title, _DWORD message, _DWORD window )
{
    fprintf( stderr, "native: SDL message box [%s] %s\n", (const char *)title, (const char *)message );
    return 0;
}

int __cdecl SDL_GetKeyName( _DWORD key )               { return (int)"?"; }         // drawn on the hotbar
int __cdecl SDL_GameControllerName( _DWORD controller ) { return (int)""; }

// what the OpenGL backend asks SDL for
void *ILB_window;
void SDL_GL_SwapWindow( void *window ) { }
void SDL_GL_GetDrawableSize( void *window, int *w, int *h ) { *w = 1920; *h = 1080; }
void *SDL_GL_GetProcAddress( const char *name ) { return 0; }
#endif

// ---------------------------------------------------------------- FMOD
// window build: port/snd_mixer.c. Headless: silent, but the system object must exist.
#ifndef ILB_WINDOW
int __stdcall FMOD_System_Create( _DWORD system )
{
    *(void **)system = calloc( 1, 64 );
    return 0;
}
#endif

// ---------------------------------------------------------------- random numbers and the hang detector
// Microsoft's rand(): 15-bit results. The game's generators divide by 32768 in places, so the C library's
// 31-bit rand() would not do.
static unsigned int rand_state = 1, rand_calls;
void ILB_native_stuck( void );

// ILB_SEED=<n>: the game seeds from the time of day; a fixed seed makes a run repeatable (determinism tests)
void __cdecl srand( unsigned int seed ) { rand_state = ILB_input_seed( getenv( "ILB_SEED" ) ? (unsigned int)atoi( getenv( "ILB_SEED" ) ) : seed ); }
unsigned int ILB_native_rand_state( void ) { return rand_state; }
void ILB_native_set_seed( unsigned int seed ) { rand_state = seed; rand_calls = 0; }
int __cdecl rand( void )
{
    if ( ++rand_calls == 200000000 )        // far more than a whole world needs: some search is not ending
        ILB_native_stuck();
    rand_state = rand_state * 214013 + 2531011;
    return ( rand_state >> 16 ) & 0x7FFF;
}

// ---------------------------------------------------------------- the game's asserts, made visible
// The linker redirects calls to assert_failed here (--wrap). Each place is printed the first time it fires,
// with a count at the end of the run; then the game's own handler runs as usual.
char __fastcall __real_assert_failed( const char *expr, const char *file, const char *func, int line, char show_stack );
static struct { const char *file; int line; unsigned int count; } assert_sites[ 512 ];
static unsigned int num_assert_sites;

char __fastcall __wrap_assert_failed( const char *expr, const char *file, const char *func, int line, char show_stack )
{
    unsigned int i = 0;
    while ( i < num_assert_sites && !( assert_sites[ i ].file == file && assert_sites[ i ].line == line ) ) i++;
    if ( i == num_assert_sites && num_assert_sites < 512 )
    {
        assert_sites[ num_assert_sites++ ].file = file;
        assert_sites[ i ].line = line;
        const char *name = file;
        for ( const char *s = file; s && *s; s++ ) if ( *s == '\\' ) name = s + 1;
        fprintf( stderr, "native: ASSERT( %s ) %s:%d in %s (frame %u)\n", expr, name, line, func, frames );
    }
    if ( i < 512 ) assert_sites[ i ].count++;
    return __real_assert_failed( expr, file, func, line, show_stack );
}

void ILB_native_assert_summary( void )
{
    for ( unsigned int i = 0; i < num_assert_sites; i++ )
        if ( assert_sites[ i ].count > 1 )
            fprintf( stderr, "native: assert at line %d fired %u times\n", assert_sites[ i ].line, assert_sites[ i ].count );
    fprintf( stderr, "native: %u different asserts fired\n", num_assert_sites );
}

// ---------------------------------------------------------------- window rectangle
// The game derives its mouse scale from the window's client rectangle (GetWindowInfo on the active window).
// WINDOWINFO: cbSize, rcWindow (left, top, right, bottom), rcClient (the same four), ...
#ifdef ILB_WINDOW
void SDL_GetWindowSize( void *window, int *w, int *h );
#endif
int ILB_input_view_width( int real );
BOOL __stdcall GetWindowInfo( HWND hwnd, PWINDOWINFO pwi )
{
    int *info = (int *)pwi, width = 1920, height = 1080;
#ifdef ILB_WINDOW
    if ( ILB_window ) SDL_GetWindowSize( ILB_window, &width, &height );
#endif
    width = ILB_input_view_width( width );      // one view per copy of a network game: see native_input.c
    info[ 1 ] = info[ 5 ] = 0;
    info[ 2 ] = info[ 6 ] = 0;
    info[ 3 ] = info[ 7 ] = width;
    info[ 4 ] = info[ 8 ] = height;
    return 1;
}
