// Stand-ins for the Windows and SDL calls used by src/, for the native tests. Include from exactly one
// translation unit per test. Files opened with CreateFileW live in an in-memory map; SDL_RWops read
// real files (the pack file test points them at the original ILB.gbp).
#pragma once
#include <windows.h>
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>

static std::map<std::wstring, std::vector<char>> fake_files;
static std::vector<std::wstring> fake_directories;
static int fake_message_boxes = 0;

struct fake_handle { std::vector<char> * data; size_t position; };

HANDLE CreateFileW( const wchar_t * name, DWORD access, DWORD, void *, DWORD creation, DWORD, HANDLE )
{
    if ( creation == CREATE_ALWAYS ) fake_files[ name ].clear();
    else if ( fake_files.find( name ) == fake_files.end() ) return INVALID_HANDLE_VALUE;
    return new fake_handle{ &fake_files[ name ], 0 };
}
BOOL WriteFile( HANDLE h, const void * data, DWORD size, DWORD * written, void * )
{
    fake_handle * f = (fake_handle *)h;
    f->data->insert( f->data->end(), (const char *)data, (const char *)data + size );
    *written = size;
    return 1;
}
BOOL ReadFile( HANDLE h, void * data, DWORD size, DWORD * read, void * )
{
    fake_handle * f = (fake_handle *)h;
    size_t n = f->data->size() - f->position;
    if ( n > size ) n = size;
    memcpy( data, f->data->data() + f->position, n );
    f->position += n;
    *read = (DWORD)n;
    return 1;
}
DWORD GetFileSize( HANDLE h, DWORD * ) { return (DWORD)( (fake_handle *)h )->data->size(); }
BOOL CloseHandle( HANDLE h ) { delete (fake_handle *)h; return 1; }
BOOL CreateDirectoryW( const wchar_t * name, void * ) { fake_directories.push_back( name ); return 1; }
BOOL MoveFileW( const wchar_t * from, const wchar_t * to )
{
    if ( fake_files.find( from ) == fake_files.end() || fake_files.find( to ) != fake_files.end() ) return 0;
    fake_files[ to ] = fake_files[ from ];
    fake_files.erase( from );
    return 1;
}
int MessageBoxW( HWND, const wchar_t *, const wchar_t *, unsigned int ) { fake_message_boxes++; return 0; }
wchar_t * _wgetenv( const wchar_t * ) { static wchar_t value[] = L"C:\\Users\\test\\AppData\\Local"; return value; }

static std::string fake_file_text( const wchar_t * name )
{
    std::vector<char> & d = fake_files[ name ];
    return std::string( d.begin(), d.end() );
}

const char * SDL_GetKeyName( int ) { return "Space"; }
SDL_RWops * SDL_RWFromFile( const char * file, const char * mode )
{
    std::string path( file );
    for ( char & c : path ) if ( c == '\\' ) c = '/';
    return (SDL_RWops *)fopen( path.c_str(), mode );
}
long long SDL_RWsize( SDL_RWops * f ) { fseek( (FILE *)f, 0, SEEK_END ); long long n = ftell( (FILE *)f ); fseek( (FILE *)f, 0, SEEK_SET ); return n; }
unsigned int SDL_RWread( SDL_RWops * f, void * ptr, unsigned int size, unsigned int num ) { return (unsigned int)fread( ptr, size, num, (FILE *)f ); }
int SDL_RWclose( SDL_RWops * f ) { return fclose( (FILE *)f ); }

// not-yet-reconstructed game code the engine core calls (see src/pending.h)
#include "g_game_globals.h"
game_globals_data game_globals;
static int fake_last_ui_width = 0;
void sub_4610D0( int width, int ) { fake_last_ui_width = width; }

// SDL game controller, scriptable from tests
static int fake_pads = 0;
static short fake_axis[ 4 ] = { 0, 0, 0, 0 };
static int fake_pad_closed = 0;
int SDL_NumJoysticks() { return fake_pads; }
int SDL_IsGameController( int ) { return 1; }
SDL_GameController * SDL_GameControllerOpen( int index ) { return (SDL_GameController *)(size_t)( index + 1 ); }
void SDL_GameControllerClose( SDL_GameController * ) { fake_pad_closed++; }
SDL_Joystick * SDL_GameControllerGetJoystick( SDL_GameController * ) { return (SDL_Joystick *)1; }
int SDL_JoystickInstanceID( SDL_Joystick * ) { return 0; }
unsigned short SDL_GameControllerGetVendor( SDL_GameController * ) { return 0x045E; }
const char * SDL_GameControllerName( SDL_GameController * ) { return "Test Pad"; }
short SDL_GameControllerGetAxis( SDL_GameController *, int axis ) { return fake_axis[ axis ]; }
int SDL_SetHint( const char *, const char * ) { return 1; }
int SDL_ShowCursor( int ) { return 0; }

#include "ec_strings.h"
safestring gamepad_confirm_string, gamepad_cancel_string;
menu_slot_data menu_slots[ max_clients ];
int window_width = 1920, window_height = 1080;
float window_scale_x = 1.0f, window_scale_y = 1.0f;
void * SDL_memset( void * dst, int c, unsigned int len ) { return memset( dst, c, len ); }
