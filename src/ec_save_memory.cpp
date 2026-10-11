//-----------------------------------------------------------------------------
// ec_save_memory.cpp
// Reconstructed from ILB.exe: save_memory_allocate 0x4080B0. save_memory_get exists only as inlined
// copies in the save / load code (its cases are pieced together from 14 of them); save_memory_free
// is the block of eight frees inlined at the end of main, and its name is chosen here.
//
// The first 248 lines of the original file are unaccounted for: nothing in the binary carries an
// assert from them.

#include "main.h"
#include "ec_save_memory.h"

static byte * game_header_memory = NULL;        // 0x3734504
static byte * game_file_memory = NULL;          // 0x3735508
static byte * account_header_memory = NULL;     // 0x373550C
static byte * cloud_header_memory = NULL;       // 0x3735510
static byte * cloud_file_memory = NULL;         // 0x3735514
static byte * town_header_memory = NULL;        // 0x3735518
static byte * account_file_memory = NULL;       // 0x373551C
static byte * town_file_memory = NULL;          // 0x3735520

//-----------------------------------------------------------------------------

void save_memory_allocate()
{
    assert_return( town_file_memory == NULL );
    town_file_memory = (byte *)drb_malloc( max_town_file_memory_size, __FILE__, __LINE__, NULL );
    assert_return( town_header_memory == NULL );
    town_header_memory = (byte *)drb_malloc( max_header_memory_size, __FILE__, __LINE__, NULL );
    assert_return( game_file_memory == NULL );
    game_file_memory = (byte *)drb_malloc( max_game_file_memory_size, __FILE__, __LINE__, NULL );
    assert_return( game_header_memory == NULL );
    game_header_memory = (byte *)drb_malloc( max_header_memory_size, __FILE__, __LINE__, NULL );
    assert_return( cloud_file_memory == NULL );
    cloud_file_memory = (byte *)drb_malloc( max_cloud_file_memory_size, __FILE__, __LINE__, NULL );
    assert_return( cloud_header_memory == NULL );
    cloud_header_memory = (byte *)drb_malloc( max_header_memory_size, __FILE__, __LINE__, NULL );
    assert_return( account_file_memory == NULL );
    account_file_memory = (byte *)drb_malloc( max_account_file_memory_size, __FILE__, __LINE__, NULL );
    assert_return( account_header_memory == NULL );
    account_header_memory = (byte *)drb_malloc( max_header_memory_size, __FILE__, __LINE__, NULL );
}

//-----------------------------------------------------------------------------

void save_memory_free()
{
    if ( game_file_memory ) { drb_free( game_file_memory ); game_file_memory = NULL; }
    if ( game_header_memory ) { drb_free( game_header_memory ); game_header_memory = NULL; }
    if ( town_file_memory ) { drb_free( town_file_memory ); town_file_memory = NULL; }
    if ( town_header_memory ) { drb_free( town_header_memory ); town_header_memory = NULL; }
    if ( cloud_file_memory ) { drb_free( cloud_file_memory ); cloud_file_memory = NULL; }
    if ( cloud_header_memory ) { drb_free( cloud_header_memory ); cloud_header_memory = NULL; }
    if ( account_file_memory ) { drb_free( account_file_memory ); account_file_memory = NULL; }
    if ( account_header_memory ) { drb_free( account_header_memory ); account_header_memory = NULL; }
}

//-----------------------------------------------------------------------------
// The file buffers check the requested size, the header buffers do not. The two cloud cases are
// never reached in this build, so their checks are assumed to follow the same pattern.

byte * save_memory_get( dword type, dword needed_size )
{
    switch ( type )
    {
    case save_memory_game_file:
        assert_return_null( needed_size < max_game_file_memory_size );
        assert_return_null( game_file_memory );
        return game_file_memory;

    case save_memory_town_file:
        assert_return_null( needed_size < max_town_file_memory_size );
        assert_return_null( town_file_memory );
        return town_file_memory;

    case save_memory_game_header:
        assert_return_null( game_header_memory );
        return game_header_memory;

    case save_memory_town_header:
        assert_return_null( town_header_memory );
        return town_header_memory;

    case save_memory_cloud_file:
        assert_return_null( needed_size < max_cloud_file_memory_size );
        assert_return_null( cloud_file_memory );
        return cloud_file_memory;

    case save_memory_cloud_header:
        assert_return_null( cloud_header_memory );
        return cloud_header_memory;

    case save_memory_account_file:
        assert_return_null( needed_size < max_account_file_memory_size );
        assert_return_null( account_file_memory );
        return account_file_memory;

    case save_memory_account_header:
        assert_return_null( account_header_memory );
        return account_header_memory;
    }

    return NULL;
}
