//-----------------------------------------------------------------------------
// ec_random.h
//
// random() is named in the 2019 source ( x = random( irlg_width ); ); in the binary every such
// place is rand() % range, so it is written here as an inline. The picker is a shared list of
// candidates: fill it with picker_add, then draw one with picker_choose.

#ifndef _EC_RANDOM_H_
#define _EC_RANDOM_H_

#define max_picker_size     1024

inline int random( int range ) { return rand() % range; }

void picker_reset();
void picker_add( int value );
int picker_choose();
void picker_remove( int value );

#endif
