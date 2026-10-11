//-----------------------------------------------------------------------------
// ec_save_memory.h
//
// Fixed buffers that save files are built in and loaded into, allocated once at start-up.
//
// Buffer names and the max_*_memory_size names are original (assert text). The save_memory_*
// type names are chosen here, and their order is a guess: save_memory_get only exists inlined
// with a constant type, so the values cannot be read from the binary. The order used is the order
// of the cases by source line.

#ifndef _EC_SAVE_MEMORY_H_
#define _EC_SAVE_MEMORY_H_

#define max_game_file_memory_size       0x4000000
#define max_town_file_memory_size       0x800000
#define max_cloud_file_memory_size      0x4000000
#define max_account_file_memory_size    0x800000
#define max_header_memory_size          0x400       // name chosen; all four header buffers use it

enum
{
    save_memory_game_file,
    save_memory_town_file,
    save_memory_game_header,
    save_memory_town_header,
    save_memory_cloud_file,
    save_memory_cloud_header,
    save_memory_account_file,
    save_memory_account_header,
};

void save_memory_allocate();
void save_memory_free();
byte * save_memory_get( dword type, dword needed_size );

#endif
