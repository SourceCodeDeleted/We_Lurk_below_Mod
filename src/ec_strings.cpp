//-----------------------------------------------------------------------------
// ec_strings.cpp
// Reconstructed from ILB.exe (0x40A1A0 - 0x40A860). Function names, parameter types and assert
// text are original. safestring_concat_int, safestring_concat_char and safestring_compare exist
// only as inlined copies in the binary; their names are original (compare) or chosen here (the
// other two), and their bodies are taken from those copies.

#include "main.h"
#include "g_strings.h"
#include <SDL.h>

safestring temp_string;

static wchar_t format_buffer[ max_string_size ];

//-----------------------------------------------------------------------------

void safestring_create_from_id( safestring * dest, const dword string_token_id )
{
    assert_return( dest );

    if ( string_token_id == invalid_id )
    {
        dest->length = 0;
        dest->text[ 0 ] = 0;
        return;
    }

    assert_return( string_token_id < (dword)num_string_definitions );

    const safestring * src = &string_definitions[ string_token_id ].string;
    dest->length = src->length;
    for ( int i = 0; i < dest->length; i++ ) dest->text[ i ] = src->text[ i ];
    dest->text[ dest->length ] = 0;
}

//-----------------------------------------------------------------------------

void safestring_create( safestring * dest, const wchar_t * src )
{
    assert_return( dest );

    if ( src == NULL )
    {
        dest->length = 0;
        dest->text[ 0 ] = 0;
        return;
    }

    int length = (int)wcslen( src );
    if ( length >= max_string_size ) length = max_string_size - 1;

    dest->length = length;
    for ( int i = 0; i < length; i++ ) dest->text[ i ] = src[ i ];
    dest->text[ length ] = 0;
}

//-----------------------------------------------------------------------------

void safestring_create_from_char( safestring * dest, const char * src )
{
    assert_return( dest );

    if ( src == NULL )
    {
        dest->length = 0;
        dest->text[ 0 ] = 0;
        return;
    }

    int length = (int)strlen( src );
    if ( length >= max_string_size ) length = max_string_size - 1;

    dest->length = length;
    for ( int i = 0; i < length; i++ ) dest->text[ i ] = src[ i ];
    dest->text[ length ] = 0;
}

//-----------------------------------------------------------------------------

void safestring_copy( safestring * dest, const safestring * src )
{
    assert_return( dest );
    assert_return( src );
    assert_return( src->length < max_string_size );

    dest->length = src->length;
    wcscpy( dest->text, src->text );
}

//-----------------------------------------------------------------------------

void safestring_concat( safestring * dest, const safestring * src )
{
    assert_return( dest );
    assert_return( src );

    int length = dest->length + src->length;
    if ( length >= max_string_size ) length = max_string_size - 1;

    for ( int i = dest->length; i < length; i++ ) dest->text[ i ] = src->text[ i - dest->length ];

    dest->length = length;
    dest->text[ length ] = 0;
}

//-----------------------------------------------------------------------------

void safestring_concat_str( safestring * dest, const wchar_t * src )
{
    assert_return( dest );
    assert_return( src );

    int length = dest->length + (int)wcslen( src );
    if ( length >= max_string_size ) length = max_string_size - 1;

    for ( int i = dest->length; i < length; i++ ) dest->text[ i ] = src[ i - dest->length ];

    dest->length = length;
    dest->text[ length ] = 0;
}

//-----------------------------------------------------------------------------

void safestring_concat_charstr( safestring * dest, const char * src )
{
    assert_return( dest );
    assert_return( src );

    int length = dest->length + (int)strlen( src );
    if ( length >= max_string_size ) length = max_string_size - 1;

    for ( int i = dest->length; i < length; i++ ) dest->text[ i ] = src[ i - dest->length ];

    dest->length = length;
    dest->text[ length ] = 0;
}

//-----------------------------------------------------------------------------

void safestring_concat_from_id( safestring * dest, const dword string_token_id )
{
    assert_return( dest );
    assert_return( string_token_id < (dword)num_string_definitions );

    const safestring * src = &string_definitions[ string_token_id ].string;

    int length = dest->length + src->length;
    if ( length >= max_string_size ) length = max_string_size - 1;

    for ( int i = dest->length; i < length; i++ ) dest->text[ i ] = src->text[ i - dest->length ];

    dest->length = length;
    dest->text[ length ] = 0;
}

//-----------------------------------------------------------------------------

void safestring_concat_int( safestring * dest, const int value )
{
    assert_return( dest );

    swprintf( format_buffer, max_string_size, L"%i", value );
    safestring_concat_str( dest, format_buffer );
}

//-----------------------------------------------------------------------------

void safestring_concat_char( safestring * dest, const wchar_t c )
{
    assert_return( dest );

    int length = dest->length + 1;
    if ( length < max_string_size )
    {
        dest->length = length;
        dest->text[ length - 1 ] = c;
        dest->text[ length ] = 0;
    }
}

//-----------------------------------------------------------------------------

void safestring_concat_float1( safestring * dest, const float value )
{
    assert_return( dest );

    swprintf( format_buffer, max_string_size, L"%0.1f", value );
    safestring_concat_str( dest, format_buffer );
}

//-----------------------------------------------------------------------------

void safestring_concat_float2( safestring * dest, const float value )
{
    assert_return( dest );

    swprintf( format_buffer, max_string_size, L"%0.2f", value );
    safestring_concat_str( dest, format_buffer );
}

//-----------------------------------------------------------------------------

void safestring_concat_key( safestring * dest, const dword key, bool add_quote )
{
    assert_return( dest );

    safestring_concat_charstr( dest, SDL_GetKeyName( key ) );
    if ( add_quote ) safestring_concat_char( dest, L'\'' );
}

//-----------------------------------------------------------------------------

void safestring_truncate( safestring * dest, const int new_length )
{
    assert_return( dest );
    assert_return( new_length >= 0 );
    assert_return( new_length < max_string_size );
    assert_return( new_length <= dest->length );

    dest->length = new_length;
    dest->text[ new_length ] = 0;
}

//-----------------------------------------------------------------------------
// Only the ignore_case path is visible in the binary (ASCII a-z folded to upper case).

bool safestring_compare( safestring * a, safestring * b, bool ignore_case )
{
    assert_return_false( a );
    assert_return_false( b );

    if ( a->length != b->length ) return false;

    for ( int i = 0; i < a->length; i++ )
    {
        wchar_t ca = a->text[ i ];
        wchar_t cb = b->text[ i ];
        if ( ignore_case )
        {
            if ( ca >= L'a' && ca <= L'z' ) ca -= 32;
            if ( cb >= L'a' && cb <= L'z' ) cb -= 32;
        }
        if ( ca != cb ) return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

void safestring_to_char( safestring * src, char * dest )
{
    assert_return( src );
    assert_return( dest );

    for ( int i = 0; i < src->length; i++ ) dest[ i ] = (char)src->text[ i ];
    dest[ src->length ] = 0;
}
