// sha256.c - SHA-256 (FIPS 180-4), hand-written for the game side of the lobby's password check. The server
// has the same function in Rust (server/src/sha256.rs); both are tested against the standard's examples.
#include <stdint.h>
#include <string.h>

static const uint32_t K[ 64 ] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

typedef struct { uint32_t state[ 8 ]; uint64_t length; unsigned char block[ 64 ]; unsigned int used; } sha256_context;

static uint32_t rotr( uint32_t x, int n ) { return ( x >> n ) | ( x << ( 32 - n ) ); }

static void compress( uint32_t *state, const unsigned char *block )
{
    uint32_t w[ 64 ], a = state[ 0 ], b = state[ 1 ], c = state[ 2 ], d = state[ 3 ], e = state[ 4 ], f = state[ 5 ], g = state[ 6 ], h = state[ 7 ];
    for ( int i = 0; i < 16; i++ )
        w[ i ] = (uint32_t)block[ 4 * i ] << 24 | (uint32_t)block[ 4 * i + 1 ] << 16 | (uint32_t)block[ 4 * i + 2 ] << 8 | block[ 4 * i + 3 ];
    for ( int i = 16; i < 64; i++ )
        w[ i ] = w[ i - 16 ] + ( rotr( w[ i - 15 ], 7 ) ^ rotr( w[ i - 15 ], 18 ) ^ ( w[ i - 15 ] >> 3 ) )
               + w[ i - 7 ] + ( rotr( w[ i - 2 ], 17 ) ^ rotr( w[ i - 2 ], 19 ) ^ ( w[ i - 2 ] >> 10 ) );
    for ( int i = 0; i < 64; i++ )
    {
        uint32_t t1 = h + ( rotr( e, 6 ) ^ rotr( e, 11 ) ^ rotr( e, 25 ) ) + ( ( e & f ) ^ ( ~e & g ) ) + K[ i ] + w[ i ];
        uint32_t t2 = ( rotr( a, 2 ) ^ rotr( a, 13 ) ^ rotr( a, 22 ) ) + ( ( a & b ) ^ ( a & c ) ^ ( b & c ) );
        h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    state[ 0 ] += a; state[ 1 ] += b; state[ 2 ] += c; state[ 3 ] += d; state[ 4 ] += e; state[ 5 ] += f; state[ 6 ] += g; state[ 7 ] += h;
}

static void start( sha256_context *c )
{
    static const uint32_t initial[ 8 ] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    memcpy( c->state, initial, sizeof initial );
    c->length = 0;
    c->used = 0;
}

static void add( sha256_context *c, const void *data, size_t size )
{
    const unsigned char *p = data;
    c->length += size;
    while ( size-- )
    {
        c->block[ c->used++ ] = *p++;
        if ( c->used == 64 ) { compress( c->state, c->block ); c->used = 0; }
    }
}

static void finish( sha256_context *c, char hex[ 65 ] )
{
    static const char digits[] = "0123456789abcdef";
    uint64_t bits = c->length * 8;
    unsigned char pad = 0x80, zero = 0, length[ 8 ];
    add( c, &pad, 1 );
    while ( c->used != 56 ) add( c, &zero, 1 );
    for ( int i = 0; i < 8; i++ ) length[ i ] = (unsigned char)( bits >> ( 56 - 8 * i ) );
    add( c, length, 8 );
    for ( int i = 0; i < 32; i++ )
    {
        unsigned char byte = (unsigned char)( c->state[ i / 4 ] >> ( 24 - 8 * ( i % 4 ) ) );
        hex[ 2 * i ] = digits[ byte >> 4 ];
        hex[ 2 * i + 1 ] = digits[ byte & 15 ];
    }
    hex[ 64 ] = 0;
}

// The digest of two pieces of data one after the other (the second may be empty), as 64 hex characters.
void sha256_hex( const void *first, size_t first_size, const void *second, size_t second_size, char hex[ 65 ] )
{
    sha256_context c;
    start( &c );
    add( &c, first, first_size );
    add( &c, second, second_size );
    finish( &c, hex );
}

// The digest of a file's contents; returns 0 if it cannot be read.
#include <stdio.h>
int sha256_file_hex( const char *path, char hex[ 65 ] )
{
    sha256_context c;
    unsigned char buffer[ 65536 ];
    size_t got;
    FILE *file = fopen( path, "rb" );
    if ( file == 0 ) return 0;
    start( &c );
    while ( ( got = fread( buffer, 1, sizeof buffer, file ) ) > 0 ) add( &c, buffer, got );
    fclose( file );
    finish( &c, hex );
    return 1;
}
