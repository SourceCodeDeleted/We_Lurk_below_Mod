//-----------------------------------------------------------------------------
// gge_mat_alpha_ui.cpp
// Material type 7: shader shaders\\dx11\\alpha_ui, 20-byte vertices.
// Reconstructed from ILB.exe: shader name 0x40f430, layout creation 0x40f440, free 0x40f490, set globals
// 0x40f4b0, vertex size 0x40f570, vertex copy 0x40f580 (the linker shares identical size and copy
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

struct alpha_ui_vertex       // 20 bytes
{
    float position[ 4 ];
    dword color;
};

static D3D11_INPUT_ELEMENT_DESC alpha_ui_layout_desc[] =
{
    { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static ID3D11InputLayout * alpha_ui_layout = NULL;        // 0x383DFF8

//-----------------------------------------------------------------------------

const char * gge_mat_alpha_ui_shader_filename()
{
    return "shaders\\dx11\\alpha_ui";
}

//-----------------------------------------------------------------------------

void gge_mat_alpha_ui_shader_created( void * vso_data, dword vso_size )
{
    gge_get_device()->CreateInputLayout( alpha_ui_layout_desc, 2, vso_data, vso_size, &alpha_ui_layout );
}

//-----------------------------------------------------------------------------

void gge_mat_alpha_ui_shader_free()
{
    if ( alpha_ui_layout )
    {
        alpha_ui_layout->Release();
        alpha_ui_layout = NULL;
    }
}

//-----------------------------------------------------------------------------

void gge_mat_alpha_ui_set_globals( dword view_id, dword material_id, int use_view_matrix )
{
    gge_get_device_context()->IASetInputLayout( alpha_ui_layout );
    gge_material_set_shader_vars( view_id, material_id, use_view_matrix );
    gge_material_set_alpha_blend( true );
}

//-----------------------------------------------------------------------------

int gge_mat_alpha_ui_vertex_size()
{
    return sizeof( alpha_ui_vertex );
}

//-----------------------------------------------------------------------------

void gge_mat_alpha_ui_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list )
{
    ID3D11DeviceContext * context = gge_get_device_context();

    D3D11_MAPPED_SUBRESOURCE mapped;
    mapped.pData = NULL;
    mapped.RowPitch = 0;
    context->Map( vertexbuffer, 0, start != 0 ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD, 0, &mapped );

    alpha_ui_vertex * dest = (alpha_ui_vertex *)mapped.pData + start;
    alpha_ui_vertex * source = (alpha_ui_vertex *)list;
    for ( int i = 0; i < count; i++ ) dest[ i ] = source[ i ];

    context->Unmap( vertexbuffer, 0 );
}
