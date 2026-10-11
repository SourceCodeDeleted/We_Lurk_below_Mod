//-----------------------------------------------------------------------------
// gge_mat_sprite.cpp
// Material type 3: shader shaders\\dx11\\sprite, 28-byte vertices.
// Reconstructed from ILB.exe: shader name 0x4101a0, layout creation 0x4101b0, free 0x410200, set globals
// 0x410220, vertex size 0x40f2b0, vertex copy 0x40f900 (the linker shares identical size and copy
// functions between types, so several types point at the same address).
//
// Function names follow gge_mat_alpha_sprite_ui_copy_vertices, the one name the binary preserves;
// the other five suffixes and the vertex struct's field names are chosen here.

#include "main.h"
#include "ec_config.h"
#include "ec_vector.h"
#include "gge_main.h"
#include "gge_materials.h"
#include "gge_material_types.h"

struct sprite_vertex       // 28 bytes
{
    vector3d position;
    vector2d uv;
    float blend_weight;
    dword color;
};

static D3D11_INPUT_ELEMENT_DESC sprite_layout_desc[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "BLENDWEIGHT", 0, DXGI_FORMAT_R32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static ID3D11InputLayout * sprite_layout = NULL;        // 0x383E018

//-----------------------------------------------------------------------------

const char * gge_mat_sprite_shader_filename()
{
    return "shaders\\dx11\\sprite";
}

//-----------------------------------------------------------------------------

void gge_mat_sprite_shader_created( void * vso_data, dword vso_size )
{
    gge_get_device()->CreateInputLayout( sprite_layout_desc, 4, vso_data, vso_size, &sprite_layout );
}

//-----------------------------------------------------------------------------

void gge_mat_sprite_shader_free()
{
    if ( sprite_layout )
    {
        sprite_layout->Release();
        sprite_layout = NULL;
    }
}

//-----------------------------------------------------------------------------

void gge_mat_sprite_set_globals( dword view_id, dword material_id, int use_view_matrix )
{
    gge_get_device_context()->IASetInputLayout( sprite_layout );
    gge_material_set_shader_vars( view_id, material_id, use_view_matrix );
    gge_material_set_shader_sampler( material_id, material_sampler_point_clamp );
    gge_material_set_alpha_blend( true );
}

//-----------------------------------------------------------------------------

int gge_mat_sprite_vertex_size()
{
    return sizeof( sprite_vertex );
}

//-----------------------------------------------------------------------------

void gge_mat_sprite_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list )
{
    ID3D11DeviceContext * context = gge_get_device_context();

    D3D11_MAPPED_SUBRESOURCE mapped;
    mapped.pData = NULL;
    mapped.RowPitch = 0;
    context->Map( vertexbuffer, 0, start != 0 ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD, 0, &mapped );

    sprite_vertex * dest = (sprite_vertex *)mapped.pData + start;
    sprite_vertex * source = (sprite_vertex *)list;
    for ( int i = 0; i < count; i++ ) dest[ i ] = source[ i ];

    context->Unmap( vertexbuffer, 0 );
}
