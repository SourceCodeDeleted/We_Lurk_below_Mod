//-----------------------------------------------------------------------------
// gge_material_types.h
//
// Not an original file name. The 14 material types (one gge_mat_*.cpp each) and the functions each
// contributes to material_type_tbl. The material_type_* names are chosen from the shader each type
// loads; the binary only has the numbers.

#ifndef _GGE_MATERIAL_TYPES_H_
#define _GGE_MATERIAL_TYPES_H_

enum
{
    material_type_plain,
    material_type_block,
    material_type_overlay,
    material_type_sprite,
    material_type_alpha_sprite,
    material_type_parallax,
    material_type_ui,
    material_type_alpha_ui,
    material_type_alpha_sprite_ui,
    material_type_title_ui,
    material_type_ui_linear,
    material_type_map,
    material_type_blur,
    material_type_bloom_blend,
};

const char * gge_mat_plain_shader_filename();
void gge_mat_plain_shader_created( void * vso_data, dword vso_size );
void gge_mat_plain_shader_free();
void gge_mat_plain_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_plain_vertex_size();
void gge_mat_plain_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_block_shader_filename();
void gge_mat_block_shader_created( void * vso_data, dword vso_size );
void gge_mat_block_shader_free();
void gge_mat_block_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_block_vertex_size();
void gge_mat_block_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_overlay_shader_filename();
void gge_mat_overlay_shader_created( void * vso_data, dword vso_size );
void gge_mat_overlay_shader_free();
void gge_mat_overlay_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_overlay_vertex_size();
void gge_mat_overlay_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_sprite_shader_filename();
void gge_mat_sprite_shader_created( void * vso_data, dword vso_size );
void gge_mat_sprite_shader_free();
void gge_mat_sprite_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_sprite_vertex_size();
void gge_mat_sprite_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_alpha_sprite_shader_filename();
void gge_mat_alpha_sprite_shader_created( void * vso_data, dword vso_size );
void gge_mat_alpha_sprite_shader_free();
void gge_mat_alpha_sprite_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_alpha_sprite_vertex_size();
void gge_mat_alpha_sprite_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_parallax_shader_filename();
void gge_mat_parallax_shader_created( void * vso_data, dword vso_size );
void gge_mat_parallax_shader_free();
void gge_mat_parallax_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_parallax_vertex_size();
void gge_mat_parallax_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_ui_shader_filename();
void gge_mat_ui_shader_created( void * vso_data, dword vso_size );
void gge_mat_ui_shader_free();
void gge_mat_ui_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_ui_vertex_size();
void gge_mat_ui_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_alpha_ui_shader_filename();
void gge_mat_alpha_ui_shader_created( void * vso_data, dword vso_size );
void gge_mat_alpha_ui_shader_free();
void gge_mat_alpha_ui_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_alpha_ui_vertex_size();
void gge_mat_alpha_ui_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_alpha_sprite_ui_shader_filename();
void gge_mat_alpha_sprite_ui_shader_created( void * vso_data, dword vso_size );
void gge_mat_alpha_sprite_ui_shader_free();
void gge_mat_alpha_sprite_ui_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_alpha_sprite_ui_vertex_size();
void gge_mat_alpha_sprite_ui_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_title_ui_shader_filename();
void gge_mat_title_ui_shader_created( void * vso_data, dword vso_size );
void gge_mat_title_ui_shader_free();
void gge_mat_title_ui_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_title_ui_vertex_size();
void gge_mat_title_ui_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_ui_linear_shader_filename();
void gge_mat_ui_linear_shader_created( void * vso_data, dword vso_size );
void gge_mat_ui_linear_shader_free();
void gge_mat_ui_linear_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_ui_linear_vertex_size();
void gge_mat_ui_linear_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_map_shader_filename();
void gge_mat_map_shader_created( void * vso_data, dword vso_size );
void gge_mat_map_shader_free();
void gge_mat_map_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_map_vertex_size();
void gge_mat_map_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_blur_shader_filename();
void gge_mat_blur_shader_created( void * vso_data, dword vso_size );
void gge_mat_blur_shader_free();
void gge_mat_blur_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_blur_vertex_size();
void gge_mat_blur_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

const char * gge_mat_bloom_blend_shader_filename();
void gge_mat_bloom_blend_shader_created( void * vso_data, dword vso_size );
void gge_mat_bloom_blend_shader_free();
void gge_mat_bloom_blend_set_globals( dword view_id, dword material_id, int use_view_matrix );
int gge_mat_bloom_blend_vertex_size();
void gge_mat_bloom_blend_copy_vertices( ID3D11Buffer * vertexbuffer, int start, int count, void * list );

#endif
