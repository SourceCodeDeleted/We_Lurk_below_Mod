//-----------------------------------------------------------------------------
// gge_gs3.cpp
// Reconstructed from ILB.exe: gge_gs3_new_sprites 0x40ABD0, gge_gs3_free_sprites 0x40ACE0,
// gge_gs3_get_sprite_size 0x40AD90, gge_gs3_change_color 0x40AEB0, pixel writer 0x40AF60,
// gge_gs3_decompress_into_buffer 0x40B000, gge_gs3_decompress 0x40B4C0,
// gge_gs3_decompress_player_graphics 0x40B5C0, gge_gs3_decompress_monster_graphics 0x40B7B0,
// gge_gs3_decompress_monster_graphics_with_palette 0x40B9A0.
//
// Guess: every caller in the binary passes sprite_index 0, x 0, y 0 and scale 1 to
// gge_gs3_decompress_into_buffer, and the compiler folded those constants in. The handling of
// other values here (x / y as a destination offset, scale as a pixel size) is what the leftover
// loop shapes in the pixel writer suggest, not something the binary confirms.
//
// gge_gs3_init 0x40AAB0 and gge_gs3_free 0x40AB40 are only reached through the graphics module
// table at 0x5D42A0 (init / free pairs for gs3, textures, materials, models, ...).

#include "main.h"
#include "ec_fileio.h"
#include "gge_gs3.h"

struct gs3_sprite_file      // 1048 bytes
{
    bool        active;
    int         sprite_type;
    byte *      data;
    dword       num_colors;
    dword       palette[ max_gs3_8bit_colors ];
    int         num_sprites;
    dword *     offsets;            // per sprite, from the start of the file
};

static gge_gs3_buffer_data decomp_buffer;                   // 0x3735D54
static int num_sprite_files = 0;                            // 0x3735D70
static gs3_sprite_file sprite_data[ max_sprite_files ];     // 0x3735D78

//-----------------------------------------------------------------------------

void gge_gs3_init()
{
    assert_return( decomp_buffer.data == NULL );
    decomp_buffer.data = (dword *)drb_malloc( max_32bit_sprite_scale * max_32bit_sprite_scale * sizeof( dword ), __FILE__, __LINE__, NULL );
    decomp_buffer.clip_left = 0;
    decomp_buffer.clip_top = 0;

    for ( int i = 0; i < max_sprite_files; i++ )
    {
        sprite_data[ i ].active = false;
        sprite_data[ i ].data = NULL;
        sprite_data[ i ].offsets = NULL;
    }
    num_sprite_files = 0;
}

//-----------------------------------------------------------------------------

void gge_gs3_free()
{
    assert_return( decomp_buffer.data );

    drb_free( decomp_buffer.data );
    decomp_buffer.data = NULL;
    decomp_buffer.width = 0;
    decomp_buffer.height = 0;

    for ( int i = 0; i < max_sprite_files; i++ )
    {
        if ( sprite_data[ i ].active ) gge_gs3_free_sprites( i );
    }

    assert_return( num_sprite_files == 0 );
}

//-----------------------------------------------------------------------------

dword gge_gs3_new_sprites( char * filename )
{
    if ( !filename ) { assert_exp( filename ); return invalid_id; }

    dword index = 0;
    while ( index < max_sprite_files && sprite_data[ index ].active ) index++;
    if ( !( index < max_sprite_files ) ) { assert_exp( index < max_sprite_files ); return invalid_id; }

    sprite_data[ index ].active = true;
    sprite_data[ index ].data = pack_file_load( filename, NULL );
    if ( !( sprite_data[index].data != NULL ) ) { assert_exp( sprite_data[index].data != NULL ); return invalid_id; }

    dword * source = (dword *)sprite_data[ index ].data;
    sprite_data[ index ].num_colors = *source++;

    if ( sprite_data[ index ].num_colors == invalid_id )
    {
        sprite_data[ index ].sprite_type = sprite_type_32bit;
    }
    else
    {
        sprite_data[ index ].sprite_type = sprite_type_8bit;
        if ( !( sprite_data[index].num_colors <= max_gs3_8bit_colors ) ) { assert_exp( sprite_data[index].num_colors <= max_gs3_8bit_colors ); return invalid_id; }
        for ( dword i = 0; i < sprite_data[ index ].num_colors; i++ ) sprite_data[ index ].palette[ i ] = *source++;
    }

    num_sprite_files++;
    sprite_data[ index ].num_sprites = *source++;
    sprite_data[ index ].offsets = source;

    return index;
}

//-----------------------------------------------------------------------------

void gge_gs3_free_sprites( dword sprite_id )
{
    assert_return( sprite_id != invalid_id );
    assert_return( sprite_id < max_sprite_files );
    assert_return( sprite_data[sprite_id].active );

    pack_file_close( sprite_data[ sprite_id ].data );
    num_sprite_files--;
    sprite_data[ sprite_id ].data = NULL;
    sprite_data[ sprite_id ].offsets = NULL;
    sprite_data[ sprite_id ].active = false;
}

//-----------------------------------------------------------------------------
// a sprite starts with two words: height - 1, then width - 1

void gge_gs3_get_sprite_size( dword sprite_id, int sprite_index, int * width, int * height )
{
    assert_return( sprite_id != invalid_id );
    assert_return( sprite_id < max_sprite_files );
    assert_return( sprite_data[sprite_id].active );
    assert_return( sprite_data[sprite_id].data );
    assert_return( sprite_index < sprite_data[sprite_id].num_sprites );
    assert_return( width );
    assert_return( height );

    word * sprite = (word *)( sprite_data[ sprite_id ].data + sprite_data[ sprite_id ].offsets[ sprite_index ] );
    *width = sprite[ 1 ] + 1;
    *height = sprite[ 0 ] + 1;
}

//-----------------------------------------------------------------------------

void gge_gs3_change_color( dword sprite_id, dword color_index, dword color )
{
    assert_return( sprite_id != invalid_id );
    assert_return( sprite_id < max_sprite_files );
    assert_return( sprite_data[sprite_id].active );
    assert_return( sprite_data[sprite_id].sprite_type == sprite_type_8bit );
    assert_return( color_index >= 0 && color_index < 256 );

    sprite_data[ sprite_id ].palette[ color_index ] = color;
}

//-----------------------------------------------------------------------------
// writes one sprite pixel as a scale x scale block, optionally clipped

static void gs3_put_pixel( gge_gs3_buffer_data * buffer, int x, int y, dword color, int scale, bool clip )
{
    for ( int py = y; py < y + scale; py++ )
    {
        if ( clip && ( py < buffer->clip_top || py >= buffer->clip_bottom ) ) continue;
        for ( int px = x; px < x + scale; px++ )
        {
            if ( clip && ( px < buffer->clip_left || px >= buffer->clip_right ) ) continue;
            buffer->data[ py * buffer->width + px ] = color;
        }
    }
}

//-----------------------------------------------------------------------------
// Transparent runs leave the buffer untouched. When the sprite lies fully inside the clip
// rectangle the rows are written directly; otherwise every pixel goes through gs3_put_pixel.

void gge_gs3_decompress_into_buffer( dword sprite_id, int sprite_index, int x, int y, int scale, gge_gs3_buffer_data * buffer )
{
    assert_return( sprite_id != invalid_id );
    assert_return( sprite_id < max_sprite_files );
    assert_return( sprite_data[sprite_id].active );
    assert_return( sprite_data[sprite_id].data );
    assert_return( sprite_index < sprite_data[sprite_id].num_sprites );
    assert_return( buffer );

    gs3_sprite_file * file = &sprite_data[ sprite_id ];
    byte * source = file->data + file->offsets[ sprite_index ];
    int height = ( (word *)source )[ 0 ] + 1;
    int width = ( (word *)source )[ 1 ] + 1;
    source += 4;

    if ( x + width * scale < buffer->clip_left ) return;
    if ( y + height * scale < buffer->clip_top ) return;
    if ( buffer->clip_right <= x || buffer->clip_bottom <= y ) return;

    bool clip = buffer->clip_left > x || x + width * scale > buffer->clip_right ||
                buffer->clip_top > y || y + height * scale > buffer->clip_bottom;
    bool direct = !clip && scale == 1;

    for ( int row = 0; row < height; row++ )
    {
        dword * dest = &buffer->data[ ( y + row ) * buffer->width + x ];
        int column = 0;

        while ( column < width )
        {
            int count;
            int ctrl;

            if ( file->sprite_type == sprite_type_32bit )
            {
                dword control = *(dword *)source;
                source += 4;
                count = ( control & ~gs3_sprite_control_dword_mask ) + 1;
                ctrl = control & gs3_sprite_control_dword_mask;

                if ( ctrl == gs3_sprite_control_dword_copy )
                {
                    for ( int i = 0; i < count; i++ )
                    {
                        dword color = *(dword *)source;
                        source += 4;
                        if ( direct ) dest[ column + i ] = color;
                        else gs3_put_pixel( buffer, x + ( column + i ) * scale, y + row * scale, color, scale, clip );
                    }
                }
                else if ( ctrl == gs3_sprite_control_dword_fill )
                {
                    dword color = *(dword *)source;
                    source += 4;
                    for ( int i = 0; i < count; i++ )
                    {
                        if ( direct ) dest[ column + i ] = color;
                        else gs3_put_pixel( buffer, x + ( column + i ) * scale, y + row * scale, color, scale, clip );
                    }
                }
                else
                {
                    assert_return( ctrl == gs3_sprite_control_dword_jump );
                }
            }
            else
            {
                ctrl = source[ 0 ] & gs3_sprite_control_byte_mask;
                count = source[ 1 ] + 1;
                source += 2;

                if ( ctrl == gs3_sprite_control_byte_copy )
                {
                    for ( int i = 0; i < count; i++ )
                    {
                        dword color = file->palette[ *source++ ];
                        if ( direct ) dest[ column + i ] = color;
                        else gs3_put_pixel( buffer, x + ( column + i ) * scale, y + row * scale, color, scale, clip );
                    }
                }
                else if ( ctrl == gs3_sprite_control_byte_fill )
                {
                    dword color = file->palette[ *source++ ];
                    for ( int i = 0; i < count; i++ )
                    {
                        if ( direct ) dest[ column + i ] = color;
                        else gs3_put_pixel( buffer, x + ( column + i ) * scale, y + row * scale, color, scale, clip );
                    }
                }
                else
                {
                    assert_return( ctrl == gs3_sprite_control_byte_jump );
                }
            }

            column += count;
        }
    }
}

//-----------------------------------------------------------------------------
// decompresses one sprite into the shared buffer and returns its pixels

dword * gge_gs3_decompress( dword sprite_id, int sprite_index, bool clear )
{
    assert_return_null( sprite_id != invalid_id );
    assert_return_null( decomp_buffer.data );

    gge_gs3_get_sprite_size( sprite_id, sprite_index, &decomp_buffer.width, &decomp_buffer.height );
    assert_return_null( decomp_buffer.width <= max_32bit_sprite_scale );
    assert_return_null( decomp_buffer.height <= max_32bit_sprite_scale );

    decomp_buffer.clip_left = 0;
    decomp_buffer.clip_top = 0;
    decomp_buffer.clip_right = decomp_buffer.width;
    decomp_buffer.clip_bottom = decomp_buffer.height;

    if ( clear ) memset( decomp_buffer.data, 0, decomp_buffer.width * decomp_buffer.height * sizeof( dword ) );

    gge_gs3_decompress_into_buffer( sprite_id, sprite_index, 0, 0, 1, &decomp_buffer );
    return decomp_buffer.data;
}

//-----------------------------------------------------------------------------
// The three functions below decode sprite 0 of an 8-bit file straight into rows of a fixed width.
// `palette` replaces the file's own palette when given.

static bool gs3_decompress_8bit_rows( gs3_sprite_file * file, byte * source, dword * dest, int width, int height, dword * palette )
{
    if ( palette == NULL ) palette = file->palette;

    for ( int row = 0; row < height; row++ )
    {
        int column = 0;
        while ( column < width )
        {
            int ctrl = source[ 0 ] & gs3_sprite_control_byte_mask;
            int count = source[ 1 ] + 1;
            source += 2;

            if ( ctrl == gs3_sprite_control_byte_copy )
            {
                for ( int i = 0; i < count; i++ ) dest[ column + i ] = palette[ *source++ ];
            }
            else if ( ctrl == gs3_sprite_control_byte_fill )
            {
                dword color = palette[ *source++ ];
                for ( int i = 0; i < count; i++ ) dest[ column + i ] = color;
            }
            else if ( ctrl != gs3_sprite_control_byte_jump )
            {
                return false;
            }

            column += count;
        }
        dest += width;
    }

    return true;
}

//-----------------------------------------------------------------------------

void gge_gs3_decompress_player_graphics( dword sprite_id, dword * texture_memory )
{
    assert_return( sprite_id != invalid_id );
    assert_return( sprite_id < max_sprite_files );
    assert_return( sprite_data[sprite_id].active );
    assert_return( sprite_data[sprite_id].data );
    assert_return( texture_memory );

    word * sprite = (word *)( sprite_data[ sprite_id ].data + sprite_data[ sprite_id ].offsets[ 0 ] );
    int width = sprite[ 1 ] + 1;
    assert_return( width == player_texture_width );
    int height = sprite[ 0 ] + 1;
    assert_return( height <= player_texture_height );

    assert_return( sprite_data[sprite_id].sprite_type == sprite_type_8bit );

    bool ctrl_ok = gs3_decompress_8bit_rows( &sprite_data[ sprite_id ], (byte *)( sprite + 2 ), texture_memory, width, height, NULL );
    assert_exp( ctrl_ok );      // the original asserts ctrl == gs3_sprite_control_byte_jump at the failing run
}

//-----------------------------------------------------------------------------

void gge_gs3_decompress_monster_graphics( dword sprite_id, dword * texture_memory )
{
    assert_return( sprite_id != invalid_id );
    assert_return( sprite_id < max_sprite_files );
    assert_return( sprite_data[sprite_id].active );
    assert_return( sprite_data[sprite_id].data );
    assert_return( texture_memory );

    word * sprite = (word *)( sprite_data[ sprite_id ].data + sprite_data[ sprite_id ].offsets[ 0 ] );
    int width = sprite[ 1 ] + 1;
    assert_return( width == monster_texture_width );
    int height = sprite[ 0 ] + 1;
    assert_return( ( height == monster_texture_height_256 ) || ( height == monster_texture_height_128 ) );

    assert_return( sprite_data[sprite_id].sprite_type == sprite_type_8bit );

    bool ctrl_ok = gs3_decompress_8bit_rows( &sprite_data[ sprite_id ], (byte *)( sprite + 2 ), texture_memory, width, height, NULL );
    assert_exp( ctrl_ok );
}

//-----------------------------------------------------------------------------
// as above, but through the caller's palette and into the shared buffer, which is cleared first

dword * gge_gs3_decompress_monster_graphics_with_palette( dword sprite_id, dword * palette )
{
    assert_return_null( sprite_id != invalid_id );
    assert_return_null( sprite_id < max_sprite_files );
    assert_return_null( sprite_data[sprite_id].active );
    assert_return_null( sprite_data[sprite_id].data );

    word * sprite = (word *)( sprite_data[ sprite_id ].data + sprite_data[ sprite_id ].offsets[ 0 ] );
    int width = sprite[ 1 ] + 1;
    assert_return_null( width == monster_texture_width );
    int height = sprite[ 0 ] + 1;
    assert_return_null( ( height == monster_texture_height_256 ) || ( height == monster_texture_height_128 ) );

    assert_return_null( sprite_data[sprite_id].sprite_type == sprite_type_8bit );
    assert_return_null( decomp_buffer.data );

    decomp_buffer.width = monster_texture_width;
    decomp_buffer.height = height;
    assert_return_null( decomp_buffer.height <= max_32bit_sprite_scale );
    decomp_buffer.clip_left = 0;
    decomp_buffer.clip_top = 0;
    decomp_buffer.clip_right = monster_texture_width;
    decomp_buffer.clip_bottom = height;

    memset( decomp_buffer.data, 0, height * monster_texture_width * sizeof( dword ) );

    bool ctrl_ok = gs3_decompress_8bit_rows( &sprite_data[ sprite_id ], (byte *)( sprite + 2 ), decomp_buffer.data, width, height, palette );
    assert_exp( ctrl_ok );

    return decomp_buffer.data;
}
