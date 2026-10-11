#pragma once
struct SDL_RWops;
const char * SDL_GetKeyName( int key );
SDL_RWops * SDL_RWFromFile( const char * file, const char * mode );
long long SDL_RWsize( SDL_RWops * context );
unsigned int SDL_RWread( SDL_RWops * context, void * ptr, unsigned int size, unsigned int maxnum );
int SDL_RWclose( SDL_RWops * context );
struct SDL_Window;
union SDL_Event { unsigned int type; unsigned char padding[ 56 ]; };
#define SDL_INIT_VIDEO 0x00000020u
int SDL_Init( unsigned int flags );
void SDL_Quit();
int SDL_PollEvent( SDL_Event * event );
void SDL_ShowWindow( SDL_Window * window );
void SDL_DestroyWindow( SDL_Window * window );
#define SDLK_LSHIFT 1073742049
#define SDLK_LCTRL  1073742048
#define SDLK_UP     1073741906
#define SDLK_DOWN   1073741905
int SDL_ShowCursor( int toggle );
struct SDL_GameController;
struct SDL_Joystick;
enum { SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY, SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY };
int SDL_NumJoysticks();
int SDL_IsGameController( int index );
SDL_GameController * SDL_GameControllerOpen( int index );
void SDL_GameControllerClose( SDL_GameController * controller );
SDL_Joystick * SDL_GameControllerGetJoystick( SDL_GameController * controller );
int SDL_JoystickInstanceID( SDL_Joystick * joystick );
unsigned short SDL_GameControllerGetVendor( SDL_GameController * controller );
const char * SDL_GameControllerName( SDL_GameController * controller );
short SDL_GameControllerGetAxis( SDL_GameController * controller, int axis );
int SDL_SetHint( const char * name, const char * value );
void * SDL_memset( void * dst, int c, unsigned int len );
