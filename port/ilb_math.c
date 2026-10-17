// ilb_math.c - sin, cos and atan2 of the game's own, the same on every platform (hand-written wrapper around
// musl's implementations in third_party/musl_math, MIT licence).
//
// Both copies of a network game run the whole simulation and only exchange input, so every calculation must
// come out the same on Linux and on Windows. Most of the C library's maths does by the IEEE standard (floor,
// sqrt, rounding); sin, cos and atan2 do not, the two systems' libraries round differently in the last bit.
// CMakeLists.txt renames the game's calls to these (sin -> ilb_sin and so on) for every file of the game,
// this one included, so musl's definitions below become ilb_sin, ilb_cos and ilb_atan2.
#define atan ilb_musl_atan      // used by atan2; not called by the game itself

#include "../third_party/musl_math/__rem_pio2_large.c"
#include "../third_party/musl_math/__rem_pio2.c"
#include "../third_party/musl_math/__sin.c"
#include "../third_party/musl_math/__cos.c"
#include "../third_party/musl_math/sin.c"
#include "../third_party/musl_math/cos.c"
#include "../third_party/musl_math/atan.c"
#include "../third_party/musl_math/atan2.c"
