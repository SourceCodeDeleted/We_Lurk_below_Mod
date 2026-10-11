//-----------------------------------------------------------------------------
// gge_textures.h
//
// Textures: up to 256 dynamic 32-bit (B8G8R8A8) Direct3D textures, mostly filled from .gs3 sprite
// files. Each texture also heads a list of the materials that use it; materials without a
// texture are kept on a separate list.
//
// Function names are original. The gge_texture_safe_* helpers are inline in the original header
// (their asserts carry this file's name with lines 77 - 98).

#ifndef _GGE_TEXTURES_H_
#define _GGE_TEXTURES_H_

#define max_textures                    256
#define max_rendering_surface_allocs    16
#define num_rendering_layers            19      // name chosen; the count is from the table size
#define max_materials_per_layer         128

struct ID3D11ShaderResourceView;

void gge_textures_init();
void gge_textures_free_all();
dword get_next_texture_index();
void texture_create( dword index, int width, int height );
void copy_texture_to_card( dword index, dword * src, int width, int height );
dword gge_texture_add_gs3file( char * filename );
dword gge_texture_add_empty_from_gs3file( char * filename );
dword gge_texture_add_from_raw( dword * texture_data, int texture_width, int texture_height );
void gge_texture_update( dword texture_id, dword * texture_data, int width, int height );
void gge_texture_update_gs3file( dword texture_id, char * filename );
void gge_texture_update_combined_gs3files( dword texture_id, char * base_filename, char * overlay_filename );
void gge_texture_free( dword texture_id );
ID3D11ShaderResourceView * gge_texture_get_texture_rv( dword texture_id );
void gge_texture_add_material( dword texture_id, dword material_id );
void gge_texture_remove_material( dword texture_id, dword material_id );
void gge_texture_update_monster_texture_with_palette( dword texture_id, char * filename, dword * palette );
void gge_texture_update_subregion( dword texture_id, dword x, dword y, dword * texture_data, int width, int height );
dword gge_texture_memory_rendering_surface_add( dword width, dword height );
void gge_texture_memory_rendering_surface_remove( dword rsa_id );
void sort_into_rendering_layers();
void gge_render_layers( dword view_id, int mode, int pass );      // 0x411B20, name chosen

//-----------------------------------------------------------------------------

inline void gge_texture_safe_new( dword & new_id, char * filename )
{
    assert_return( new_id == invalid_id );
    new_id = gge_texture_add_gs3file( filename );
    assert_return( new_id != invalid_id );
}

inline void gge_texture_safe_new_empty_from_file( dword & new_id, char * filename )
{
    assert_return( new_id == invalid_id );
    new_id = gge_texture_add_empty_from_gs3file( filename );
    assert_return( new_id != invalid_id );
}

inline void gge_texture_safe_new_from_raw( dword & new_id, dword * texture_data, int texture_width, int texture_height )
{
    assert_return( new_id == invalid_id );
    new_id = gge_texture_add_from_raw( texture_data, texture_width, texture_height );
    assert_return( new_id != invalid_id );
}

inline void gge_texture_safe_free( dword & id )
{
    assert_return( id != invalid_id );
    gge_texture_free( id );
    id = invalid_id;
}

#endif
