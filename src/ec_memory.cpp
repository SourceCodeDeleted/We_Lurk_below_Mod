//-----------------------------------------------------------------------------
// ec_memory.cpp
// Reconstructed from ILB.exe: memory_system_close 0x407D50, drb_malloc 0x407E50, drb_free 0x407F90.
// Function names and assert text (including allocations_hash_tbl, mem_ptr, num_memory_allocations,
// memory_hash_tbl_size) are original; the other field and variable names are chosen here.
// memory_system_init is the table set-up found inlined in gdi_init (0x581B40); its name is chosen
// here to pair with memory_system_close.

#include "main.h"
#include "ec_config.h"
#include <windows.h>

#define memory_hash_tbl_size        0x2000
#define memory_hash_tbl_mask        ( memory_hash_tbl_size - 1 )

struct memory_allocation        // 4120 bytes
{
    bool        unused;
    safestring  file;
    safestring  info;
    int         line;
    dword       size;
    void *      mem_ptr;
};

static int next_allocation_index = 0;                                   // 0x17044F4
static memory_allocation allocations_hash_tbl[ memory_hash_tbl_size ];  // 0x17044F8
static int num_memory_allocations = 0;                                  // 0x37344F8
static dword memory_allocated = 0;                                      // 0x37344FC

static wchar_t message_text[ 1024 ];    // shared message buffer in the binary ("Text")

//-----------------------------------------------------------------------------

void memory_system_init()
{
    memset( allocations_hash_tbl, 0, sizeof( allocations_hash_tbl ) );
    for ( int i = 0; i < memory_hash_tbl_size; i++ )
    {
        allocations_hash_tbl[ i ].unused = true;
        allocations_hash_tbl[ i ].mem_ptr = NULL;
    }

    num_memory_allocations = 0;
    memory_allocated = 0;
    next_allocation_index = 0;
}

//-----------------------------------------------------------------------------

void memory_system_close()
{
    if ( num_memory_allocations == 0 ) return;

    for ( int i = 0; i < memory_hash_tbl_size; i++ )
    {
        if ( allocations_hash_tbl[ i ].unused ) continue;

        assert_return( allocations_hash_tbl[i].mem_ptr != NULL );

        if ( allocations_hash_tbl[ i ].info.length != 0 )
            swprintf( message_text, 1024, L"Unfreed Memory\n\nFile : %s\nLine : %i\nSize : %i\nInfo : %s",
                allocations_hash_tbl[ i ].file.text, allocations_hash_tbl[ i ].line, allocations_hash_tbl[ i ].size,
                allocations_hash_tbl[ i ].info.text );
        else
            swprintf( message_text, 1024, L"Unfreed Memory\n\nFile : %s\nLine : %i\nSize : %i",
                allocations_hash_tbl[ i ].file.text, allocations_hash_tbl[ i ].line, allocations_hash_tbl[ i ].size );

        MessageBoxW( NULL, message_text, L"MEMORY NOT FREED", MB_ICONEXCLAMATION );
    }
}

//-----------------------------------------------------------------------------

void * drb_malloc( dword size, const char * file, int line, const char * info )
{
    assert_return_null( num_memory_allocations < memory_hash_tbl_size );

    int index = next_allocation_index;
    while ( !allocations_hash_tbl[ index ].unused ) index = ( index + 1 ) & memory_hash_tbl_mask;

    assert_return_null( allocations_hash_tbl[index].mem_ptr == NULL );

    void * p = malloc( size );
    if ( p == NULL )
    {
        debug_log_format_line( L"Memory Allocation Failed! %i", size );
        if ( config.verbose_loading ) debug_log_save( false );
    }
    assert_return_null( p );

    allocations_hash_tbl[ index ].unused = false;
    safestring_create_from_char( &allocations_hash_tbl[ index ].file, file );
    safestring_create_from_char( &allocations_hash_tbl[ index ].info, info );
    memory_allocated += size;
    num_memory_allocations++;
    allocations_hash_tbl[ index ].line = line;
    allocations_hash_tbl[ index ].size = size;
    allocations_hash_tbl[ index ].mem_ptr = p;

    // the search restarts one past the previous start, not one past the slot just used
    next_allocation_index = ( next_allocation_index + 1 ) & memory_hash_tbl_mask;

    return p;
}

//-----------------------------------------------------------------------------

void drb_free( void * p )
{
    int index = 0;
    int count = 0;

    while ( allocations_hash_tbl[ index ].unused || allocations_hash_tbl[ index ].mem_ptr != p )
    {
        assert_return( count < memory_hash_tbl_size );
        count++;
        index = ( index + 1 ) & memory_hash_tbl_mask;
    }

    free( allocations_hash_tbl[ index ].mem_ptr );
    allocations_hash_tbl[ index ].unused = true;
    allocations_hash_tbl[ index ].mem_ptr = NULL;
    safestring_create( &allocations_hash_tbl[ index ].file, NULL );
    safestring_create( &allocations_hash_tbl[ index ].info, NULL );
    memory_allocated -= allocations_hash_tbl[ index ].size;
    num_memory_allocations--;
}
