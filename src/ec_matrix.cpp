//-----------------------------------------------------------------------------
// ec_matrix.cpp
// Reconstructed from ILB.exe: matrix_lookat_lh 0x406DE0, matrix_perspective_fov_lh 0x4070F0,
// matrix_multiply 0x407250, matrix_inverse 0x407650, matrix_unproject 0x407C10.
//
// matrix_inverse is written as the same cofactor expansion the binary uses, but not in the
// binary's exact order of floating-point operations, so results can differ in the last bits.

#include "main.h"
#include "ec_matrix.h"
#include <math.h>

//-----------------------------------------------------------------------------
// leaves a vector that is already unit length alone; a zero vector becomes `fallback`

static void normalize_or( vector3d * v, float fallback_x, float fallback_y, float fallback_z )
{
    float length_squared = v->x * v->x + v->y * v->y + v->z * v->z;
    if ( length_squared >= 0.9999f && length_squared <= 1.0001f ) return;

    float length = (float)sqrt( length_squared );
    if ( length <= 0.0001f )
    {
        v->x = fallback_x;
        v->y = fallback_y;
        v->z = fallback_z;
    }
    else
    {
        v->x /= length;
        v->y /= length;
        v->z /= length;
    }
}

//-----------------------------------------------------------------------------

bool matrix_lookat_lh( matrix * result, vector3d & eye, vector3d & at, vector3d & up )
{
    assert_return_false( result );

    vector3d z_axis = { at.x - eye.x, at.y - eye.y, at.z - eye.z };
    normalize_or( &z_axis, 0.0f, 0.0f, 1.0f );

    vector3d x_axis = { up.y * z_axis.z - up.z * z_axis.y, up.z * z_axis.x - up.x * z_axis.z, up.x * z_axis.y - up.y * z_axis.x };
    normalize_or( &x_axis, 0.0f, 0.0f, 1.0f );

    vector3d y_axis = { z_axis.y * x_axis.z - z_axis.z * x_axis.y, z_axis.z * x_axis.x - z_axis.x * x_axis.z, z_axis.x * x_axis.y - z_axis.y * x_axis.x };

    result->m[ 0 ][ 0 ] = x_axis.x;  result->m[ 0 ][ 1 ] = y_axis.x;  result->m[ 0 ][ 2 ] = z_axis.x;  result->m[ 0 ][ 3 ] = 0.0f;
    result->m[ 1 ][ 0 ] = x_axis.y;  result->m[ 1 ][ 1 ] = y_axis.y;  result->m[ 1 ][ 2 ] = z_axis.y;  result->m[ 1 ][ 3 ] = 0.0f;
    result->m[ 2 ][ 0 ] = x_axis.z;  result->m[ 2 ][ 1 ] = y_axis.z;  result->m[ 2 ][ 2 ] = z_axis.z;  result->m[ 2 ][ 3 ] = 0.0f;
    result->m[ 3 ][ 0 ] = -( x_axis.x * eye.x + x_axis.y * eye.y + x_axis.z * eye.z );
    result->m[ 3 ][ 1 ] = -( y_axis.x * eye.x + y_axis.y * eye.y + y_axis.z * eye.z );
    result->m[ 3 ][ 2 ] = -( z_axis.x * eye.x + z_axis.y * eye.y + z_axis.z * eye.z );
    result->m[ 3 ][ 3 ] = 1.0f;

    return true;
}

//-----------------------------------------------------------------------------

bool matrix_perspective_fov_lh( matrix * result, float fov, float width, float height, float z_near, float z_far )
{
    assert_return_false( result );
    assert_return_false( width > 0 );
    assert_return_false( height > 0 );
    assert_return_false( fov > 0.0f );

    float y_scale = (float)cos( fov * 0.5f ) / (float)sin( fov * 0.5f );

    memset( result, 0, sizeof( matrix ) );
    result->m[ 0 ][ 0 ] = ( height / width ) * y_scale;
    result->m[ 1 ][ 1 ] = y_scale;
    result->m[ 2 ][ 2 ] = z_far / ( z_far - z_near );
    result->m[ 2 ][ 3 ] = 1.0f;
    result->m[ 3 ][ 2 ] = -( z_near * z_far ) / ( z_far - z_near );

    return true;
}

//-----------------------------------------------------------------------------
// result = b * a. result may not be a or b.

void matrix_multiply( matrix * result, matrix * a, matrix * b )
{
    for ( int row = 0; row < 4; row++ )
    {
        for ( int column = 0; column < 4; column++ )
        {
            result->m[ row ][ column ] =
                b->m[ row ][ 0 ] * a->m[ 0 ][ column ] + b->m[ row ][ 1 ] * a->m[ 1 ][ column ] +
                b->m[ row ][ 2 ] * a->m[ 2 ][ column ] + b->m[ row ][ 3 ] * a->m[ 3 ][ column ];
        }
    }
}

//-----------------------------------------------------------------------------
// Cofactor inverse. As in the binary there is no check for a zero determinant, and it always
// returns true.

bool matrix_inverse( matrix * result, matrix * source )
{
    const float * s = &source->m[ 0 ][ 0 ];
    float cofactor[ 16 ];

    for ( int row = 0; row < 4; row++ )
    {
        for ( int column = 0; column < 4; column++ )
        {
            // 3x3 minor without this row and column
            float minor[ 9 ];
            int n = 0;
            for ( int r = 0; r < 4; r++ )
            {
                if ( r == row ) continue;
                for ( int c = 0; c < 4; c++ )
                {
                    if ( c == column ) continue;
                    minor[ n++ ] = s[ r * 4 + c ];
                }
            }
            float determinant =
                minor[ 0 ] * ( minor[ 4 ] * minor[ 8 ] - minor[ 5 ] * minor[ 7 ] ) -
                minor[ 1 ] * ( minor[ 3 ] * minor[ 8 ] - minor[ 5 ] * minor[ 6 ] ) +
                minor[ 2 ] * ( minor[ 3 ] * minor[ 7 ] - minor[ 4 ] * minor[ 6 ] );
            cofactor[ row * 4 + column ] = ( ( row + column ) & 1 ) ? -determinant : determinant;
        }
    }

    float inverse_determinant = 1.0f / ( s[ 0 ] * cofactor[ 0 ] + s[ 1 ] * cofactor[ 1 ] + s[ 2 ] * cofactor[ 2 ] + s[ 3 ] * cofactor[ 3 ] );

    // the inverse is the transposed cofactor matrix over the determinant
    for ( int row = 0; row < 4; row++ )
        for ( int column = 0; column < 4; column++ )
            result->m[ row ][ column ] = cofactor[ column * 4 + row ] * inverse_determinant;

    return true;
}

//-----------------------------------------------------------------------------
// Screen position (pixels, z as depth 0..1) back to world space.

bool matrix_unproject( vector3d * result, vector3d * screen, matrix * view, matrix * projection )
{
    matrix combined, inverse;
    matrix_multiply( &combined, projection, view );     // view * projection
    matrix_inverse( &inverse, &combined );

    float x = ( ( screen->x - view_port.x ) / view_port.width ) * 2.0f - 1.0f;
    float y = ( 1.0f - ( screen->y - view_port.y ) / view_port.height ) * 2.0f - 1.0f;
    float z = screen->z;

    float w = x * inverse.m[ 0 ][ 3 ] + y * inverse.m[ 1 ][ 3 ] + z * inverse.m[ 2 ][ 3 ] + inverse.m[ 3 ][ 3 ];
    result->x = ( x * inverse.m[ 0 ][ 0 ] + y * inverse.m[ 1 ][ 0 ] + z * inverse.m[ 2 ][ 0 ] + inverse.m[ 3 ][ 0 ] ) / w;
    result->y = ( x * inverse.m[ 0 ][ 1 ] + y * inverse.m[ 1 ][ 1 ] + z * inverse.m[ 2 ][ 1 ] + inverse.m[ 3 ][ 1 ] ) / w;
    result->z = ( x * inverse.m[ 0 ][ 2 ] + y * inverse.m[ 1 ][ 2 ] + z * inverse.m[ 2 ][ 2 ] + inverse.m[ 3 ][ 2 ] ) / w;

    return true;
}
