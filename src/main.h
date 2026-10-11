//-----------------------------------------------------------------------------
// It Lurks Below - reconstructed source
// main.h: basic types and the engine core headers every file uses.
//
// Reconstruction notes: the original main.h is not visible anywhere; only its name is known
// (g_items.cpp includes it first). Type names dword / byte are the original ones.

#ifndef _MAIN_H_
#define _MAIN_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <stddef.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;

#define invalid_id      0xffffffff

#include "ec_assert.h"
#include "ec_strings.h"
#include "ec_memory.h"
#include "ec_debug_log.h"

#endif
