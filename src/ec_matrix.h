//-----------------------------------------------------------------------------
// ec_matrix.h
//
// 4x4 matrices, row-major, row-vector convention (Direct3D style): v' = v * M.
// matrix_lookat_lh and matrix_perspective_fov_lh are original names; matrix_multiply,
// matrix_inverse and matrix_unproject are chosen here (their functions carry no asserts).

#ifndef _EC_MATRIX_H_
#define _EC_MATRIX_H_

#include "ec_vector.h"

struct matrix
{
    float       m[ 4 ][ 4 ];
};

// window area used by matrix_unproject (0x651EC8); owned by the view code, name chosen
struct view_port_data { float x, y, width, height; };
extern view_port_data view_port;

bool matrix_lookat_lh( matrix * result, vector3d & eye, vector3d & at, vector3d & up );
bool matrix_perspective_fov_lh( matrix * result, float fov, float width, float height, float z_near, float z_far );
void matrix_multiply( matrix * result, matrix * a, matrix * b );
bool matrix_inverse( matrix * result, matrix * source );
bool matrix_unproject( vector3d * result, vector3d * screen, matrix * view, matrix * projection );

#endif
