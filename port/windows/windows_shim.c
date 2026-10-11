// windows_shim.c - what the Windows build needs under the game's platform layer (hand-written; built on its
// own with the Windows C headers, only by tools/build_windows.sh).
//
// The platform layer (draft/native) is written without system headers and uses a handful of names the Linux
// C library has and the Windows one spells differently. They are supplied here. The log that the Linux
// build prints to the terminal goes to ILB_windows.log next to the program.
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef stderr
FILE *stderr;

__attribute__(( constructor )) static void shim_start( void )
{
    stderr = __acrt_iob_func( 2 );
    if ( getenv( "ILB_LOG_CONSOLE" ) == 0 )
        freopen( "ILB_windows.log", "w", stderr );
    setvbuf( stderr, 0, _IOLBF, 4096 );
}

// (the Windows headers have their own inline _snprintf and _vsnprintf, so these two get their link names by hand)
int shim_vsnprintf( char *buffer, size_t size, const char *format, va_list arguments ) __asm__( "__vsnprintf" );
int shim_snprintf( char *buffer, size_t size, const char *format, ... ) __asm__( "__snprintf" );
int shim_vsnprintf( char *buffer, size_t size, const char *format, va_list arguments )
{
    return vsnprintf( buffer, size, format, arguments );     // the C99 one: always terminates, returns the full length
}
int shim_snprintf( char *buffer, size_t size, const char *format, ... )
{
    va_list arguments;
    int length;
    va_start( arguments, format );
    length = vsnprintf( buffer, size, format, arguments );
    va_end( arguments );
    return length;
}

int strcasecmp( const char *a, const char *b )      { return _stricmp( a, b ); }
int setenv( const char *name, const char *value, int replace )
{
    if ( !replace && getenv( name ) ) return 0;
    return _putenv_s( name, value );
}
int unsetenv( const char *name )                    { return _putenv_s( name, "" ); }

// The crash report of the debug builds hangs on Unix signals; there is none on Windows.
int sigaction( int signal, const void *action, void *old_action )
{
    (void)signal; (void)action; (void)old_action;
    return 0;
}
