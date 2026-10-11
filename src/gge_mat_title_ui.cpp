//-----------------------------------------------------------------------------
// gge_mat_title_ui.cpp
// Material type 9: shader shaders\\dx11\\title_ui, 28-byte vertices.
// Reconstructed from ILB.exe: shader name 0x4102f0, layout creation 0x410300, free 0x410350, set globals
// 0x410370, vertex size 0x40f2b0, vertex copy 0x40f900 (the linker shares identical size and copy
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

struct title_ui_vertex       // 28 bytes
{
    float position[ 4 ];
    vector2d uv;
    dword color;
};

static D3D11_INPUT_ELEMENT_DESC title_ui_layout_desc[] =
{
    { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static ID3D11InputLayout * title_ui_layout = NULL;        // 0x383E01C

//-----------------------------------------------------------------------------

const char * gge_mat_title_ui_shader_filename()
{
    return "shaders\\dx11\\title_ui";
}

//-----------------------------------------------------------------------------

void gge_mat_title_ui_shader_created( void * vso_data, dword vso_size )
{
    gge_get_device()->CreateInputLayout( title_ui_layout_desc, 3, vso_data, vso_size, &title_ui_layout );
}

//-----------------------------------------------------------------------------

void gge_mat_title_ui_shader_free()
{
    if ( title_ui_layout )
    {
        title_ui_layout->Release();
        title_ui_layout = NULL;
    }
}

//-----------------------------------------------------------------------------

void gge_mat_title_ui_set_globals( dword view_id, dword material_id, int use_view_matrix )
{
    gge_get_device_context()->IASetInputLayout( title_ui_layout );
    gge_material_set_shader_vars( view_id, material_id, use_view_matrix );
    gge_material_set_shader_sampler( material_id, material_sampler_point_clamp );
    gge_material_set_alpha_blend( true );
}

//-----------------------------------------------------------------------------

int gge_mat_title_ui_vertex_size()
{
    return sizeof( title_ui_vertex );
}

//-----------------------------------------------------------------------------

void gge_mat_title_ui_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list )
{
    ID3D11DeviceContext * context = gge_get_device_context();

    D3D11_MAPPED_SUBRESOURCE mapped;
    mapped.pData = NULL;
    mapped.RowPitch = 0;
    context->Map( vertexbuffer, 0, start != 0 ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD, 0, &mapped );

    title_ui_vertex * dest = (title_ui_vertex *)mapped.pData + start;
    title_ui_vertex * source = (title_ui_vertex *)list;
    for ( int i = 0; i < count; i++ ) dest[ i ] = source[ i ];

    context->Unmap( vertexbuffer, 0 );
}
