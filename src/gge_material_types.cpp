//-----------------------------------------------------------------------------
// gge_material_types.cpp
//
// Not an original file name: material_type_tbl (0x5D42C8) most likely lives in gge_materials.cpp.
// It is kept apart so that gge_materials.cpp can be built and tested without all 14 type files.

#include "main.h"
#include "gge_main.h"
#include "gge_materials.h"
#include "gge_material_types.h"

#define material_type_row( name ) \
    { gge_mat_##name##_shader_filename, gge_mat_##name##_shader_created, gge_mat_##name##_shader_free, \
      gge_mat_##name##_set_globals, gge_mat_##name##_vertex_size, gge_mat_##name##_copy_vertices }

material_type_data material_type_tbl[ num_material_types ] =
{
    material_type_row( plain ),
    material_type_row( block ),
    material_type_row( overlay ),
    material_type_row( sprite ),
    material_type_row( alpha_sprite ),
    material_type_row( parallax ),
    material_type_row( ui ),
    material_type_row( alpha_ui ),
    material_type_row( alpha_sprite_ui ),
    material_type_row( title_ui ),
    material_type_row( ui_linear ),
    material_type_row( map ),
    material_type_row( blur ),
    material_type_row( bloom_blend ),
};
