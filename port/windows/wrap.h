// wrap.h - the Windows build's stand-in for the linker's --wrap (hand-written; included in front of every
// game and platform source by CMakeLists.txt, Windows only).
//
// The platform layer stands in front of some functions: every call of X goes to __wrap_X, and __real_X is
// the original. On Linux the linker does that (--wrap=X in CMakeLists.txt, the same list). lld-link has no
// dependable option for it, so here the names are changed as the sources are compiled; __real_X for the SDL
// functions is in real_thunks.c. assert_failed is the game's own function: the file that defines it
// (ILB_DEFINES_ASSERT_FAILED, set for that file by CMakeLists.txt) defines __real_assert_failed instead.
#pragma once
#ifdef ILB_DEFINES_ASSERT_FAILED
#define assert_failed                   __real_assert_failed
#else
#define assert_failed                   __wrap_assert_failed
#endif
#ifdef ILB_WINDOW
#define SDL_RWFromFile                  __wrap_SDL_RWFromFile
#define SDL_PollEvent                   __wrap_SDL_PollEvent
#define SDL_CreateWindow                __wrap_SDL_CreateWindow
#define SDL_GetWindowSurface            __wrap_SDL_GetWindowSurface
#define SDL_FillRect                    __wrap_SDL_FillRect
#define SDL_UpdateWindowSurface         __wrap_SDL_UpdateWindowSurface
#define SDL_GetMouseState               __wrap_SDL_GetMouseState
#define SDL_GetKeyboardState            __wrap_SDL_GetKeyboardState
#define SDL_SetWindowPosition           __wrap_SDL_SetWindowPosition
#define SDL_NumJoysticks                __wrap_SDL_NumJoysticks
#define SDL_IsGameController            __wrap_SDL_IsGameController
#define SDL_GameControllerOpen          __wrap_SDL_GameControllerOpen
#define SDL_GameControllerGetJoystick   __wrap_SDL_GameControllerGetJoystick
#define SDL_JoystickInstanceID          __wrap_SDL_JoystickInstanceID
#define SDL_GameControllerGetVendor     __wrap_SDL_GameControllerGetVendor
#define SDL_GameControllerName          __wrap_SDL_GameControllerName
#define SDL_GameControllerGetAxis       __wrap_SDL_GameControllerGetAxis
#define SDL_GameControllerClose         __wrap_SDL_GameControllerClose
#endif
