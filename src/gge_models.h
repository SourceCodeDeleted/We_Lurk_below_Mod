//-----------------------------------------------------------------------------
// gge_models.h
//
// Models: a dynamic vertex buffer (and optionally an index buffer) drawn with one material.
// Up to 1024. "Standard indexed" models are lists of quads that share one prebuilt index buffer.
//
// Function names are original, as are the names in assert text (models, max_models, num_models,
// total_vertex_memory, standard_max_index_buffer_size, vertexbuffer, indexbuffer, num_vertices,
// max_vertices, model_flags_visible / _indexed / _standard_indexed, max_indicies, vb_okay).

#ifndef _GGE_MODELS_H_
#define _GGE_MODELS_H_

#define max_models                          1024
#define standard_max_index_buffer_size      0x4000      // in indices (words)

#define model_flags_visible                 0x0001
#define model_flags_indexed                 0x0004
#define model_flags_standard_indexed        0x0008

void gge_models_init();
void gge_models_free();
dword gge_model_new( dword material_id, int max_vertices );
dword gge_model_indexed_new( dword material_id, int max_vertices, int max_indicies );
dword gge_model_standard_indexed_new( dword material_id, int max_vertices, int max_indicies );
void gge_model_reset_vertices( dword model_id );
void gge_model_add_vertices( dword model_id, int num_to_add, void * vertices );
void gge_model_add_indexed_vertices( dword model_id, int num_verts_to_add, void * vertices, int num_indices_to_add, word * indices );
void gge_model_add_standard_indexed_vertices( dword model_id, int num_verts_to_add, void * vertices, int num_indices_to_add );
bool gge_model_do_vertices_fit( dword model_id, int num_to_add );
void gge_model_free( dword model_id );
bool gge_model_should_render( dword model_id );
void gge_model_render( dword model_id, dword vertex_size );
dword gge_model_get_next_id( dword model_id );
void gge_model_set_next_id( dword model_id, dword next_id );

#endif
