//-----------------------------------------------------------------------------
// ec_random.cpp
// No function of this file exists stand-alone in ILB.exe. picker_add (assert at line 72) and
// picker_choose (line 93) are reconstructed from their inlined copies (43 and 30 of them);
// picker_size and max_picker_size are named in the asserts. picker_reset and picker_remove are
// names chosen here for the two other things callers do with the list directly: set the size to
// zero, and drop a chosen value by moving the last entry into its place.

#include "main.h"
#include "ec_random.h"

static int picker_size = 0;                     // 0x3734500
static int picker_list[ max_picker_size ];      // 0x3734508

//-----------------------------------------------------------------------------

void picker_reset()
{
    picker_size = 0;
}

//-----------------------------------------------------------------------------

void picker_add( int value )
{
    assert_return( picker_size < max_picker_size );

    picker_list[ picker_size++ ] = value;
}

//-----------------------------------------------------------------------------
// returns -1 when the list is empty

int picker_choose()
{
    if ( !( picker_size > 0 ) )
    {
        assert_exp( picker_size > 0 );
        return -1;
    }

    return picker_list[ rand() % picker_size ];
}

//-----------------------------------------------------------------------------

void picker_remove( int value )
{
    for ( int i = 0; i < picker_size; i++ )
    {
        if ( picker_list[ i ] == value )
        {
            picker_size--;
            picker_list[ i ] = picker_list[ picker_size ];
            return;
        }
    }
}
