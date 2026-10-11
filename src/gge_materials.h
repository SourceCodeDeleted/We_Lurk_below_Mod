//-----------------------------------------------------------------------------
// gge_materials.h
//
// Materials: a shader type plus the texture it draws with. Up to 256. Materials using the same
// texture are chained through next_id from the texture's material_id_list (materials with no
// texture hang off a separate list), and every material heads a list of the models that use it.
//
// Function names and the names in assert text (materials, material_type_tbl, num_material_types,
// max_materials, num_materials, model_id_list, vertex_size, vertex_copy, fx_set_globals,
// material_shader_filename, shaders) are original. The parameter name rendering_layer comes from
// gge_material_get_rendering_layer returning that field.

#ifndef _GGE_MATERIALS_H_
#define _GGE_MATERIALS_H_

struct ID3D11Buffer;
struct ID3D11ShaderResourceView;

// second argument of gge_material_set_shader_sampler; the names are chosen here
enum
{
    material_sampler_point_clamp = 0,
    material_sampler_point_wrap,        // only U wraps
    material_sampler_linear_wrap,       // only U wraps
};

#define max_materials           256
#define num_material_types      14

// One entry per shader type (gge_mat_*.cpp); table of 14 x 24 bytes at 0x5D42C8. Member names quoted
// in asserts are original (material_shader_filename, fx_set_globals, vertex_size, vertex_copy);
// shader_created and shader_free are chosen here. fx_set_globals' parameter names are guesses.
// The table itself is defined in gge_material_types.cpp.
struct material_type_data
{
    const char * ( *    material_shader_filename )();               // path without extension, e.g. shaders\dx11\alpha_ui
    void ( *            shader_created )( void * vso_data, dword vso_size );    // builds the type's input layout
    void ( *            shader_free )();
    void ( *            fx_set_globals )( dword view_id, dword material_id, int use_view_matrix );
    int ( *             vertex_size )();
    void ( *            vertex_copy )( ID3D11Buffer * vertexbuffer, int start, int count, void * list );
};
extern material_type_data material_type_tbl[ num_material_types ];

void gge_materials_init();
void gge_materials_free();
dword gge_material_new( int material_type, dword texture_id, int rendering_layer );
void gge_material_free( dword material_id );
void gge_material_set_next_id( dword material_id, dword next_id );
dword gge_material_get_next_id( dword material_id );
dword gge_material_get_vertex_size( dword material_id );
int gge_material_get_rendering_layer( dword material_id );
void gge_material_add_model( dword material_id, dword model_id );
void gge_material_remove_from_model_list( dword material_id, dword model_id );
bool gge_material_set_shaders( dword material_type );
void gge_material_render( dword view_id, dword material_id, int use_view_matrix );
void gge_material_set_global_data_shader_vars( int use_view_matrix, bool unknown_flag, float scale );
void gge_material_set_shader_vars( dword view_id, dword material_id, int use_view_matrix );
void gge_material_set_shader_texture_resource_view( ID3D11ShaderResourceView * rv );
void gge_material_set_shader_sampler( dword material_id, int sampler );
void gge_material_set_alpha_blend( bool alpha_blend );
void gge_material_copy_vertices( dword material_id, ID3D11Buffer * vertexbuffer, int start, int count, void * list );

#endif
