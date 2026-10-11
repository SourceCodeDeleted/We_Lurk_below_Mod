// Native test of src/ec_matrix.cpp, src/ec_spline.cpp and src/ec_random.cpp.
#include "main.h"
#include "ec_matrix.h"
#include "ec_spline.h"
#include "ec_random.h"
#include "g_strings.h"
#include "fake_platform.h"
#include <math.h>

string_definition_data * string_definitions = NULL;
int num_string_definitions = 0;
view_port_data view_port = { 0.0f, 0.0f, 1920.0f, 1080.0f };

static int failures = 0;
#define CHECK( cond ) do { if ( !( cond ) ) { printf( "FAILED line %d: %s\n", __LINE__, #cond ); failures++; } } while ( 0 )
static bool near( float a, float b, float tolerance = 0.0005f ) { return fabsf( a - b ) <= tolerance; }

// v * M with perspective divide, the convention the engine uses
static vector3d transform( vector3d v, matrix & m, float * w_out = NULL )
{
    float w = v.x * m.m[ 0 ][ 3 ] + v.y * m.m[ 1 ][ 3 ] + v.z * m.m[ 2 ][ 3 ] + m.m[ 3 ][ 3 ];
    vector3d r = {
        ( v.x * m.m[ 0 ][ 0 ] + v.y * m.m[ 1 ][ 0 ] + v.z * m.m[ 2 ][ 0 ] + m.m[ 3 ][ 0 ] ) / w,
        ( v.x * m.m[ 0 ][ 1 ] + v.y * m.m[ 1 ][ 1 ] + v.z * m.m[ 2 ][ 1 ] + m.m[ 3 ][ 1 ] ) / w,
        ( v.x * m.m[ 0 ][ 2 ] + v.y * m.m[ 1 ][ 2 ] + v.z * m.m[ 2 ][ 2 ] + m.m[ 3 ][ 2 ] ) / w };
    if ( w_out ) *w_out = w;
    return r;
}

int main()
{
    assert_system_init();
    memory_system_init();

    // look-at: the eye maps to the origin, the target lies straight ahead on +z
    vector3d eye = { 10.0f, 20.0f, -30.0f }, at = { 10.0f, 20.0f, 0.0f }, up = { 0.0f, 1.0f, 0.0f };
    matrix view;
    CHECK( matrix_lookat_lh( &view, eye, at, up ) );
    vector3d e = transform( eye, view ), a = transform( at, view );
    CHECK( near( e.x, 0 ) && near( e.y, 0 ) && near( e.z, 0 ) );
    CHECK( near( a.x, 0 ) && near( a.y, 0 ) && near( a.z, 30.0f ) );
    vector3d right = transform( vector3d{ 11.0f, 20.0f, 0.0f }, view );
    CHECK( near( right.x, 1.0f ) && near( right.y, 0 ) );

    // perspective: near plane -> depth 0, far plane -> depth 1
    matrix projection;
    CHECK( matrix_perspective_fov_lh( &projection, 1.0f, 1920.0f, 1080.0f, 1.0f, 100.0f ) );
    CHECK( near( transform( vector3d{ 0, 0, 1.0f }, projection ).z, 0.0f ) );
    CHECK( near( transform( vector3d{ 0, 0, 100.0f }, projection ).z, 1.0f ) );
    CHECK( near( projection.m[ 0 ][ 0 ] / projection.m[ 1 ][ 1 ], 1080.0f / 1920.0f ) );
    CHECK( !matrix_perspective_fov_lh( &projection, 0.0f, 1920.0f, 1080.0f, 1.0f, 100.0f ) );
    CHECK( matrix_perspective_fov_lh( &projection, 1.0f, 1920.0f, 1080.0f, 1.0f, 100.0f ) );

    // inverse: M * inverse(M) is the identity
    matrix combined, inverse, identity;
    matrix_multiply( &combined, &projection, &view );       // view * projection
    CHECK( matrix_inverse( &inverse, &combined ) );
    matrix_multiply( &identity, &inverse, &combined );
    bool is_identity = true;
    for ( int r = 0; r < 4; r++ ) for ( int c = 0; c < 4; c++ ) if ( !near( identity.m[ r ][ c ], r == c ? 1.0f : 0.0f, 0.001f ) ) is_identity = false;
    CHECK( is_identity );

    // unproject: project a world point to the screen and back
    vector3d world = { 13.0f, 18.5f, 5.0f };
    vector3d clip = transform( world, combined );
    vector3d screen = { ( clip.x + 1.0f ) * 0.5f * view_port.width, ( 1.0f - ( clip.y + 1.0f ) * 0.5f ) * view_port.height, clip.z };
    vector3d back;
    CHECK( matrix_unproject( &back, &screen, &view, &projection ) );
    CHECK( near( back.x, world.x, 0.01f ) && near( back.y, world.y, 0.01f ) && near( back.z, world.z, 0.01f ) );

    // spline: evenly spaced knots on a line interpolate linearly; ends are clamped
    vector2d knots[ 4 ] = { { 0, 0 }, { 10, 5 }, { 20, 10 }, { 30, 15 } };
    vector2d out = { -1, -1 };
    spline_position( knots, 4, 1, 0.0f, &out, NULL );
    CHECK( out.x == 10.0f && out.y == 5.0f );
    spline_position( knots, 4, 1, 1.0f, &out, NULL );
    CHECK( out.x == 20.0f && out.y == 10.0f );
    spline_position( knots, 4, 1, 0.5f, &out, NULL );
    CHECK( near( out.x, 15.0f ) && near( out.y, 7.5f ) );
    spline_position( knots, 4, 3, 0.5f, &out, NULL );            // last knot: stays there
    CHECK( near( out.x, 30.0f, 0.7f ) );
    out.x = -1;
    spline_position( knots, 1, 0, 0.5f, &out, NULL );            // too few knots: asserts, leaves out untouched
    CHECK( out.x == -1.0f );

    // picker
    CHECK( picker_choose() == -1 );
    for ( int i = 0; i < 5; i++ ) picker_add( 100 + i );
    bool in_range = true;
    for ( int i = 0; i < 200; i++ ) { int v = picker_choose(); if ( v < 100 || v > 104 ) in_range = false; }
    CHECK( in_range );
    picker_remove( 102 );
    bool removed = true;
    for ( int i = 0; i < 200; i++ ) if ( picker_choose() == 102 ) removed = false;
    CHECK( removed );
    picker_reset();
    CHECK( picker_choose() == -1 );

    printf( failures ? "%d check(s) FAILED\n" : "math: all checks passed\n", failures );
    return failures != 0;
}
