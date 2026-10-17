// ilb_alloc.c - the game's malloc, which hands out memory that starts as zeros (hand-written).
//
// The decompiled code reads some of the memory it allocates before it has written all of it, where the
// decompiler lost a value. Linux happens to give fresh large blocks as zeros; Windows gives whatever its heap
// held before. A newly generated world then showed nothing but fog on Windows. Zeros on every platform make
// both behave the same, which network games between them need as well. CMakeLists.txt renames the game's
// calls (malloc -> ilb_malloc).
#include <stddef.h>

void *calloc( size_t, size_t );

void *ilb_malloc( size_t size )
{
    return calloc( 1, size ? size : 1 );
}
