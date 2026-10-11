//-----------------------------------------------------------------------------
// gge_materials.cpp  (set-up and bookkeeping part)
// Reconstructed from ILB.exe: shader_load 0x40DD90, gge_materials_init 0x40DF70, gge_materials_free
// 0x40E2B0 (shader_free inlined), gge_material_new 0x40E740 (get_next_material_index and
// material_set_vertex_size inlined), gge_material_free 0x40E910, gge_material_set_next_id
// 0x40EE60, gge_material_get_next_id 0x40EEC0; gge_material_get_vertex_size and
// gge_material_get_rendering_layer from their inlined copies.
//
// gge_material_set_shaders 0x40E460, gge_material_render 0x40E560, gge_material_copy_vertices
// 0x40E9B0, gge_material_set_global_data_shader_vars 0x40EA30, gge_material_set_shader_vars 0x40EB60,
// gge_material_set_shader_texture_resource_view 0x40ECA0, gge_material_set_shader_sampler 0x40EDD0;
// gge_material_set_alpha_blend from its copies inlined in the material types.
//
// Guesses in the drawing code, to be confirmed when the callers are reconstructed:
//   - the meaning of the third parameter of gge_material_render / set_shader_vars (called
//     use_view_matrix here: 0 swaps the view's first matrix for a fixed one)
//   - the parameter names of gge_material_set_global_data_shader_vars, and the meaning of the
//     values it sends (gamma and window size are clear, the config.unknown_5x pair is not)
//
// Not reconstructed yet (same file): gge_material_set_shader_blend_textures_resource_view (only
// exists inlined in 0x40CD40).

#include "main.h"
#include "ec_fileio.h"
#include "gge_main.h"
#include "gge_textures.h"
#include "gge_materials.h"
#include "gge_models.h"
#include "gge_view.h"
#include "ec_config.h"
#include "pending.h"

struct material_data        // 24 bytes
{
    dword       material_type;      // invalid_id = unused
    dword       texture_id;
    dword       model_id_list;
    int         rendering_layer;
    dword       vertex_size;
    dword       next_id;            // next material using the same texture
};

static int num_materials = 0;                           // 0x383C758
static material_data materials[ max_materials ];        // 0x383C778

struct shader_data          // 8 bytes
{
    ID3D11VertexShader *    vertex_shader;
    ID3D11PixelShader *     pixel_shader;
};

// Variable names below are chosen from how the objects are created; only shaders is original.
static char pixel_shader_filename[ 1024 ];              // 0x383BF30
static char vertex_shader_filename[ 1024 ];             // 0x383C350
static ID3D11Buffer * global_constant_buffer = NULL;    // 0x383C750, 128 bytes
static ID3D11Buffer * material_constant_buffer = NULL;  // 0x383C754, 32 bytes
static ID3D11SamplerState * sampler_linear_wrap = NULL; // 0x383C75C
static ID3D11BlendState * blend_alpha = NULL;           // 0x383C770
static ID3D11SamplerState * sampler_point_clamp = NULL; // 0x383C774
static shader_data shaders[ num_material_types ];       // 0x383DF78
static ID3D11BlendState * blend_none = NULL;            // 0x383DFE8
static ID3D11SamplerState * sampler_point_wrap = NULL;  // 0x383DFEC

// contents of the two constant buffers
struct global_shader_constants      // 128 bytes, buffer slot 0
{
    matrix      projection_matrix;
    matrix      view_matrix;
};
struct material_shader_constants    // 32 bytes, buffer slot 1
{
    float       gamma_offset;       // ( gamma - 1 ) / 2
    float       gamma;
    float       unknown_8;          // config.unknown_54
    float       unknown_C;          // config.unknown_50
    float       width;              // window size in pixels, times scale
    float       height;
    float       unknown_18;         // 1 or 0
    float       unused_1C;
};
static global_shader_constants global_constants;        // 0x383BEB0
static material_shader_constants material_constants;    // 0x383C330
static float blend_factor[ 4 ];                         // 0x383C760, never written: all zero

//-----------------------------------------------------------------------------
// loads <name>.vso and <name>.pso from the pack file for one material type

static void shader_load( dword index )
{
    assert_return( index < num_material_types );
    assert_return( material_type_tbl[ index ].material_shader_filename );

    const char * name = material_type_tbl[ index ].material_shader_filename();
    ID3D11Device * device = gge_get_device();
    dword size;

    strcpy( vertex_shader_filename, name );
    strcat( vertex_shader_filename, ".vso" );
    byte * data = pack_file_load( vertex_shader_filename, &size );
    HRESULT result = device->CreateVertexShader( data, size, NULL, &shaders[ index ].vertex_shader );
    d3d_check( result );
    if ( material_type_tbl[ index ].shader_created ) material_type_tbl[ index ].shader_created( data, size );
    pack_file_close( data );

    strcpy( pixel_shader_filename, name );
    strcat( pixel_shader_filename, ".pso" );
    data = pack_file_load( pixel_shader_filename, &size );
    result = device->CreatePixelShader( data, size, NULL, &shaders[ index ].pixel_shader );
    d3d_check( result );
    pack_file_close( data );
}

//-----------------------------------------------------------------------------

static void shader_free( int index )
{
    assert_return( index >= 0 );
    assert_return( index < max_materials );     // sic: the binary checks against max_materials here

    if ( material_type_tbl[ index ].shader_free ) material_type_tbl[ index ].shader_free();

    if ( shaders[ index ].vertex_shader )
    {
        shaders[ index ].vertex_shader->Release();
        shaders[ index ].vertex_shader = NULL;
    }
    if ( shaders[ index ].pixel_shader )
    {
        shaders[ index ].pixel_shader->Release();
        shaders[ index ].pixel_shader = NULL;
    }
}

//-----------------------------------------------------------------------------
// Note that unused entries keep texture_id 0, not invalid_id: only type, model list and next id
// are set after the entry is zeroed.

void gge_materials_init()
{
    assert_return( num_materials == 0 );

    for ( int i = 0; i < max_materials; i++ )
    {
        memset( &materials[ i ], 0, sizeof( material_data ) );
        materials[ i ].material_type = invalid_id;
        materials[ i ].model_id_list = invalid_id;
        materials[ i ].next_id = invalid_id;
    }

    for ( dword i = 0; i < num_material_types; i++ ) shader_load( i );

    ID3D11Device * device = gge_get_device();

    D3D11_BUFFER_DESC buffer_desc;
    buffer_desc.ByteWidth = 128;
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;
    buffer_desc.StructureByteStride = 0;
    HRESULT result = device->CreateBuffer( &buffer_desc, NULL, &global_constant_buffer );
    d3d_check( result );
    buffer_desc.ByteWidth = 32;
    result = device->CreateBuffer( &buffer_desc, NULL, &material_constant_buffer );
    d3d_check( result );

    D3D11_BLEND_DESC blend_desc;
    memset( &blend_desc, 0, sizeof( blend_desc ) );
    blend_desc.RenderTarget[ 0 ].BlendEnable = TRUE;
    blend_desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[ 0 ].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[ 0 ].DestBlendAlpha = D3D11_BLEND_ZERO;
    blend_desc.RenderTarget[ 0 ].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[ 0 ].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    result = device->CreateBlendState( &blend_desc, &blend_alpha );
    d3d_check( result );

    memset( &blend_desc, 0, sizeof( blend_desc ) );
    blend_desc.RenderTarget[ 0 ].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    result = device->CreateBlendState( &blend_desc, &blend_none );
    d3d_check( result );

    D3D11_SAMPLER_DESC sampler_desc;
    memset( &sampler_desc, 0, sizeof( sampler_desc ) );
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    device->CreateSamplerState( &sampler_desc, &sampler_point_clamp );

    // the next two each change one field and keep the rest: only U wraps
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    device->CreateSamplerState( &sampler_desc, &sampler_point_wrap );

    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    device->CreateSamplerState( &sampler_desc, &sampler_linear_wrap );
}

//-----------------------------------------------------------------------------

void gge_materials_free()
{
    for ( dword i = 0; i < max_materials; i++ )
    {
        if ( materials[ i ].material_type != invalid_id ) gge_material_free( i );
    }

    assert_return( num_materials == 0 );

    for ( int i = 0; i < num_material_types; i++ ) shader_free( i );

    if ( global_constant_buffer ) { global_constant_buffer->Release(); global_constant_buffer = NULL; }
    if ( material_constant_buffer ) { material_constant_buffer->Release(); material_constant_buffer = NULL; }
    if ( blend_alpha ) { blend_alpha->Release(); blend_alpha = NULL; }
    if ( blend_none ) { blend_none->Release(); blend_none = NULL; }
    if ( sampler_point_clamp ) { sampler_point_clamp->Release(); sampler_point_clamp = NULL; }
    if ( sampler_point_wrap ) { sampler_point_wrap->Release(); sampler_point_wrap = NULL; }
    if ( sampler_linear_wrap ) { sampler_linear_wrap->Release(); sampler_linear_wrap = NULL; }
}

//-----------------------------------------------------------------------------

static dword get_next_material_index()
{
    dword index = 0;
    while ( index < max_materials && materials[ index ].material_type != invalid_id ) index++;

    if ( !( index < max_materials ) ) { assert_exp( index < max_materials ); return invalid_id; }
    return index;
}

//-----------------------------------------------------------------------------

static void material_set_vertex_size( dword material_id )
{
    dword mt = materials[ material_id ].material_type;
    assert_return( material_type_tbl[mt].vertex_size );

    materials[ material_id ].vertex_size = material_type_tbl[ mt ].vertex_size();
}

//-----------------------------------------------------------------------------

dword gge_material_new( int material_type, dword texture_id, int rendering_layer )
{
    if ( !( material_type >= 0 && material_type < num_material_types ) ) { assert_exp( material_type >= 0 && material_type < num_material_types ); return invalid_id; }
    if ( !( num_materials < max_materials ) ) { assert_exp( num_materials < max_materials ); return invalid_id; }

    dword index = get_next_material_index();
    if ( !( index != invalid_id ) ) { assert_exp( index != invalid_id ); return invalid_id; }

    materials[ index ].rendering_layer = rendering_layer;
    materials[ index ].material_type = material_type;
    materials[ index ].model_id_list = invalid_id;
    materials[ index ].texture_id = texture_id;
    materials[ index ].next_id = invalid_id;
    material_set_vertex_size( index );

    gge_texture_add_material( texture_id, index );

    num_materials++;
    return index;
}

//-----------------------------------------------------------------------------
// The model list is checked after the material has already been unlinked and marked unused, as in
// the binary: freeing a material that models still use leaves it half freed and still counted.

void gge_material_free( dword material_id )
{
    assert_return( material_id < max_materials );

    gge_texture_remove_material( materials[ material_id ].texture_id, material_id );
    materials[ material_id ].texture_id = invalid_id;
    materials[ material_id ].material_type = invalid_id;

    assert_return( materials[ material_id ].model_id_list == invalid_id );

    num_materials--;
    materials[ material_id ].model_id_list = invalid_id;
}

//-----------------------------------------------------------------------------

void gge_material_set_next_id( dword material_id, dword next_id )
{
    assert_return( material_id >= 0 && material_id < max_materials );
    assert_return( ( next_id >= 0 && next_id < max_materials ) || ( next_id == invalid_id ) );

    materials[ material_id ].next_id = next_id;
}

//-----------------------------------------------------------------------------

dword gge_material_get_next_id( dword material_id )
{
    if ( !( material_id >= 0 && material_id < max_materials ) ) { assert_exp( material_id >= 0 && material_id < max_materials ); return invalid_id; }
    if ( !( materials[ material_id ].material_type != invalid_id ) ) { assert_exp( materials[ material_id ].material_type != invalid_id ); return invalid_id; }

    return materials[ material_id ].next_id;
}

//-----------------------------------------------------------------------------
// returns invalid_id for a bad material (callers then compute sizes with -1, as the binary does)

dword gge_material_get_vertex_size( dword material_id )
{
    if ( !( material_id < max_materials ) ) { assert_exp( material_id < max_materials ); return invalid_id; }

    return materials[ material_id ].vertex_size;
}

//-----------------------------------------------------------------------------

int gge_material_get_rendering_layer( dword material_id )
{
    if ( !( material_id >= 0 && material_id < max_materials ) ) { assert_exp( material_id >= 0 && material_id < max_materials ); return -1; }
    if ( !( materials[ material_id ].material_type != invalid_id ) ) { assert_exp( materials[ material_id ].material_type != invalid_id ); return -1; }

    return materials[ material_id ].rendering_layer;
}

//-----------------------------------------------------------------------------
// puts a model at the head of the material's model list

void gge_material_add_model( dword material_id, dword model_id )
{
    assert_return( material_id < max_materials );

    gge_model_set_next_id( model_id, materials[ material_id ].model_id_list );
    materials[ material_id ].model_id_list = model_id;
}

//-----------------------------------------------------------------------------

void gge_material_remove_from_model_list( dword material_id, dword model_id )
{
    assert_return( material_id < max_materials );
    assert_return( materials[ material_id ].model_id_list != invalid_id );

    dword previous = invalid_id;
    dword list = materials[ material_id ].model_id_list;
    while ( list != model_id )
    {
        previous = list;
        list = gge_model_get_next_id( list );
        assert_return( list != invalid_id );
    }

    dword next_id = gge_model_get_next_id( model_id );
    if ( previous == invalid_id ) materials[ material_id ].model_id_list = next_id;
    else gge_model_set_next_id( previous, next_id );
}

//-----------------------------------------------------------------------------
// hands vertices to the material type, which knows the vertex layout

void gge_material_copy_vertices( dword material_id, ID3D11Buffer * vertexbuffer, int start, int count, void * list )
{
    assert_return( material_id < max_materials );

    dword mt = materials[ material_id ].material_type;
    assert_return( ( mt >= 0 ) && ( mt < num_material_types ) );
    assert_return( material_type_tbl[ mt ].vertex_copy );

    material_type_tbl[ mt ].vertex_copy( vertexbuffer, start, count, list );
}

//-----------------------------------------------------------------------------
// binds a material type's two shaders and both constant buffers

bool gge_material_set_shaders( dword material_type )
{
    assert_return_false( material_type < num_material_types );
    assert_return_false( shaders[ material_type ].vertex_shader );
    assert_return_false( shaders[ material_type ].pixel_shader );

    ID3D11DeviceContext * context = gge_get_device_context();
    assert_return_false( context );

    context->VSSetShader( shaders[ material_type ].vertex_shader, NULL, 0 );
    context->VSSetConstantBuffers( 0, 1, &global_constant_buffer );
    context->VSSetConstantBuffers( 1, 1, &material_constant_buffer );
    context->PSSetShader( shaders[ material_type ].pixel_shader, NULL, 0 );
    context->PSSetConstantBuffers( 0, 1, &global_constant_buffer );
    context->PSSetConstantBuffers( 1, 1, &material_constant_buffer );

    return true;
}

//-----------------------------------------------------------------------------
// draws every model of a material that has something to draw

void gge_material_render( dword view_id, dword material_id, int use_view_matrix )
{
    assert_return( material_id < max_materials );
    assert_return( materials[ material_id ].material_type != invalid_id );

    gge_get_device_context();

    dword mt = materials[ material_id ].material_type;
    if ( mt == invalid_id || materials[ material_id ].model_id_list == invalid_id ) return;

    bool set_ok = gge_material_set_shaders( mt );
    assert_return( set_ok );
    assert_return( material_type_tbl[ mt ].fx_set_globals );

    material_type_tbl[ mt ].fx_set_globals( view_id, material_id, use_view_matrix );

    for ( dword model_id = materials[ material_id ].model_id_list; model_id != invalid_id; model_id = gge_model_get_next_id( model_id ) )
    {
        if ( gge_model_should_render( model_id ) ) gge_model_render( model_id, materials[ material_id ].vertex_size );
    }
}

//-----------------------------------------------------------------------------

void gge_material_set_global_data_shader_vars( int use_view_matrix, bool unknown_flag, float scale )
{
    ID3D11DeviceContext * context = gge_get_device_context();
    assert_return( context );

    material_constants.gamma = config.gamma;
    material_constants.gamma_offset = ( config.gamma - 1.0f ) * 0.5f;
    material_constants.unknown_C = config.unknown_50;
    material_constants.unknown_8 = config.unknown_54;

    int width = ( use_view_matrix != 0 && game_globals.unknown_2044 == 2 ) ? window_width / 2 : window_width;
    material_constants.unknown_18 = 1.0f;
    material_constants.width = (float)width * scale;
    material_constants.height = (float)window_height * scale;
    if ( !unknown_flag ) material_constants.unknown_18 = 0.0f;

    context->UpdateSubresource( material_constant_buffer, 0, NULL, &material_constants, 0, 0 );
}

//-----------------------------------------------------------------------------
// sends the view's matrices to the shaders

void gge_material_set_shader_vars( dword view_id, dword material_id, int use_view_matrix )
{
    ID3D11DeviceContext * context = gge_get_device_context();
    assert_return( context );

    global_constants.projection_matrix = views[ view_id ].projection_matrix;
    if ( use_view_matrix != 0 ) global_constants.view_matrix = views[ view_id ].view_matrix;
    else global_constants.view_matrix = screen_matrix;

    context->UpdateSubresource( global_constant_buffer, 0, NULL, &global_constants, 0, 0 );

    gge_material_set_global_data_shader_vars( use_view_matrix, true, 1.0f );
}

//-----------------------------------------------------------------------------

void gge_material_set_shader_texture_resource_view( ID3D11ShaderResourceView * rv )
{
    assert_return( rv );

    ID3D11DeviceContext * context = gge_get_device_context();
    assert_return( context );

    context->PSSetShaderResources( 0, 1, &rv );
}

//-----------------------------------------------------------------------------
// binds the material's texture and one of the three samplers

void gge_material_set_shader_sampler( dword material_id, int sampler )
{
    gge_material_set_shader_texture_resource_view( gge_texture_get_texture_rv( materials[ material_id ].texture_id ) );

    ID3D11DeviceContext * context = gge_get_device_context();
    assert_return( context );

    if ( sampler == material_sampler_point_clamp ) context->PSSetSamplers( 0, 1, &sampler_point_clamp );
    else if ( sampler == material_sampler_point_wrap ) context->PSSetSamplers( 0, 1, &sampler_point_wrap );
    else if ( sampler == material_sampler_linear_wrap ) context->PSSetSamplers( 0, 1, &sampler_linear_wrap );
}

//-----------------------------------------------------------------------------

void gge_material_set_alpha_blend( bool alpha_blend )
{
    ID3D11DeviceContext * context = gge_get_device_context();
    assert_return( context );

    context->OMSetBlendState( alpha_blend ? blend_alpha : blend_none, blend_factor, 0xffffffff );
}
