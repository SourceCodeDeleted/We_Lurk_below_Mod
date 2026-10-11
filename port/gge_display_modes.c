// gge_display_modes.c - the list of screen resolutions for the options menu (hand-written).
//
// Replaces gge_create_display_mode_list (0x40D960, gge_main.cpp), which asked DXGI for the first monitor's
// modes. The rules are the original's: keep modes from 1024x720 up to 3840x2160, drop a mode equal to the
// one before it, write them as (width, height) pairs to the game's list at 0x817238 (512 entries at most)
// and return how many there are. SDL lists modes largest first, DXGI smallest first, so the order is turned.
#include "ILB_draft_decls.h"

#define DISPLAY_MODE_LIST   ( (int *)ILB_ADDR( 0x817238 ) )
#define MAX_DISPLAY_MODES   512

#ifdef ILB_WINDOW
typedef struct { unsigned int format; int w, h, refresh_rate; void *driverdata; } SDL_DisplayMode;
int SDL_GetNumDisplayModes( int display );
int SDL_GetDisplayMode( int display, int index, SDL_DisplayMode *mode );
#endif

int sub_40D960()
{
    int *list = DISPLAY_MODE_LIST, count = 0;
#ifdef ILB_WINDOW
    int available = SDL_GetNumDisplayModes( 0 );
    debug_log_format_line( (wchar_t *)L"Display Modes found: %i", available );
    for ( int i = available - 1; i >= 0 && count < MAX_DISPLAY_MODES; i-- )
    {
        SDL_DisplayMode mode;
        if ( SDL_GetDisplayMode( 0, i, &mode ) != 0 )
            continue;
        if ( mode.w < 1024 || mode.h < 720 || mode.w > 3840 || mode.h > 2160 )
            continue;
        if ( count > 0 && list[ ( count - 1 ) * 2 ] == mode.w && list[ ( count - 1 ) * 2 + 1 ] == mode.h )
            continue;
        debug_log_format_line( (wchar_t *)L"Display Mode Added: %ix%i <format:%i> <%iHz>", mode.w, mode.h, 87, mode.refresh_rate );
        list[ count * 2 ] = mode.w;
        list[ count * 2 + 1 ] = mode.h;
        count++;
    }
#else
    // headless debug build: one mode, so the menu code has something to show
    list[ 0 ] = 1920;
    list[ 1 ] = 1080;
    count = 1;
#endif
    debug_log_format_line( (wchar_t *)L"Display Modes Added: %i", count );
    return count;
}
