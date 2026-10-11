//-----------------------------------------------------------------------------
// gge_mat_parallax.cpp
// Material type 5: shader shaders\\dx11\\parallax, 28-byte vertices.
// Reconstructed from ILB.exe: shader name 0x40fe10, layout creation 0x40fe20, free 0x40fe70, set globals
// 0x40fe90, vertex size 0x40f2b0, vertex copy 0x40f900 (the linker shares identical size and copy
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

struct parallax_vertex       // 28 bytes
{
    float position[ 4 ];
    vector2d uv;
    dword color;
};

static D3D11_INPUT_ELEMENT_DESC parallax_layout_desc[] =
{
    { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static ID3D11InputLayout * parallax_layout = NULL;        // 0x383E010

//-----------------------------------------------------------------------------

const char * gge_mat_parallax_shader_filename()
{
    return "shaders\\dx11\\parallax";
}

//-----------------------------------------------------------------------------

void gge_mat_parallax_shader_created( void * vso_data, dword vso_size )
{
    gge_get_device()->CreateInputLayout( parallax_layout_desc, 3, vso_data, vso_size, &parallax_layout );
}

//-----------------------------------------------------------------------------

void gge_mat_parallax_shader_free()
{
    if ( parallax_layout )
    {
        parallax_layout->Release();
        parallax_layout = NULL;
    }
}

//-----------------------------------------------------------------------------

void gge_mat_parallax_set_globals( dword view_id, dword material_id, int use_view_matrix )
{
    gge_get_device_context()->IASetInputLayout( parallax_layout );
    gge_material_set_shader_vars( view_id, material_id, use_view_matrix );
    gge_material_set_shader_sampler( material_id, material_sampler_point_clamp );
    gge_material_set_alpha_blend( false );
}

//-----------------------------------------------------------------------------

int gge_mat_parallax_vertex_size()
{
    return sizeof( parallax_vertex );
}

//-----------------------------------------------------------------------------

void gge_mat_parallax_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list )
{
    ID3D11DeviceContext * context = gge_get_device_context();

    D3D11_MAPPED_SUBRESOURCE mapped;
    mapped.pData = NULL;
    mapped.RowPitch = 0;
    context->Map( vertexbuffer, 0, start != 0 ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD, 0, &mapped );

    parallax_vertex * dest = (parallax_vertex *)mapped.pData + start;
    parallax_vertex * source = (parallax_vertex *)list;
    for ( int i = 0; i < count; i++ ) dest[ i ] = source[ i ];

    context->Unmap( vertexbuffer, 0 );
}
