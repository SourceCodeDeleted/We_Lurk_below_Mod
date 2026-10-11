//-----------------------------------------------------------------------------
// gge_textures.cpp  (texture half)
// Reconstructed from ILB.exe: gge_textures_init 0x4117E0, gge_textures_free_all 0x411870,
// get_next_texture_index 0x411C20, texture_create 0x411CA0, copy_texture_to_card 0x411E50,
// gge_texture_add_empty_from_gs3file 0x411FF0, gge_texture_update 0x4120B0,
// gge_texture_update_combined_gs3files 0x412120, gge_texture_free 0x4122A0.
// From inlined copies: gge_texture_add_gs3file (in 0x408290), gge_texture_get_texture_rv (0x40EDD0).
//
// Written from their assert text and the pattern of their siblings, because their inlined copies
// were not read line by line: gge_texture_add_from_raw and gge_texture_update_gs3file. Treat those
// two as guesses.
//
// gge_texture_remove_material 0x412390; gge_texture_add_material from its copy inlined in
// gge_material_new.
//
// sort_into_rendering_layers 0x4118F0, gge_render_layers 0x411B20 (name chosen),
// gge_texture_update_subregion 0x412520. From inlined copies: the rendering surface table
// (gge_texture_memory_rendering_surface_add / _remove).
//
// More guesses:
//   - gge_texture_update_monster_texture_with_palette: asserts are original, the body follows
//     gge_texture_update_gs3file with the palette decoder.
//   - gge_texture_update_subregion: the only copy has its arguments folded in. It copies 512 x 256
//     pixels from texture_data, starting y rows down a 512-wide source, to the top of the texture,
//     ignoring the row pitch. The general form written here reduces to that for x 0, width 512,
//     height 256; the map type (write-discard) is assumed.
//   - gge_render_layers' parameter names, and what the per-layer pass table means.

#include "main.h"
#include "gge_main.h"
#include "gge_gs3.h"
#include "gge_textures.h"
#include "gge_materials.h"

struct texture_data_entry       // 20 bytes
{
    bool                        active;
    ID3D11Texture2D *           texture;
    ID3D11ShaderResourceView *  resource_view;
    dword                       memory_size;        // bytes
    dword                       material_id_list;   // head of the list of materials using this texture
};

static dword total_texture_memory = 0;                  // 0x3852034
static int num_textures = 0;                            // 0x3854684
static dword rendering_surface_allocations_tbl[ max_rendering_surface_allocs ];     // 0x3854688: bytes per surface, invalid_id = free
static texture_data_entry textures[ max_textures ];     // 0x38546C8
static dword null_material_id_list = invalid_id;        // 0x5D4538: materials with no texture

// materials grouped by rendering layer, rebuilt each frame
struct rendering_layer_data     // 516 bytes
{
    dword       material_id[ max_materials_per_layer ];
    int         num_materials;
};
static rendering_layer_data rendering_layers[ num_rendering_layers ];      // 0x3852038

// which pass draws each layer in gge_render_layers' mode 1 (0x585E38)
static const int rendering_layer_pass[ num_rendering_layers ] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };

//-----------------------------------------------------------------------------

void gge_textures_init()
{
    assert_return( num_textures == 0 );

    for ( int i = 0; i < max_textures; i++ )
    {
        textures[ i ].active = false;
        textures[ i ].texture = NULL;
        textures[ i ].material_id_list = invalid_id;
        textures[ i ].memory_size = 0;
    }

    null_material_id_list = invalid_id;
    total_texture_memory = 0;
    for ( int i = 0; i < max_rendering_surface_allocs; i++ ) rendering_surface_allocations_tbl[ i ] = invalid_id;
}

//-----------------------------------------------------------------------------

void gge_textures_free_all()
{
    for ( dword i = 0; i < max_textures; i++ )
    {
        if ( textures[ i ].active ) gge_texture_free( i );
    }

    assert_return( null_material_id_list == invalid_id );
    assert_return( num_textures == 0 );
    assert_return( total_texture_memory == 0 );
}

//-----------------------------------------------------------------------------

dword get_next_texture_index()
{
    dword index = 0;
    while ( index < max_textures && textures[ index ].active ) index++;

    if ( !( index < max_textures ) ) { assert_exp( index < max_textures ); return invalid_id; }
    return index;
}

//-----------------------------------------------------------------------------

void texture_create( dword index, int width, int height )
{
    assert_return( index < max_textures );
    assert_return( textures[ index ].texture == NULL );

    D3D11_TEXTURE2D_DESC desc;
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;

    ID3D11Device * device = gge_get_device();

    HRESULT result = device->CreateTexture2D( &desc, NULL, &textures[ index ].texture );
    d3d_check( result );
    result = device->CreateShaderResourceView( textures[ index ].texture, NULL, &textures[ index ].resource_view );
    d3d_check( result );

    textures[ index ].memory_size = width * height * sizeof( dword );
    total_texture_memory += textures[ index ].memory_size;
}

//-----------------------------------------------------------------------------

void copy_texture_to_card( dword index, dword * src, int width, int height )
{
    assert_return( index < max_textures );
    assert_return( textures[ index ].texture );
    assert_return( src );

    dword size = width * height;
    assert_return( size <= textures[ index ].memory_size / sizeof( dword ) );

    ID3D11DeviceContext * context = gge_get_device_context();

    D3D11_MAPPED_SUBRESOURCE d3d_dest;
    d3d_dest.pData = NULL;
    d3d_dest.RowPitch = 0;
    context->Map( textures[ index ].texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &d3d_dest );
    assert_return( d3d_dest.pData );

    dword * dest = (dword *)d3d_dest.pData;
    for ( int y = 0; y < height; y++ )
    {
        for ( int x = 0; x < width; x++ ) dest[ x ] = *src++;
        dest += d3d_dest.RowPitch / sizeof( dword );
    }

    context->Unmap( textures[ index ].texture, 0 );
}

//-----------------------------------------------------------------------------
// new texture holding sprite 0 of a .gs3 file

dword gge_texture_add_gs3file( char * filename )
{
    if ( !( num_textures < max_textures ) ) { assert_exp( num_textures < max_textures ); return invalid_id; }

    dword index = get_next_texture_index();
    if ( !( index != invalid_id ) ) { assert_exp( index != invalid_id ); return invalid_id; }

    textures[ index ].active = true;
    textures[ index ].material_id_list = invalid_id;

    dword sprite_id = gge_gs3_new_sprites( filename );
    int width, height;
    gge_gs3_get_sprite_size( sprite_id, 0, &width, &height );
    texture_create( index, width, height );
    dword * pixels = gge_gs3_decompress( sprite_id, 0, true );
    copy_texture_to_card( index, pixels, width, height );
    gge_gs3_free_sprites( sprite_id );

    num_textures++;
    return index;
}

//-----------------------------------------------------------------------------
// new texture with the size of a .gs3 file's sprite 0, contents not set

dword gge_texture_add_empty_from_gs3file( char * filename )
{
    if ( !( num_textures < max_textures ) ) { assert_exp( num_textures < max_textures ); return invalid_id; }

    dword index = get_next_texture_index();
    if ( !( index != invalid_id ) ) { assert_exp( index != invalid_id ); return invalid_id; }

    textures[ index ].active = true;
    textures[ index ].material_id_list = invalid_id;

    dword sprite_id = gge_gs3_new_sprites( filename );
    int width, height;
    gge_gs3_get_sprite_size( sprite_id, 0, &width, &height );
    texture_create( index, width, height );
    gge_gs3_free_sprites( sprite_id );

    num_textures++;
    return index;
}

//-----------------------------------------------------------------------------
// GUESS (see the file header): asserts are original, the body follows gge_texture_add_gs3file

dword gge_texture_add_from_raw( dword * texture_data, int texture_width, int texture_height )
{
    if ( !texture_data ) { assert_exp( texture_data ); return invalid_id; }
    if ( !( texture_width >= 0 ) ) { assert_exp( texture_width >= 0 ); return invalid_id; }
    if ( !( texture_height >= 0 ) ) { assert_exp( texture_height >= 0 ); return invalid_id; }
    if ( !( num_textures < max_textures ) ) { assert_exp( num_textures < max_textures ); return invalid_id; }

    dword index = get_next_texture_index();
    if ( !( index != invalid_id ) ) { assert_exp( index != invalid_id ); return invalid_id; }

    textures[ index ].active = true;
    textures[ index ].material_id_list = invalid_id;

    texture_create( index, texture_width, texture_height );
    copy_texture_to_card( index, texture_data, texture_width, texture_height );

    num_textures++;
    return index;
}

//-----------------------------------------------------------------------------

void gge_texture_update( dword texture_id, dword * texture_data, int width, int height )
{
    assert_return( texture_id < max_textures );
    assert_return( textures[ texture_id ].active );
    assert_return( texture_data );

    copy_texture_to_card( texture_id, texture_data, width, height );
}

//-----------------------------------------------------------------------------
// GUESS (see the file header): asserts are original, the body follows the combined version below

void gge_texture_update_gs3file( dword texture_id, char * filename )
{
    assert_return( texture_id < max_textures );
    assert_return( textures[ texture_id ].active );

    dword sprite_id = gge_gs3_new_sprites( filename );
    int width, height;
    gge_gs3_get_sprite_size( sprite_id, 0, &width, &height );
    assert_return( textures[ texture_id ].memory_size == width * height * sizeof( dword ) );

    dword * pixels = gge_gs3_decompress( sprite_id, 0, true );
    gge_gs3_free_sprites( sprite_id );
    copy_texture_to_card( texture_id, pixels, width, height );
}

//-----------------------------------------------------------------------------
// Fills a texture with one sprite sheet drawn over another: the overlay's transparent runs leave
// the base showing. Both sheets must have the texture's size.

void gge_texture_update_combined_gs3files( dword texture_id, char * base_filename, char * overlay_filename )
{
    assert_return( texture_id < max_textures );
    assert_return( textures[ texture_id ].active );

    int width, height;

    dword sprite_id = gge_gs3_new_sprites( base_filename );
    assert_return( sprite_id != invalid_id );
    gge_gs3_get_sprite_size( sprite_id, 0, &width, &height );
    assert_return( textures[ texture_id ].memory_size == width * height * sizeof( dword ) );
    dword * pixels = gge_gs3_decompress( sprite_id, 0, true );
    gge_gs3_free_sprites( sprite_id );

    sprite_id = gge_gs3_new_sprites( overlay_filename );
    assert_return( sprite_id != invalid_id );
    gge_gs3_get_sprite_size( sprite_id, 0, &width, &height );
    assert_return( textures[ texture_id ].memory_size == width * height * sizeof( dword ) );
    gge_gs3_decompress( sprite_id, 0, false );
    gge_gs3_free_sprites( sprite_id );

    copy_texture_to_card( texture_id, pixels, width, height );
}

//-----------------------------------------------------------------------------

void gge_texture_free( dword texture_id )
{
    assert_return( texture_id < max_textures );
    assert_return( textures[ texture_id ].active );

    if ( textures[ texture_id ].texture )
    {
        total_texture_memory -= textures[ texture_id ].memory_size;
        textures[ texture_id ].memory_size = 0;
        textures[ texture_id ].texture->Release();
        textures[ texture_id ].texture = NULL;
        if ( textures[ texture_id ].resource_view )
        {
            textures[ texture_id ].resource_view->Release();
            textures[ texture_id ].resource_view = NULL;
        }
    }

    assert_return( textures[ texture_id ].material_id_list == invalid_id );

    num_textures--;
    textures[ texture_id ].material_id_list = invalid_id;
    textures[ texture_id ].active = false;
}

//-----------------------------------------------------------------------------

ID3D11ShaderResourceView * gge_texture_get_texture_rv( dword texture_id )
{
    assert_return_null( texture_id < max_textures );
    assert_return_null( textures[ texture_id ].active == true );
    assert_return_null( textures[ texture_id ].texture != NULL );

    return textures[ texture_id ].resource_view;
}

//-----------------------------------------------------------------------------
// puts a material at the head of its texture's list (or of the no-texture list)

void gge_texture_add_material( dword texture_id, dword material_id )
{
    if ( texture_id == invalid_id )
    {
        gge_material_set_next_id( material_id, null_material_id_list );
        null_material_id_list = material_id;
        return;
    }

    assert_return( texture_id < max_textures );

    gge_material_set_next_id( material_id, textures[ texture_id ].material_id_list );
    textures[ texture_id ].material_id_list = material_id;
}

//-----------------------------------------------------------------------------

void gge_texture_remove_material( dword texture_id, dword material_id )
{
    dword * list_head;

    if ( texture_id == invalid_id )
    {
        assert_return( null_material_id_list != invalid_id );
        list_head = &null_material_id_list;
    }
    else
    {
        assert_return( texture_id < max_textures );
        assert_return( textures[ texture_id ].material_id_list != invalid_id );
        list_head = &textures[ texture_id ].material_id_list;
    }

    dword previous = invalid_id;
    dword list = *list_head;
    while ( list != material_id )
    {
        previous = list;
        list = gge_material_get_next_id( list );
        assert_return( list != invalid_id );
    }

    dword next_id = gge_material_get_next_id( material_id );
    if ( previous == invalid_id ) *list_head = next_id;
    else gge_material_set_next_id( previous, next_id );
}

//-----------------------------------------------------------------------------
// GUESS: see the file header

void gge_texture_update_monster_texture_with_palette( dword texture_id, char * filename, dword * palette )
{
    assert_return( texture_id < max_textures );
    assert_return( textures[ texture_id ].active );

    dword sprite_id = gge_gs3_new_sprites( filename );
    int width, height;
    gge_gs3_get_sprite_size( sprite_id, 0, &width, &height );
    assert_return( textures[ texture_id ].memory_size == width * height * sizeof( dword ) );

    dword * pixels = gge_gs3_decompress_monster_graphics_with_palette( sprite_id, palette );
    gge_gs3_free_sprites( sprite_id );
    copy_texture_to_card( texture_id, pixels, width, height );
}

//-----------------------------------------------------------------------------
// GUESS: see the file header

void gge_texture_update_subregion( dword texture_id, dword x, dword y, dword * texture_data, int width, int height )
{
    assert_return( texture_id < max_textures );
    assert_return( textures[ texture_id ].active == true );
    assert_return( textures[ texture_id ].texture != NULL );
    assert_return( texture_data );

    ID3D11DeviceContext * context = gge_get_device_context();

    D3D11_MAPPED_SUBRESOURCE mapped;
    mapped.pData = NULL;
    mapped.RowPitch = 0;
    HRESULT result = context->Map( textures[ texture_id ].texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
    d3d_check( result );

    memcpy( mapped.pData, texture_data + y * width + x, width * height * sizeof( dword ) );

    context->Unmap( textures[ texture_id ].texture, 0 );
}

//-----------------------------------------------------------------------------
// Render targets are not in the texture table; these two only account for their memory.

dword gge_texture_memory_rendering_surface_add( dword width, dword height )
{
    dword index = 0;
    while ( index < max_rendering_surface_allocs && rendering_surface_allocations_tbl[ index ] != invalid_id ) index++;
    if ( !( index < max_rendering_surface_allocs ) ) { assert_exp( index < max_rendering_surface_allocs ); return invalid_id; }

    dword size = width * height * sizeof( dword );
    total_texture_memory += size;
    rendering_surface_allocations_tbl[ index ] = size;
    return index;
}

void gge_texture_memory_rendering_surface_remove( dword rsa_id )
{
    assert_return( rsa_id < max_rendering_surface_allocs );
    assert_return( rendering_surface_allocations_tbl[ rsa_id ] != invalid_id );

    total_texture_memory -= rendering_surface_allocations_tbl[ rsa_id ];
    rendering_surface_allocations_tbl[ rsa_id ] = invalid_id;
}

//-----------------------------------------------------------------------------
// Puts every material into its rendering layer: first the materials of each texture, then the
// ones without a texture. The caller clears the layer counts first.

static bool rendering_layer_add_list( dword material_id )
{
    while ( material_id != invalid_id )
    {
        int layer = gge_material_get_rendering_layer( material_id );
        int index = rendering_layers[ layer ].num_materials;
        if ( !( index < max_materials_per_layer ) ) { assert_exp( index < max_materials_per_layer ); return false; }

        rendering_layers[ layer ].material_id[ index ] = material_id;
        rendering_layers[ layer ].num_materials++;

        material_id = gge_material_get_next_id( material_id );
    }
    return true;
}

void sort_into_rendering_layers()
{
    for ( int i = 0; i < max_textures; i++ )
    {
        if ( !textures[ i ].active ) continue;
        if ( !rendering_layer_add_list( textures[ i ].material_id_list ) ) return;
    }

    rendering_layer_add_list( null_material_id_list );
}

//-----------------------------------------------------------------------------
// Draws the layers for one view. pass 0 re-sorts the materials first. mode 0 draws every layer in
// screen space; mode 1 draws, with the view's camera, the layers whose table entry equals pass.

void gge_render_layers( dword view_id, int mode, int pass )
{
    if ( pass == 0 )
    {
        for ( int i = 0; i < num_rendering_layers; i++ ) rendering_layers[ i ].num_materials = 0;
        sort_into_rendering_layers();
    }

    if ( mode == 0 )
    {
        for ( int layer = 0; layer < num_rendering_layers; layer++ )
            for ( int i = 0; i < rendering_layers[ layer ].num_materials; i++ )
                gge_material_render( view_id, rendering_layers[ layer ].material_id[ i ], 0 );
    }
    else if ( mode == 1 )
    {
        for ( int layer = 0; layer < num_rendering_layers; layer++ )
        {
            if ( rendering_layer_pass[ layer ] != pass ) continue;
            for ( int i = 0; i < rendering_layers[ layer ].num_materials; i++ )
                gge_material_render( view_id, rendering_layers[ layer ].material_id[ i ], 1 );
        }
    }
}
