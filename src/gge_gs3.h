//-----------------------------------------------------------------------------
// gge_gs3.h
//
// .gs3 sprite files: one or more run-length-encoded images, either 8-bit with a palette of up to
// 256 colours or 32-bit. Pixels are dwords with the bytes in B, G, R, A order (the textures they
// fill are created as DXGI_FORMAT_B8G8R8A8_UNORM).
//
// Original names: all gge_gs3_* functions, gge_gs3_buffer_data, and from assert text sprite_data,
// max_sprite_files, max_gs3_8bit_colors, sprite_type_8bit, num_sprites, num_colors, num_sprite_files, decomp_buffer,
// max_32bit_sprite_scale, gs3_sprite_control_byte_jump, gs3_sprite_control_dword_jump,
// player_texture_width / height, monster_texture_width, monster_texture_height_256 / _128.
// The other control names, sprite_type_32bit and the struct field names are chosen here.

#ifndef _GGE_GS3_H_
#define _GGE_GS3_H_

#define max_sprite_files                1024
#define max_gs3_8bit_colors             256
#define max_32bit_sprite_scale          4096

#define player_texture_width            256
#define player_texture_height           512
#define monster_texture_width           128
#define monster_texture_height_256      256
#define monster_texture_height_128      128

enum
{
    sprite_type_8bit = 0,
    sprite_type_32bit,
};

// run kinds: bits 6-7 of the control byte (8-bit sprites) or bits 14-15 of the control dword
#define gs3_sprite_control_byte_copy        0x00    // literal pixels follow
#define gs3_sprite_control_byte_jump        0x40    // transparent: skip pixels
#define gs3_sprite_control_byte_fill        0x80    // one pixel repeated
#define gs3_sprite_control_byte_mask        0xC0
#define gs3_sprite_control_dword_copy       0x0000
#define gs3_sprite_control_dword_jump       0x4000
#define gs3_sprite_control_dword_fill       0x8000
#define gs3_sprite_control_dword_mask       0xC000

struct gge_gs3_buffer_data      // 28 bytes
{
    dword *     data;
    int         width;          // also the row pitch, in pixels
    int         height;
    int         clip_left;
    int         clip_top;
    int         clip_right;     // exclusive
    int         clip_bottom;
};

void gge_gs3_init();
void gge_gs3_free();
dword gge_gs3_new_sprites( char * filename );
void gge_gs3_free_sprites( dword sprite_id );
void gge_gs3_get_sprite_size( dword sprite_id, int sprite_index, int * width, int * height );
void gge_gs3_change_color( dword sprite_id, dword color_index, dword color );
void gge_gs3_decompress_into_buffer( dword sprite_id, int sprite_index, int x, int y, int scale, gge_gs3_buffer_data * buffer );
dword * gge_gs3_decompress( dword sprite_id, int sprite_index, bool clear );
void gge_gs3_decompress_player_graphics( dword sprite_id, dword * texture_memory );
void gge_gs3_decompress_monster_graphics( dword sprite_id, dword * texture_memory );
dword * gge_gs3_decompress_monster_graphics_with_palette( dword sprite_id, dword * palette );

#endif
