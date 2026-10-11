//-----------------------------------------------------------------------------
// gge_mat_plain.cpp
// Material type 0: shader shaders\\dx11\\plain, 16-byte vertices.
// Reconstructed from ILB.exe: shader name 0x40ff60, layout creation 0x40ff70, free 0x40ffc0, set globals
// 0x40fff0, vertex size 0x4100b0, vertex copy 0x4100c0 (the linker shares identical size and copy
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

struct plain_vertex       // 16 bytes
{
    vector3d position;
    dword color;
};

static D3D11_INPUT_ELEMENT_DESC plain_layout_desc[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static ID3D11InputLayout * plain_layout = NULL;        // 0x383E014

//-----------------------------------------------------------------------------

const char * gge_mat_plain_shader_filename()
{
    return "shaders\\dx11\\plain";
}

//-----------------------------------------------------------------------------

void gge_mat_plain_shader_created( void * vso_data, dword vso_size )
{
    gge_get_device()->CreateInputLayout( plain_layout_desc, 2, vso_data, vso_size, &plain_layout );
}

//-----------------------------------------------------------------------------

void gge_mat_plain_shader_free()
{
    if ( plain_layout )
    {
        plain_layout->Release();
        plain_layout = NULL;
    }
}

//-----------------------------------------------------------------------------

void gge_mat_plain_set_globals( dword view_id, dword material_id, int use_view_matrix )
{
    gge_get_device_context()->IASetInputLayout( plain_layout );
    gge_material_set_shader_vars( view_id, material_id, use_view_matrix );
    gge_material_set_alpha_blend( true );
}

//-----------------------------------------------------------------------------

int gge_mat_plain_vertex_size()
{
    return sizeof( plain_vertex );
}

//-----------------------------------------------------------------------------

void gge_mat_plain_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list )
{
    ID3D11DeviceContext * context = gge_get_device_context();

    D3D11_MAPPED_SUBRESOURCE mapped;
    mapped.pData = NULL;
    mapped.RowPitch = 0;
    context->Map( vertexbuffer, 0, start != 0 ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD, 0, &mapped );

    plain_vertex * dest = (plain_vertex *)mapped.pData + start;
    plain_vertex * source = (plain_vertex *)list;
    for ( int i = 0; i < count; i++ ) dest[ i ] = source[ i ];

    context->Unmap( vertexbuffer, 0 );
}
