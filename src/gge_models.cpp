//-----------------------------------------------------------------------------
// gge_models.cpp
// Reconstructed from ILB.exe: index copy helper 0x4106E0, gge_models_init 0x4107A0, gge_models_free
// 0x4109C0, create_vertex_buffer 0x410A60, gge_model_new 0x410B60, gge_model_indexed_new 0x410CF0,
// gge_model_standard_indexed_new 0x410F70, gge_model_add_vertices 0x411140,
// gge_model_add_indexed_vertices 0x411200, gge_model_add_standard_indexed_vertices 0x411300,
// gge_model_free 0x4113E0, gge_model_render 0x4115D0, gge_model_set_next_id 0x411780.
// From inlined copies: gge_model_reset_vertices, gge_model_should_render, gge_model_get_next_id.
//
// Guess: gge_model_do_vertices_fit's body. Only its assert was read from an inlined copy; the
// comparison is what the name and its use (checking before adding) imply.
//
// copy_indices and the struct fields not named in asserts are names chosen here.

#include "main.h"
#include "gge_main.h"
#include "gge_materials.h"
#include "gge_models.h"

struct model_data       // 48 bytes
{
    dword           flags;
    ID3D11Buffer *  vertexbuffer;       // NULL = unused slot
    dword           vertex_buffer_size; // bytes
    ID3D11Buffer *  indexbuffer;
    dword           index_buffer_size;  // bytes
    dword           unknown_14;         // set to 0 on creation, not used by this file
    int             num_vertices;
    int             max_vertices;
    int             num_indices;
    int             max_indices;
    dword           material_id;
    dword           next_id;            // next model using the same material
};

static dword total_vertex_memory = 0;                               // 0x383E028
static int num_models = 0;                                          // 0x383E02C
static word standard_indices[ standard_max_index_buffer_size ];    // 0x383E030
static model_data models[ max_models ];                             // 0x3846030
static ID3D11Buffer * standard_index_buffer = NULL;                 // 0x3852030

//-----------------------------------------------------------------------------
// writes indices into an index buffer starting at `start`; a write from 0 discards the old contents

static void copy_indices( ID3D11Buffer * indexbuffer, int start, int count, word * indices )
{
    ID3D11DeviceContext * context = gge_get_device_context();

    D3D11_MAPPED_SUBRESOURCE mapped;
    mapped.pData = NULL;
    mapped.RowPitch = 0;
    context->Map( indexbuffer, 0, start != 0 ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD, 0, &mapped );

    word * dest = (word *)mapped.pData + start;
    for ( int i = 0; i < count; i++ ) dest[ i ] = indices[ i ];

    context->Unmap( indexbuffer, 0 );
}

//-----------------------------------------------------------------------------
// Clears the table and builds the shared index buffer for quads: vertices b, b+1, b+2, b+3 become
// the triangles ( b, b+2, b+3 ) and ( b, b+3, b+1 ).

void gge_models_init()
{
    num_models = 0;
    total_vertex_memory = 0;

    for ( int i = 0; i < max_models; i++ )
    {
        models[ i ].vertexbuffer = NULL;
        models[ i ].vertex_buffer_size = 0;
        models[ i ].indexbuffer = NULL;
        models[ i ].index_buffer_size = 0;
    }

    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = standard_max_index_buffer_size * sizeof( word );
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    assert_return( d3d_device );
    HRESULT result = d3d_device->CreateBuffer( &desc, NULL, &standard_index_buffer );
    d3d_check( result );
    total_vertex_memory += desc.ByteWidth;

    int i = 0;
    word vertex = 0;
    assert_return( i < ( standard_max_index_buffer_size - 6 ) );
    while ( i < standard_max_index_buffer_size - 6 )
    {
        standard_indices[ i + 0 ] = vertex;
        standard_indices[ i + 1 ] = vertex + 2;
        standard_indices[ i + 2 ] = vertex + 3;
        standard_indices[ i + 3 ] = vertex;
        standard_indices[ i + 4 ] = vertex + 3;
        standard_indices[ i + 5 ] = vertex + 1;
        vertex += 4;
        i += 6;
    }

    copy_indices( standard_index_buffer, 0, i, standard_indices );
}

//-----------------------------------------------------------------------------

void gge_models_free()
{
    for ( dword i = 0; i < max_models; i++ )
    {
        if ( models[ i ].vertexbuffer ) gge_model_free( i );
    }

    total_vertex_memory -= standard_max_index_buffer_size * sizeof( word );
    if ( standard_index_buffer )
    {
        standard_index_buffer->Release();
        standard_index_buffer = NULL;
    }

    assert_return( total_vertex_memory == 0 );
    assert_return( num_models == 0 );
}

//-----------------------------------------------------------------------------

static bool create_vertex_buffer( int index )
{
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = models[ index ].vertex_buffer_size;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    ID3D11Device * device = gge_get_device();
    HRESULT result = device->CreateBuffer( &desc, NULL, &models[ index ].vertexbuffer );
    d3d_check( result );

    assert_return_false( models[ index ].vertexbuffer );

    total_vertex_memory += models[ index ].vertex_buffer_size;
    return true;
}

//-----------------------------------------------------------------------------

static dword get_next_model_index()
{
    dword index = 0;
    while ( index < max_models && models[ index ].vertexbuffer != NULL ) index++;
    return index;
}

//-----------------------------------------------------------------------------

dword gge_model_new( dword material_id, int max_vertices )
{
    if ( !( num_models < max_models ) ) { assert_exp( num_models < max_models ); return invalid_id; }

    dword index = get_next_model_index();
    if ( !( index < max_models ) ) { assert_exp( index < max_models ); return invalid_id; }

    models[ index ].flags = model_flags_visible;
    models[ index ].num_vertices = 0;
    models[ index ].max_vertices = max_vertices;
    models[ index ].unknown_14 = 0;
    models[ index ].material_id = material_id;

    gge_material_add_model( material_id, index );
    models[ index ].vertex_buffer_size = max_vertices * gge_material_get_vertex_size( material_id );

    bool vb_okay = create_vertex_buffer( index );
    if ( !vb_okay ) { assert_exp( vb_okay ); return invalid_id; }

    num_models++;
    return index;
}

//-----------------------------------------------------------------------------

dword gge_model_indexed_new( dword material_id, int max_vertices, int max_indicies )
{
    if ( !( num_models < max_models ) ) { assert_exp( num_models < max_models ); return invalid_id; }

    dword index = get_next_model_index();
    if ( !( index < max_models ) ) { assert_exp( index < max_models ); return invalid_id; }

    models[ index ].flags = model_flags_visible | model_flags_indexed;
    models[ index ].num_vertices = 0;
    models[ index ].max_vertices = max_vertices;
    models[ index ].unknown_14 = 0;
    models[ index ].num_indices = 0;
    models[ index ].max_indices = max_indicies;
    models[ index ].material_id = material_id;

    gge_material_add_model( material_id, index );
    models[ index ].vertex_buffer_size = max_vertices * gge_material_get_vertex_size( material_id );
    models[ index ].index_buffer_size = max_indicies * sizeof( word );

    bool vb_okay = create_vertex_buffer( index );
    if ( !vb_okay ) { assert_exp( vb_okay ); return invalid_id; }

    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = models[ index ].index_buffer_size;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    ID3D11Device * device = gge_get_device();
    HRESULT result = device->CreateBuffer( &desc, NULL, &models[ index ].indexbuffer );
    d3d_check( result );
    if ( !models[ index ].indexbuffer ) { assert_exp( models[ index ].indexbuffer ); return invalid_id; }

    total_vertex_memory += models[ index ].index_buffer_size;
    num_models++;
    return index;
}

//-----------------------------------------------------------------------------

dword gge_model_standard_indexed_new( dword material_id, int max_vertices, int max_indicies )
{
    if ( !( num_models < max_models ) ) { assert_exp( num_models < max_models ); return invalid_id; }
    if ( !( max_indicies < standard_max_index_buffer_size ) ) { assert_exp( max_indicies < standard_max_index_buffer_size ); return invalid_id; }

    dword index = get_next_model_index();
    if ( !( index < max_models ) ) { assert_exp( index < max_models ); return invalid_id; }

    models[ index ].flags = model_flags_visible | model_flags_standard_indexed;
    models[ index ].num_vertices = 0;
    models[ index ].max_vertices = max_vertices;
    models[ index ].unknown_14 = 0;
    models[ index ].num_indices = 0;
    models[ index ].max_indices = max_indicies;
    models[ index ].index_buffer_size = 0;
    models[ index ].material_id = material_id;

    gge_material_add_model( material_id, index );
    models[ index ].vertex_buffer_size = max_vertices * gge_material_get_vertex_size( material_id );

    bool vb_okay = create_vertex_buffer( index );
    if ( !vb_okay ) { assert_exp( vb_okay ); return invalid_id; }

    num_models++;
    return index;
}

//-----------------------------------------------------------------------------

void gge_model_reset_vertices( dword model_id )
{
    assert_return( model_id >= 0 && model_id < max_models );

    models[ model_id ].num_vertices = 0;
    models[ model_id ].num_indices = 0;
}

//-----------------------------------------------------------------------------

void gge_model_add_vertices( dword model_id, int num_to_add, void * vertices )
{
    assert_return( model_id >= 0 && model_id < max_models );
    assert_return( ( models[model_id].num_vertices + num_to_add ) <= models[model_id].max_vertices );
    assert_return( ( models[ model_id ].flags & ( model_flags_standard_indexed | model_flags_indexed ) ) == 0 );

    if ( num_to_add > 0 )
    {
        int start = models[ model_id ].num_vertices;
        models[ model_id ].num_vertices += num_to_add;
        gge_material_copy_vertices( models[ model_id ].material_id, models[ model_id ].vertexbuffer, start, num_to_add, vertices );
    }
}

//-----------------------------------------------------------------------------

void gge_model_add_indexed_vertices( dword model_id, int num_verts_to_add, void * vertices, int num_indices_to_add, word * indices )
{
    assert_return( model_id >= 0 && model_id < max_models );
    assert_return( ( models[ model_id ].num_vertices + num_verts_to_add ) <= models[ model_id ].max_vertices );
    assert_return( models[ model_id ].flags & model_flags_indexed );
    assert_return( ( models[ model_id ].flags & model_flags_standard_indexed ) == 0 );

    if ( num_verts_to_add > 0 )
    {
        int start = models[ model_id ].num_vertices;
        models[ model_id ].num_vertices += num_verts_to_add;
        gge_material_copy_vertices( models[ model_id ].material_id, models[ model_id ].vertexbuffer, start, num_verts_to_add, vertices );
    }

    if ( num_indices_to_add > 0 )
    {
        int start = models[ model_id ].num_indices;
        models[ model_id ].num_indices += num_indices_to_add;
        copy_indices( models[ model_id ].indexbuffer, start, num_indices_to_add, indices );
    }
}

//-----------------------------------------------------------------------------

void gge_model_add_standard_indexed_vertices( dword model_id, int num_verts_to_add, void * vertices, int num_indices_to_add )
{
    assert_return( model_id >= 0 && model_id < max_models );
    assert_return( ( models[ model_id ].num_vertices + num_verts_to_add ) <= models[ model_id ].max_vertices );
    assert_return( models[ model_id ].flags & model_flags_standard_indexed );
    assert_return( ( models[ model_id ].flags & model_flags_indexed ) == 0 );

    if ( num_verts_to_add > 0 )
    {
        int start = models[ model_id ].num_vertices;
        models[ model_id ].num_vertices += num_verts_to_add;
        gge_material_copy_vertices( models[ model_id ].material_id, models[ model_id ].vertexbuffer, start, num_verts_to_add, vertices );
    }

    models[ model_id ].num_indices += num_indices_to_add;
}

//-----------------------------------------------------------------------------
// GUESS: see the file header

bool gge_model_do_vertices_fit( dword model_id, int num_to_add )
{
    assert_return_false( model_id >= 0 && model_id < max_models );

    return ( models[ model_id ].num_vertices + num_to_add ) <= models[ model_id ].max_vertices;
}

//-----------------------------------------------------------------------------

void gge_model_free( dword model_id )
{
    assert_return( model_id >= 0 && model_id < max_models );
    assert_return( models[ model_id ].vertexbuffer );

    dword material_id = models[ model_id ].material_id;
    total_vertex_memory -= models[ model_id ].max_vertices * gge_material_get_vertex_size( material_id );
    gge_material_remove_from_model_list( material_id, model_id );

    if ( models[ model_id ].vertexbuffer )
    {
        models[ model_id ].vertexbuffer->Release();
        models[ model_id ].vertexbuffer = NULL;
    }
    if ( models[ model_id ].indexbuffer )
    {
        total_vertex_memory -= models[ model_id ].max_indices * sizeof( word );
        models[ model_id ].indexbuffer->Release();
        models[ model_id ].indexbuffer = NULL;
    }

    num_models--;
    models[ model_id ].flags = 0;
}

//-----------------------------------------------------------------------------

bool gge_model_should_render( dword model_id )
{
    assert_return_false( model_id >= 0 && model_id < max_models );

    if ( ( models[ model_id ].flags & model_flags_visible ) == 0 ) return false;
    if ( models[ model_id ].num_vertices == 0 ) return false;
    if ( ( models[ model_id ].flags & ( model_flags_indexed | model_flags_standard_indexed ) ) && models[ model_id ].num_indices == 0 ) return false;

    return true;
}

//-----------------------------------------------------------------------------

void gge_model_render( dword model_id, dword vertex_size )
{
    assert_return( model_id >= 0 && model_id < max_models );
    assert_return( models[ model_id ].vertexbuffer );
    assert_return( models[ model_id ].flags & model_flags_visible );
    assert_return( models[ model_id ].num_vertices );

    ID3D11DeviceContext * context = gge_get_device_context();

    UINT stride = vertex_size;
    UINT offset = 0;
    context->IASetVertexBuffers( 0, 1, &models[ model_id ].vertexbuffer, &stride, &offset );

    if ( models[ model_id ].flags & model_flags_indexed )
    {
        context->IASetIndexBuffer( models[ model_id ].indexbuffer, DXGI_FORMAT_R16_UINT, 0 );
        context->DrawIndexed( models[ model_id ].num_indices, 0, 0 );
    }
    else if ( models[ model_id ].flags & model_flags_standard_indexed )
    {
        context->IASetIndexBuffer( standard_index_buffer, DXGI_FORMAT_R16_UINT, 0 );
        context->DrawIndexed( models[ model_id ].num_indices, 0, 0 );
    }
    else
    {
        context->Draw( models[ model_id ].num_vertices, 0 );
    }
}

//-----------------------------------------------------------------------------

dword gge_model_get_next_id( dword model_id )
{
    if ( !( model_id >= 0 && model_id < max_models ) ) { assert_exp( model_id >= 0 && model_id < max_models ); return invalid_id; }

    return models[ model_id ].next_id;
}

//-----------------------------------------------------------------------------

void gge_model_set_next_id( dword model_id, dword next_id )
{
    assert_return( model_id >= 0 && model_id < max_models );
    assert_return( ( next_id >= 0 && next_id < max_models ) || ( next_id == invalid_id ) );

    models[ model_id ].next_id = next_id;
}
