//-----------------------------------------------------------------------------
// ec_assert.cpp
// Reconstructed from ILB.exe: assert_exists 0x401000, assert_failed 0x401220.
// Table and variable names are chosen here; function names are original.

#include "main.h"

//-----------------------------------------------------------------------------
// each assert that has already been reported, keyed by line and file

#define assert_hash_tbl_size        4096
#define assert_hash_tbl_mask        ( assert_hash_tbl_size - 1 )
#define max_assert_stack_lines      32
#define num_assert_stack_lines_shown 4

struct assert_hash_entry
{
    dword       line;       // invalid_id = empty
    safestring  file;
};

static safestring assert_stack[ max_assert_stack_lines ];      // 0xE9C510; nothing found that fills it yet
static assert_hash_entry assert_hash_tbl[ assert_hash_tbl_size ];

//-----------------------------------------------------------------------------
// Table set-up, found inlined in gdi_init (0x581B40). The function name is chosen here.

void assert_system_init()
{
    for ( int i = 0; i < assert_hash_tbl_size; i++ )
    {
        assert_hash_tbl[ i ].line = invalid_id;
        safestring_create( &assert_hash_tbl[ i ].file, NULL );
    }
    for ( int i = 0; i < max_assert_stack_lines; i++ ) safestring_create( &assert_stack[ i ], NULL );
}

//-----------------------------------------------------------------------------
// Returns true if this assert was reported before (or the table is full), otherwise records it.

bool assert_exists( int line, const char * file )
{
    assert_return_false( file );

    int index = line & assert_hash_tbl_mask;
    safestring_create_from_char( &temp_string, file );

    for ( int count = 0; count < assert_hash_tbl_size; count++ )
    {
        if ( assert_hash_tbl[ index ].line == invalid_id )
        {
            assert_hash_tbl[ index ].line = line;
            safestring_copy( &assert_hash_tbl[ index ].file, &temp_string );
            return false;
        }
        if ( assert_hash_tbl[ index ].line == (dword)line )
        {
            if ( safestring_compare( &assert_hash_tbl[ index ].file, &temp_string, true ) ) return true;
        }
        index = ( index + 1 ) & assert_hash_tbl_mask;
    }

    return true;
}

//-----------------------------------------------------------------------------

bool assert_failed( const char * expression, const char * file, const char * function, int line, bool show_stack )
{
    if ( !expression || !file || !function || line < 0 ) return false;
    if ( assert_exists( line, file ) ) return false;

    safestring_create( &temp_string, L"ASSERT( " );
    safestring_concat_charstr( &temp_string, expression );
    safestring_concat_str( &temp_string, L" )" );
    debug_log_add_line( temp_string.text );

    safestring_create( &temp_string, L"Function: " );
    safestring_concat_charstr( &temp_string, function );
    debug_log_add_line( temp_string.text );

    safestring_create( &temp_string, L"File: " );
    safestring_concat_charstr( &temp_string, file );
    debug_log_add_line( temp_string.text );

    safestring_create( &temp_string, L"Line: " );
    safestring_concat_int( &temp_string, line );
    debug_log_add_line( temp_string.text );

    if ( show_stack )
    {
        debug_log_add_line( L"-- Stack --" );
        for ( int i = 1; i <= num_assert_stack_lines_shown; i++ )
            debug_log_add_line( assert_stack[ max_assert_stack_lines - i ].text );
    }

    return false;
}
