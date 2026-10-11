//-----------------------------------------------------------------------------
// ec_fileio.h
//
// Pack file (ILB.gbp) access, and save files under %LOCALAPPDATA%\Graybeard Games\It Lurks Below.

#ifndef _EC_FILEIO_H_
#define _EC_FILEIO_H_

// directory the game runs from, with a trailing backslash (0x16B95B0, filled in by main);
// the name is chosen here
extern char game_path[ 1024 ];

dword pack_file_get_index( const char * name );
byte * pack_file_load( const char * name, dword * size );
void pack_file_close( byte * memory );
bool packfile_load();
void packfile_close();

byte * appdata_file_load( const char * name, dword * size );
bool appdata_file_load_into_mem( const char * name, byte * memory_location, dword memory_size, dword * size );
bool appdata_file_load_partial_into_mem( const char * name, byte * memory_location, dword memory_size, dword size_to_read );
bool appdata_file_exist( const char * name );
bool appdata_file_save( char * name, void * out_data, dword out_size );
struct config_strings;
bool appdata_file_text_save( char * name, config_strings * string_list, int num_lines );
bool appdata_file_rename( char * old_name, char * new_name );

#endif
