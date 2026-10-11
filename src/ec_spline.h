//-----------------------------------------------------------------------------
// ec_spline.h

#ifndef _EC_SPLINE_H_
#define _EC_SPLINE_H_

#include "ec_vector.h"

#define min_spline_knots    2
#define max_spline_knots    10

void spline_position( vector2d * knots, int num_knots, int current_knot, float time_delta, vector2d * out_pos, vector2d * unused );

#endif
