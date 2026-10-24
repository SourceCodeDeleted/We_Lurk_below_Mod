// rand_trace.h - every rand() of the game's code says where it is called from (hand-written; force-included
// into the draft's files by CMakeLists.txt and tools/build_windows.sh, after the declarations).
//
// Two copies of a network game must draw the same random numbers in the same order. When they do not, the
// list of where each number was drawn, step by step, shows the first place they part - and file and line are
// the same on Linux and Windows, unlike the programs' addresses. The list is only written while a network
// session's debug records are on (draft/native/native_input.c, net_debug_on); otherwise this costs a call.
#pragma once
#include "ILB_draft_decls.h"
int ILB_rand_at( const char *file, int line );
#define rand() ILB_rand_at( __FILE_NAME__, __LINE__ )
