//-----------------------------------------------------------------------------
// ec_fileio.cpp
// Reconstructed from ILB.exe: LZW helpers 0x404F70 - 0x4050B0, pack_file_get_index 0x4053B0,
// pack_file_load 0x405520, pack_file_close 0x405680, packfile_load 0x4056E0, packfile_close 0x405BC0,
// appdata path builder 0x405C40, appdata_file_* 0x405D70 - 0x406240. appdata_file_load exists only as
// an inlined copy (inside the config loader at 0x4022F0), local_save only inside appdata_file_save.
//
// Original names: all pack_file / packfile / appdata_file functions (appdata_file_text_save from its
// copy inlined in the config save at 0x402710), file_load, local_save,
// lzw_table_add, and from assert text pack_file, file_hash_list, uncompressed_size, lzw_table_size,
// lzw_max_table_size, read_ok, out_size, memory_location, memory_size, size_to_read, bytesread,
// filesize, out_data, old_name, new_name, string_list, num_lines, len. Names chosen here: appdata_set_filename, appdata_filename,
// the other LZW functions and the remaining struct fields.

#include "main.h"
#include "ec_fileio.h"
#include "ec_config.h"
#include <SDL.h>
#include <windows.h>

//-----------------------------------------------------------------------------
// pack file layout
//
//   dword num_files
//   num_files x pack_file_header, each followed by name_length name bytes (stored +66)
//   file data, concatenated in directory order

#define pack_file_name_offset       66
#define max_pack_file_name          256

#define file_hash_list_size         1024
#define file_hash_list_mask         ( file_hash_list_size - 1 )

struct pack_file_header     // 16 bytes
{
    dword       compressed_size;
    dword       uncompressed_size;
    dword       unknown;            // stored in the list but never read
    byte        name_length;
    byte        padding[ 3 ];
};

struct file_hash_entry      // 276 bytes
{
    dword       uncompressed_size;
    dword       unknown;
    dword       compressed_size;
    byte *      data;               // inside pack_file
    byte *      memory;             // loaded copy, NULL until pack_file_load is called
    char        name[ max_pack_file_name ];
};

char game_path[ 1024 ];

static byte * pack_file = NULL;                                 // 0x16B8DA4
static char pack_file_path[ 1024 ];                             // 0x16B99B0
static file_hash_entry file_hash_list[ file_hash_list_size ];   // 0x16B9DB0

//-----------------------------------------------------------------------------
// LZW: 16-bit codes, 0-255 are literals, 256 resets the table, new entries start at 257

#define lzw_max_table_size      4096
#define lzw_reset_code          256

struct lzw_table_entry
{
    word        c;          // last byte of the sequence
    word        prefix;     // code of the sequence without its last byte
};

static int lzw_table_size = 0;                              // 0x16B95AC
static lzw_table_entry lzw_table[ lzw_max_table_size ];     // 0x16FEDB0

//-----------------------------------------------------------------------------

static void lzw_table_reset()
{
    for ( int i = 0; i <= lzw_reset_code; i++ )
    {
        lzw_table[ i ].c = (word)i;
        lzw_table[ i ].prefix = 0xffff;
    }
    lzw_table_size = lzw_reset_code + 1;
}

//-----------------------------------------------------------------------------

static int lzw_output_code( byte * dest, int position, word code )
{
    if ( code <= 0xff )
    {
        dest[ position ] = (byte)code;
        return position + 1;
    }

    position = lzw_output_code( dest, position, lzw_table[ code ].prefix );
    return lzw_output_code( dest, position, lzw_table[ code ].c );
}

//-----------------------------------------------------------------------------

static word lzw_first_char( word code )
{
    while ( code > 0xff ) code = lzw_table[ code ].prefix;
    return code;
}

//-----------------------------------------------------------------------------

static void lzw_table_add( word prefix, word c )
{
    assert_return( lzw_table_size < lzw_max_table_size );

    lzw_table[ lzw_table_size ].c = c;
    lzw_table[ lzw_table_size ].prefix = prefix;
    lzw_table_size++;
}

//-----------------------------------------------------------------------------

static void lzw_decompress( const word * source, dword num_codes, byte * dest, dword * out_size )
{
    lzw_table_reset();

    word previous = source[ 0 ];
    int position = lzw_output_code( dest, 0, previous );

    dword i = 1;
    while ( i < num_codes )
    {
        word code = source[ i++ ];

        if ( code == lzw_reset_code )
        {
            lzw_table_reset();
            if ( i >= num_codes ) break;
            code = source[ i++ ];
            position = lzw_output_code( dest, position, code );
        }
        else if ( code >= lzw_table_size )
        {
            // the code about to be defined: previous sequence plus its own first byte
            word c = lzw_first_char( previous );
            lzw_table_add( previous, c );
            position = lzw_output_code( dest, position, previous );
            position = lzw_output_code( dest, position, c );
        }
        else
        {
            position = lzw_output_code( dest, position, code );
            lzw_table_add( previous, lzw_first_char( code ) );
        }

        previous = code;
    }

    *out_size = position;
}

//-----------------------------------------------------------------------------

static byte * file_load( const char * filename )
{
    SDL_RWops * file = SDL_RWFromFile( filename, "rb" );
    assert_return_null( file != NULL );

    dword size = (dword)SDL_RWsize( file );
    byte * memory = (byte *)drb_malloc( size, __FILE__, __LINE__, NULL );

    size_t read_ok = SDL_RWread( file, memory, size, 1 );
    if ( read_ok == 0 )
    {
        assert_exp( read_ok != 0 );
        SDL_RWclose( file );
        drb_free( memory );
        return NULL;
    }

    SDL_RWclose( file );
    return memory;
}

//-----------------------------------------------------------------------------
// names hash to the 16-bit sum of their characters

static dword pack_file_name_hash( const char * name )
{
    short sum = 0;
    int length = (int)strlen( name );
    for ( int i = 0; i < length; i++ ) sum += name[ i ];
    return sum & file_hash_list_mask;
}

//-----------------------------------------------------------------------------
// The hash only picks the starting slot. The search does not stop at empty slots and compares
// without case, so a name in different capitalisation is still found, just after more probes.

dword pack_file_get_index( const char * name )
{
    if ( pack_file == NULL )
    {
        assert_exp( pack_file );
        return invalid_id;
    }

    dword index = pack_file_name_hash( name );
    int count = 0;
    while ( _stricmp( name, file_hash_list[ index ].name ) != 0 )
    {
        count++;
        index = ( index + 1 ) & file_hash_list_mask;
        if ( count >= file_hash_list_size ) return invalid_id;
    }

    return index;
}

//-----------------------------------------------------------------------------

byte * pack_file_load( const char * name, dword * size )
{
    assert_return_null( pack_file );
    assert_return_null( name );

    dword index = pack_file_get_index( name );
    if ( index == invalid_id )
    {
        safestring_create( &temp_string, L"Error loading file: " );
        safestring_concat_charstr( &temp_string, name );
        debug_log_add_line( temp_string.text );
    }
    assert_return_null( index != invalid_id );

    if ( size ) *size = file_hash_list[ index ].uncompressed_size;

    if ( file_hash_list[ index ].memory == NULL )
    {
        byte * memory = (byte *)drb_malloc( file_hash_list[ index ].uncompressed_size, __FILE__, __LINE__, NULL );
        file_hash_list[ index ].memory = memory;

        if ( file_hash_list[ index ].compressed_size == file_hash_list[ index ].uncompressed_size )
        {
            memcpy( memory, file_hash_list[ index ].data, file_hash_list[ index ].compressed_size );
        }
        else
        {
            dword out_size;
            lzw_decompress( (const word *)file_hash_list[ index ].data, file_hash_list[ index ].compressed_size / 2, memory, &out_size );
            assert_return_null( out_size == file_hash_list[index].uncompressed_size );
        }
    }

    return file_hash_list[ index ].memory;
}

//-----------------------------------------------------------------------------

void pack_file_close( byte * memory )
{
    assert_return( pack_file );

    for ( int i = 0; i < file_hash_list_size; i++ )
    {
        if ( file_hash_list[ i ].memory == memory )
        {
            drb_free( memory );
            file_hash_list[ i ].memory = NULL;
        }
    }
}

//-----------------------------------------------------------------------------

bool packfile_load()
{
    assert_return_false( pack_file == NULL );

    strcpy( pack_file_path, game_path );
    strcat( pack_file_path, "\\" );
    strcat( pack_file_path, "ILB.gbp" );

    pack_file = file_load( pack_file_path );
    assert_return_false( pack_file );

    memset( file_hash_list, 0, sizeof( file_hash_list ) );

    dword num_files = *(dword *)pack_file;
    byte * first_header = pack_file + sizeof( dword );

    // file data starts after the last directory entry
    byte * data = first_header;
    for ( dword i = 0; i < num_files; i++ ) data += sizeof( pack_file_header ) + ( (pack_file_header *)data )->name_length;

    pack_file_header * header = (pack_file_header *)first_header;
    for ( dword i = 0; i < num_files; i++ )
    {
        char name[ 1024 ];
        byte * stored_name = (byte *)( header + 1 );
        for ( int c = 0; c < header->name_length; c++ ) name[ c ] = (char)( stored_name[ c ] - pack_file_name_offset );
        name[ header->name_length ] = 0;

        dword index = pack_file_name_hash( name );
        int count = 0;
        while ( file_hash_list[ index ].name[ 0 ] != 0 )
        {
            count++;
            index = ( index + 1 ) & file_hash_list_mask;
            if ( count >= file_hash_list_size ) index = invalid_id;
            assert_return_false( index != invalid_id );
        }

        strcpy( file_hash_list[ index ].name, name );
        file_hash_list[ index ].data = data;
        file_hash_list[ index ].compressed_size = header->compressed_size;
        file_hash_list[ index ].uncompressed_size = header->uncompressed_size;
        file_hash_list[ index ].unknown = header->unknown;
        file_hash_list[ index ].memory = NULL;

        data += header->compressed_size;
        header = (pack_file_header *)( stored_name + header->name_length );
    }

    return true;
}

//-----------------------------------------------------------------------------

void packfile_close()
{
    for ( int i = 0; i < file_hash_list_size; i++ )
    {
        if ( file_hash_list[ i ].memory != NULL )
        {
            drb_free( file_hash_list[ i ].memory );
            file_hash_list[ i ].memory = NULL;
        }
    }

    assert_return( pack_file );

    drb_free( pack_file );
    pack_file = NULL;
    memset( file_hash_list, 0, sizeof( file_hash_list ) );
}

//-----------------------------------------------------------------------------
// Save files live in %LOCALAPPDATA%\Graybeard Games\It Lurks Below\. This builds the full path of
// one of them in appdata_filename, creating the two folders on the way.

static safestring appdata_filename;     // 0x1702DB0

static void appdata_set_filename( const char * name )
{
    safestring_create( &appdata_filename, _wgetenv( L"LOCALAPPDATA" ) );
    safestring_concat_str( &appdata_filename, L"\\Graybeard Games\\" );
    CreateDirectoryW( appdata_filename.text, NULL );
    safestring_concat_str( &appdata_filename, L"It Lurks Below\\" );
    CreateDirectoryW( appdata_filename.text, NULL );
    safestring_concat_charstr( &appdata_filename, name );
}

//-----------------------------------------------------------------------------
// Returns memory the caller frees with drb_free. The name check is assumed from the sibling
// functions: the only copy in the binary is inlined with a constant name, which removes it.

byte * appdata_file_load( const char * name, dword * size )
{
    assert_return_null( name );

    appdata_set_filename( name );
    HANDLE file = CreateFileW( appdata_filename.text, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    assert_return_null( file != INVALID_HANDLE_VALUE );

    DWORD filesize = GetFileSize( file, NULL );
    if ( size ) *size = filesize;

    byte * memory = (byte *)drb_malloc( filesize, __FILE__, __LINE__, NULL );

    DWORD bytesread;
    ReadFile( file, memory, filesize, &bytesread, NULL );
    if ( bytesread != filesize )
    {
        assert_exp( bytesread == filesize );
        CloseHandle( file );
        drb_free( memory );
        return NULL;
    }

    CloseHandle( file );
    return memory;
}

//-----------------------------------------------------------------------------
// Reads the whole file into the caller's buffer; fails quietly if the buffer is too small.

bool appdata_file_load_into_mem( const char * name, byte * memory_location, dword memory_size, dword * size )
{
    assert_return_false( name );
    assert_return_false( memory_location );
    assert_return_false( memory_size > 0 );

    appdata_set_filename( name );
    HANDLE file = CreateFileW( appdata_filename.text, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    assert_return_false( file != INVALID_HANDLE_VALUE );

    DWORD filesize = GetFileSize( file, NULL );
    if ( memory_size < filesize )
    {
        CloseHandle( file );
        return false;
    }
    if ( size ) *size = filesize;

    DWORD bytesread;
    ReadFile( file, memory_location, filesize, &bytesread, NULL );
    if ( bytesread != filesize )
    {
        assert_exp( bytesread == filesize );
        CloseHandle( file );
        return false;
    }

    CloseHandle( file );
    return true;
}

//-----------------------------------------------------------------------------

bool appdata_file_load_partial_into_mem( const char * name, byte * memory_location, dword memory_size, dword size_to_read )
{
    assert_return_false( name );
    assert_return_false( memory_location );
    assert_return_false( memory_size > 0 );
    assert_return_false( size_to_read <= memory_size );

    appdata_set_filename( name );
    HANDLE file = CreateFileW( appdata_filename.text, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    assert_return_false( file != INVALID_HANDLE_VALUE );

    DWORD bytesread;
    ReadFile( file, memory_location, size_to_read, &bytesread, NULL );
    if ( bytesread != size_to_read )
    {
        assert_exp( bytesread == size_to_read );
        CloseHandle( file );
        return false;
    }

    CloseHandle( file );
    return true;
}

//-----------------------------------------------------------------------------

bool appdata_file_exist( const char * name )
{
    assert_return_false( name );

    appdata_set_filename( name );
    HANDLE file = CreateFileW( appdata_filename.text, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( file == INVALID_HANDLE_VALUE ) return false;

    CloseHandle( file );
    return true;
}

//-----------------------------------------------------------------------------
// The handle is compared with NULL as in the binary, although a failed CreateFileW returns
// INVALID_HANDLE_VALUE; a failed create therefore falls through to WriteFile and is reported as
// a short write.

static bool local_save( const wchar_t * filename, void * out_data, dword out_size )
{
    static wchar_t message[ 1024 ];

    HANDLE file = CreateFileW( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    assert_return_false( file );

    DWORD byteswritten = 0;
    BOOL write_ok = WriteFile( file, out_data, out_size, &byteswritten, NULL );
    CloseHandle( file );
    if ( write_ok && byteswritten == out_size ) return true;

    swprintf( message, 1024, L"Save Error! Bytes written: %i, Requested: %i", byteswritten, out_size );
    debug_log_add_line( message );
    return false;
}

//-----------------------------------------------------------------------------

bool appdata_file_save( char * name, void * out_data, dword out_size )
{
    assert_return_false( name );
    assert_return_false( out_data );
    assert_return_false( out_size > 0 );

    appdata_set_filename( name );
    return local_save( appdata_filename.text, out_data, out_size );
}

//-----------------------------------------------------------------------------
// Writes num_lines C strings, one per config_strings slot, without adding line ends.

bool appdata_file_text_save( char * name, config_strings * string_list, int num_lines )
{
    assert_return_false( string_list );
    assert_return_false( num_lines > 0 );

    appdata_set_filename( name );
    HANDLE file = CreateFileW( appdata_filename.text, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    assert_return_false( file );

    for ( int i = 0; i < num_lines; i++ )
    {
        DWORD len = (DWORD)strlen( string_list[ i ].string );
        DWORD byteswritten;
        WriteFile( file, string_list[ i ].string, len, &byteswritten, NULL );
        assert_exp( byteswritten == len );
    }

    CloseHandle( file );
    return true;
}

//-----------------------------------------------------------------------------

bool appdata_file_rename( char * old_name, char * new_name )
{
    assert_return_false( old_name );
    assert_return_false( new_name );

    appdata_set_filename( old_name );
    safestring old_filename = appdata_filename;
    appdata_set_filename( new_name );

    return MoveFileW( old_filename.text, appdata_filename.text ) != 0;
}
