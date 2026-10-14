// windows_shim.c - what the Windows build needs under the game's platform layer (hand-written; built on its
// own with the Windows C headers, only for the Windows program).
//
// The platform layer (draft/native) is written without system headers and uses a handful of names the Linux
// C library has and the Windows one spells differently. They are supplied here. The log that the Linux
// build prints to the terminal goes to ILB_windows.log next to the program, and a crash, which on Linux the
// signal handler of draft/native/native_trace.c reports, is reported here.
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#undef stderr
FILE *stderr;

// the address the program is linked for (/base in CMakeLists.txt and lld-link's default): the link map, and
// with it tools/windows_where.py, names addresses as they are there, wherever Windows loaded the program
#define LINKED_BASE 0x400000u

static char log_path[ MAX_PATH ] = "the console";
void *ILB_native_trace_entry( unsigned int i );     // draft/native/native_trace.c

static int in_program_code( unsigned int address, unsigned int base )
{
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)base;
    const IMAGE_NT_HEADERS *nt = (const IMAGE_NT_HEADERS *)( base + dos->e_lfanew );
    const IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION( nt );
    for ( unsigned int i = 0; i < nt->FileHeader.NumberOfSections; i++ )
        if ( ( section[ i ].Characteristics & IMAGE_SCN_MEM_EXECUTE ) && address >= base + section[ i ].VirtualAddress
             && address < base + section[ i ].VirtualAddress + section[ i ].Misc.VirtualSize )
            return 1;
    return 0;
}

static void crash_box( void )
{
    char text[ MAX_PATH + 200 ];
    if ( getenv( "ILB_NO_CRASH_BOX" ) ) return;
    snprintf( text, sizeof text, "The game has stopped because of an error.\n\nWhat happened is written at the end of\n%s", log_path );
    MessageBoxA( 0, text, "It Lurks Below", MB_OK | MB_ICONERROR | MB_SETFOREGROUND );
}

// Every address is printed as the link map has it. "stack" is each value on the stack that points into the
// program's code: the return addresses, callers outward, with some stale ones in between.
static LONG WINAPI on_crash( EXCEPTION_POINTERS *info )
{
    static LONG reported;
    const EXCEPTION_RECORD *record = info->ExceptionRecord;
    const CONTEXT *c = info->ContextRecord;
    unsigned int base = (unsigned int)GetModuleHandleA( 0 ), shift = LINKED_BASE - base;
    if ( InterlockedExchange( &reported, 1 ) ) return EXCEPTION_EXECUTE_HANDLER;
    fprintf( stderr, "native: CRASH exception %08lx at %08x%s", record->ExceptionCode, (unsigned int)record->ExceptionAddress + shift,
             in_program_code( (unsigned int)record->ExceptionAddress, base ) ? "" : " (not in the program: address as it is, unshifted, is " );
    if ( !in_program_code( (unsigned int)record->ExceptionAddress, base ) )
        fprintf( stderr, "%08x)", (unsigned int)record->ExceptionAddress );
    if ( record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2 )
        fprintf( stderr, ", %s address %08lx", record->ExceptionInformation[ 0 ] == 0 ? "reading" : record->ExceptionInformation[ 0 ] == 1 ? "writing" : "executing",
                 (unsigned long)record->ExceptionInformation[ 1 ] );
    fprintf( stderr, "\nnative:   program loaded at %08x; eax %08lx ebx %08lx ecx %08lx edx %08lx esi %08lx edi %08lx ebp %08lx esp %08lx\n", base,
             c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp );
    fprintf( stderr, "native:   stack:" );
    {
        const unsigned int *stack = (const unsigned int *)c->Esp, *top = (const unsigned int *)( (NT_TIB *)NtCurrentTeb() )->StackBase;
        int shown = 0;
        if ( record->ExceptionCode != EXCEPTION_STACK_OVERFLOW )
            for ( ; stack + 1 <= top && shown < 48; stack++ )
                if ( in_program_code( *stack, base ) ) { fprintf( stderr, " %08x", *stack + shift ); shown++; }
    }
    fprintf( stderr, "\n" );
    if ( ILB_native_trace_entry( 0 ) )      // builds that record function entries (the "native" variant)
    {
        fprintf( stderr, "native:   entered, most recent first:" );
        for ( unsigned int i = 0; i < 512 && ILB_native_trace_entry( i ); i++ )
            fprintf( stderr, " %08x", (unsigned int)ILB_native_trace_entry( i ) + shift );
        fprintf( stderr, "\n" );
    }
    fflush( stderr );
    crash_box();
    return EXCEPTION_EXECUTE_HANDLER;
}

// Hang report: a watcher thread looks at the main thread once a second. A game that is running normally
// spends part of every frame outside the program (in the graphics driver, in SDL, waiting for the next
// frame); one that is caught in a loop of its own stays inside. After 15 seconds of that, where it is goes to
// the log, in the same form as a crash report, and again each time it has left the program and got stuck anew.
static HANDLE main_thread;
static const unsigned int *main_stack_top;

static void report_stuck( unsigned int base, unsigned int shift )
{
    CONTEXT c;
    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    if ( SuspendThread( main_thread ) == (DWORD)-1 ) return;
    if ( GetThreadContext( main_thread, &c ) )
    {
        int shown = 0;
        fprintf( stderr, "native: STUCK for 15 seconds in the program's own code, at %08lx; eax %08lx ebx %08lx ecx %08lx edx %08lx esi %08lx edi %08lx ebp %08lx esp %08lx\n",
                 c.Eip + shift, c.Eax, c.Ebx, c.Ecx, c.Edx, c.Esi, c.Edi, c.Ebp, c.Esp );
        fprintf( stderr, "native:   stack:" );
        for ( const unsigned int *stack = (const unsigned int *)c.Esp; stack + 1 <= main_stack_top && shown < 48; stack++ )
            if ( in_program_code( *stack, base ) ) { fprintf( stderr, " %08x", *stack + shift ); shown++; }
        fprintf( stderr, "\n" );
        if ( ILB_native_trace_entry( 0 ) )
        {
            fprintf( stderr, "native:   entered, most recent first:" );
            for ( unsigned int i = 0; i < 64 && ILB_native_trace_entry( i ); i++ )
                fprintf( stderr, " %08x", (unsigned int)ILB_native_trace_entry( i ) + shift );
            fprintf( stderr, "\n" );
        }
    }
    ResumeThread( main_thread );
}

static DWORD WINAPI watch_main_thread( void *unused )
{
    unsigned int base = (unsigned int)GetModuleHandleA( 0 ), shift = LINKED_BASE - base, inside = 0;
    (void)unused;
    for ( ;; )
    {
        CONTEXT c;
        Sleep( 1000 );
        c.ContextFlags = CONTEXT_CONTROL;
        if ( SuspendThread( main_thread ) == (DWORD)-1 ) return 0;
        int ok = GetThreadContext( main_thread, &c );
        ResumeThread( main_thread );
        inside = ok && in_program_code( c.Eip, base ) ? inside + 1 : 0;
        if ( inside == 15 )
            report_stuck( base, shift );
    }
}

static void on_abort( int signal_number )
{
    (void)signal_number;
    fprintf( stderr, "native: the program called abort()\n" );
    fflush( stderr );
    crash_box();
    _exit( 3 );
}

static void on_bad_argument( const wchar_t *expression, const wchar_t *function, const wchar_t *file, unsigned int line, uintptr_t reserved )
{
    (void)expression; (void)function; (void)file; (void)line; (void)reserved;
    RaiseException( 0xE0000001, 0, 0, 0 );      // a C library function was given an argument it rejects: report as a crash, with the stack
}

__attribute__(( constructor )) static void shim_start( void )
{
    stderr = __acrt_iob_func( 2 );
    if ( getenv( "ILB_LOG_CONSOLE" ) == 0 )
    {
        // next to the program; where that folder cannot be written to (a game installed under Program Files), the temporary folder
        char path[ MAX_PATH ];
        DWORD length = GetFullPathNameA( "ILB_windows.log", sizeof path, path, 0 );
        FILE *log = length > 0 && length < sizeof path ? freopen( path, "w", stderr ) : 0;
        if ( log == 0 )
        {
            length = GetTempPathA( sizeof path, path );
            if ( length > 0 && length < sizeof path - 20 )
            {
                strcat( path, "ILB_windows.log" );
                log = freopen( path, "w", __acrt_iob_func( 2 ) );
            }
        }
        if ( log == 0 ) { strcpy( path, "NUL" ); log = fopen( path, "w" ); }
        if ( log ) { stderr = log; strcpy( log_path, path ); }
    }
    setvbuf( stderr, 0, _IONBF, 0 );        // (line buffering does not exist here: it would keep the last lines back when the program dies)
    SetUnhandledExceptionFilter( on_crash );
    signal( SIGABRT, on_abort );
    _set_abort_behavior( 0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT );
    _set_invalid_parameter_handler( on_bad_argument );
    main_stack_top = (const unsigned int *)( (NT_TIB *)NtCurrentTeb() )->StackBase;
    if ( DuplicateHandle( GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &main_thread, 0, FALSE, DUPLICATE_SAME_ACCESS ) )
        CreateThread( 0, 0, watch_main_thread, 0, 0, 0 );   // (its handle stays open: the platform layer has a CloseHandle of its own, for files)
    if ( getenv( "ILB_TEST_CRASH" ) )       // to see the crash report work
        *(volatile int *)4 = 0;
    if ( getenv( "ILB_TEST_HANG" ) )        // to see the hang report work
        for ( volatile int spin = 1; spin; ) ;
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

// The crash report of the Linux builds hangs on Unix signals; there is none on Windows (see on_crash above).
int sigaction( int signal, const void *action, void *old_action )
{
    (void)signal; (void)action; (void)old_action;
    return 0;
}
