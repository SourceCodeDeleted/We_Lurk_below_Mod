//-----------------------------------------------------------------------------
// ec_vector.h
//
// Vector types. The original header also holds inline helpers (an assert places vector2d_rotate at
// its line 371); none of those are reconstructed yet. Struct names are original, taken from
// function signatures.

#ifndef _EC_VECTOR_H_
#define _EC_VECTOR_H_

struct vector2d { float x, y; };
struct ivector2d { int x, y; };
struct vector3d { float x, y, z; };

#endif
