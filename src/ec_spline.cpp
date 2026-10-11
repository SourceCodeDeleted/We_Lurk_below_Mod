//-----------------------------------------------------------------------------
// ec_spline.cpp
// Reconstructed from ILB.exe: spline_position 0x4092A0. Parameter names are from the assert text.
// The sixth parameter appears in the function's signature but the compiled code never touches it.

#include "main.h"
#include "ec_spline.h"

//-----------------------------------------------------------------------------
// Catmull-Rom position between knots[ current_knot ] and the next knot; the ends are clamped.

void spline_position( vector2d * knots, int num_knots, int current_knot, float time_delta, vector2d * out_pos, vector2d * unused )
{
    assert_return( knots );
    assert_return( num_knots >= min_spline_knots && num_knots <= max_spline_knots );
    assert_return( current_knot >= 0 && current_knot < num_knots );
    assert_return( time_delta >= 0.0f && time_delta <= 1.0f );
    assert_return( out_pos );

    if ( time_delta == 0.0f )
    {
        *out_pos = knots[ current_knot ];
        return;
    }

    int k0 = ( current_knot > 0 ) ? current_knot - 1 : 0;
    int k2 = ( current_knot + 1 < num_knots ) ? current_knot + 1 : num_knots - 1;
    int k3 = ( k2 + 1 < num_knots ) ? k2 + 1 : num_knots - 1;

    if ( time_delta == 1.0f )
    {
        *out_pos = knots[ k2 ];
        return;
    }

    vector2d p0 = knots[ k0 ], p1 = knots[ current_knot ], p2 = knots[ k2 ], p3 = knots[ k3 ];
    float t = time_delta;

    out_pos->x = ( ( ( p1.x * 3.0f - p0.x * 1.0f - p2.x * 3.0f + p3.x ) * t * t * t )
                 + ( ( p0.x + p0.x - p1.x * 5.0f + p2.x * 4.0f - p3.x ) * t * t )
                 + ( ( p2.x - p0.x ) * t )
                 + ( p1.x + p1.x ) ) * 0.5f;
    out_pos->y = ( ( ( p1.y * 3.0f - p0.y * 1.0f - p2.y * 3.0f + p3.y ) * t * t * t )
                 + ( ( p0.y + p0.y - p1.y * 5.0f + p2.y * 4.0f - p3.y ) * t * t )
                 + ( ( p2.y - p0.y ) * t )
                 + ( p1.y + p1.y ) ) * 0.5f;
}
