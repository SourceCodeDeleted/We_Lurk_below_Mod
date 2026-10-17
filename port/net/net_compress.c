// net_compress.c - packing the save files two copies exchange at the start of a session (hand-written
// wrapper; built on its own with the system's C headers, like the other network files).
//
// A world file is about 20 MB and mostly the same few bytes over and over: packed it is under a tenth of that,
// which on an ordinary home connection is the difference between a joiner waiting a minute and a few seconds.
// The packing is zlib's (deflate), from miniz (third_party/miniz, MIT licence), compiled in here with
// everything but the in-memory packing and unpacking left out.
#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ARCHIVE_WRITING_APIS
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "../../third_party/miniz/miniz.c"

// Packs `size` bytes. Returns a new buffer (free it) and its size in *packed_size, or 0.
unsigned char *net_pack( const unsigned char *data, unsigned int size, unsigned int *packed_size )
{
    mz_ulong bound = mz_compressBound( size ), length = bound;
    unsigned char *packed = malloc( bound ? bound : 1 );
    if ( packed == 0 )
        return 0;
    if ( mz_compress2( packed, &length, data, size, 3 ) != MZ_OK )     // level 3: nearly as small as the best, several times quicker
    {
        free( packed );
        return 0;
    }
    *packed_size = (unsigned int)length;
    return packed;
}

// Unpacks into `out`, which holds exactly `size` bytes. Returns 1 if that many came out.
int net_unpack( const unsigned char *packed, unsigned int packed_size, unsigned char *out, unsigned int size )
{
    mz_ulong length = size;
    return mz_uncompress( out, &length, packed, packed_size ) == MZ_OK && length == size;
}
