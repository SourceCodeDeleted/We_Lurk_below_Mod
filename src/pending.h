//-----------------------------------------------------------------------------
// pending.h
//
// Not part of the original source. Declarations for code that reconstructed files already call but
// that has not been reconstructed itself. Each entry gives where it lives in ILB.exe. Nothing here
// is defined yet, so a program using these will not link: that is intended, it keeps the gaps
// visible. Entries move to their real headers as their files are written.
//
// sub_XXXXXX names are functions whose original name is unknown. Names marked "chosen" stand for
// code that only exists inlined in the binary.

#ifndef _PENDING_H_
#define _PENDING_H_

struct SDL_Window;
union SDL_Event;

// main.cpp, same file as main()
void main_reset_startup_globals();      // chosen. Inlined in main after game_globals is cleared: a dozen
                                        // flag/default writes to game_globals fields 0x2038 - 0x2084, and the
                                        // clears of 0x38FA426, 0x38C770F and 0x651AD4 after the arguments are read
bool sub_5817D0();                      // client start-up: SDL_Init, window, graphics, fmod_sound_engine_init,
                                        // controller, cursor, then set_app_mode( appmode_init )
bool main_process_event( SDL_Event * event );   // chosen. 0x581280: SDL event dispatch to the app mode callbacks;
                                                // returns true to quit
// near g_game.cpp
void sub_4610D0( int width, int height );   // derives the UI scale factors in game_globals from the window size

// unknown owner (UI text)
struct safestring;
extern safestring gamepad_confirm_string;   // 0x60A150, names chosen: one-glyph strings set by controller_start
extern safestring gamepad_cancel_string;    // 0x60A958

// gge_window.cpp / gge_main.cpp
extern SDL_Window * app_window;         // 0x3855B08, name chosen
extern int window_width;                // 0x652138, names chosen
extern int window_height;               // 0x65213C
extern float window_scale_x;            // 0x652148: SDL mouse units to window pixels
extern float window_scale_y;            // 0x65214C
bool sub_412AC0();                      // creates the window (gge_create_app_window is inlined in it)
bool sub_412E60();                      // graphics device start-up; failure shows string 78 as "Fatal Error"
void sub_412C50();
void sub_40C5D0();                      // frees the render-to-texture resources (gge_main.cpp)

// gdi_main.cpp
bool gdi_init();                        // original name. Inlined in main: 1024x768 DIB section for server mode

// g_char_slot.cpp or g_collision.cpp (file boundary)
void sub_42ECA0();                      // resets a table of 4 48-byte entries at 0x60F2C0

// g_strings (the strings table descriptor at 0x5D41F8)
struct excel_parse_table;
extern excel_parse_table string_excel_data;     // name chosen after item_excel_data in the 2019 source

#endif
