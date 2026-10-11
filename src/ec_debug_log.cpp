//-----------------------------------------------------------------------------
// ec_debug_log.cpp
// Reconstructed from ILB.exe: debug_log_add_line 0x404180, debug_log_format_line 0x404290,
// debug_log_save 0x4043D0; debug_log_add_char_str from its inlined copy inside 0x402FC0.
// Function names and the names in the assert text (num_debug_log_lines, max_debug_log_lines,
// debug_log_line_length, max_debug_log_line_length) are original.
//
// Not recovered yet: the function that allocates debug_log_data.

#include "main.h"
#include <windows.h>

#define max_debug_log_lines             4096
#define max_debug_log_line_length       1024

static char * debug_log_data = NULL;                               // 0x16B8DA0
static int num_debug_log_lines = 0;                                 // 0x16B4D9C
static dword debug_log_line_length[ max_debug_log_lines ];
static wchar_t debug_log_format_buffer[ max_debug_log_line_length ];

//-----------------------------------------------------------------------------

void debug_log_start()
{
    assert_return( debug_log_data == NULL );
    debug_log_data = (char *)drb_malloc( max_debug_log_lines * max_debug_log_line_length, __FILE__, __LINE__, NULL );
    assert_return( debug_log_data != NULL );

    num_debug_log_lines = 0;
}

//-----------------------------------------------------------------------------
// lines are stored as 8-bit text: wide characters are truncated

void debug_log_add_line( const wchar_t * line )
{
    if ( debug_log_data == NULL ) return;

    assert_return( num_debug_log_lines >= 0 );
    assert_return( num_debug_log_lines < max_debug_log_lines );

    int length = (int)wcslen( line );
    debug_log_line_length[ num_debug_log_lines ] = length + 2;
    assert_return( debug_log_line_length[ num_debug_log_lines ] < max_debug_log_line_length - 1 );

    char * dest = &debug_log_data[ num_debug_log_lines * max_debug_log_line_length ];
    for ( int i = 0; line[ i ] != 0; i++ ) dest[ i ] = (char)line[ i ];
    dest[ length ] = 0;
    strcat( dest, "\r\n" );

    num_debug_log_lines++;
}

//-----------------------------------------------------------------------------

void debug_log_format_line( const wchar_t * format, ... )
{
    if ( debug_log_data == NULL ) return;

    assert_return( num_debug_log_lines >= 0 );
    assert_return( num_debug_log_lines < max_debug_log_lines );

    va_list args;
    va_start( args, format );
    vswprintf( debug_log_format_buffer, max_debug_log_line_length, format, args );
    va_end( args );

    int length = (int)wcslen( debug_log_format_buffer );
    debug_log_line_length[ num_debug_log_lines ] = length + 2;
    assert_return( debug_log_line_length[ num_debug_log_lines ] < max_debug_log_line_length - 1 );

    char * dest = &debug_log_data[ num_debug_log_lines * max_debug_log_line_length ];
    for ( int i = 0; debug_log_format_buffer[ i ] != 0; i++ ) dest[ i ] = (char)debug_log_format_buffer[ i ];
    dest[ length ] = 0;
    strcat( dest, "\r\n" );

    num_debug_log_lines++;
}

//-----------------------------------------------------------------------------
// As in the binary, this does not set debug_log_line_length for the new line, so
// debug_log_save writes it with whatever length that slot held before.

void debug_log_add_char_str( const char * line )
{
    if ( debug_log_data == NULL ) return;

    assert_return( num_debug_log_lines >= 0 );
    assert_return( num_debug_log_lines < max_debug_log_lines );

    char * dest = &debug_log_data[ num_debug_log_lines * max_debug_log_line_length ];
    int length = (int)strlen( line );
    for ( int i = 0; i < length; i++ ) dest[ i ] = line[ i ];
    dest[ length ] = 0;
    strcat( dest, "\r\n" );

    num_debug_log_lines++;
}

//-----------------------------------------------------------------------------

void debug_log_save( bool free_log )
{
    if ( num_debug_log_lines > 0 )
    {
        HANDLE file = CreateFileW( L"debug_log.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        assert_return( file );

        for ( int i = 0; i < num_debug_log_lines; i++ )
        {
            DWORD byteswritten;
            WriteFile( file, &debug_log_data[ i * max_debug_log_line_length ], debug_log_line_length[i], &byteswritten, NULL );
            assert_exp( byteswritten == debug_log_line_length[i] );
        }

        CloseHandle( file );
    }

    if ( free_log )
    {
        drb_free( debug_log_data );
        debug_log_data = NULL;
        num_debug_log_lines = 0;
    }
}
