# It Lurks Below - rebuilt from the decompiled draft.
#
#   cmake -G Ninja -B build/headless -DILB_VARIANT=headless      debug build: no window, sanitizers
#   cmake -G Ninja -B build/native   -DILB_VARIANT=native        playable build: SDL2 window + OpenGL
#   cmake -G Ninja -B build/play     -DILB_VARIANT=play          the same without the debugging aids: fast
#   cmake --build build/<variant>
#
# The game code is draft/build/src/*.c, generated from the IDA export by tools/draft_fix.py and
# tools/draft_data.py ("cmake --build ... --target generate" re-runs them). It assumes 4-byte pointers, so
# every variant is a 32-bit build. The Windows exe (the same code as the play variant) is cross-built on
# Linux by tools/build_windows.sh.
cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER clang CACHE STRING "")
project(ItLurksBelow C)

set(ILB_VARIANT "headless" CACHE STRING "headless (no window, sanitizers), native (SDL2 + OpenGL, sanitizers) or play (SDL2 + OpenGL, optimised)")
set_property(CACHE ILB_VARIANT PROPERTY STRINGS headless native play)
if(ILB_VARIANT STREQUAL "play")
    set(ILB_FAST ON)
else()
    set(ILB_FAST OFF)
endif()
option(ILB_SANITIZE "AddressSanitizer and UBSan" ON)
option(ILB_MOD_SEASON_OUTFITS "Vendor outfits by in-game season (port/mod_season_outfits.c, not in the original); art from tools/make_season_npcs.py" ON)
option(ILB_MOD_SEASON_EVENTS "Holiday events by in-game season (port/mod_season_events.c, not in the original): pumpkins drop in Autumn" ON)
option(ILB_MOD_SEASON_BUILDINGS "Building decoration by in-game season (port/mod_season_buildings.c, not in the original): Christmas lights, tree and Jolly Ol' Graybeard in Winter only, pumpkins on the furniture in Autumn" ON)
option(ILB_MOD_LOCAL_COOP "F5 on the title menu opens the game's own Local Multiplayer set-up screen (port/mod_local_coop.c)" ON)
option(ILB_FIX_STUCK_PLAYER "A player who loads inside solid ground is lifted out (port/fix_stuck_player.c, not in the original)" ON)
option(ILB_MOD_COOP_SPAWN "In a two-player game both players start at the Homestone (port/mod_coop_spawn.c, not in the original)" ON)
option(ILB_MOD_NET_MENU "Play asks: single player, local multiplayer, host or join a game through the lobby server (port/mod_net_menu.c, not in the original)" ON)
option(ILB_MOD_START_FLOW "Changed story start (port/mod_start_flow.c, not in the original): Homestone with the home base quest, Learning Potion after Commander Vile" ON)
option(ILB_CHEATS "Testing shortcuts (port/cheats.c): ` level-up points, F1 gold and orbs for ancient skills, F2 survival points, F3 crafting materials, F4 god mode. Off unless asked for" OFF)

set(DRAFT ${CMAKE_SOURCE_DIR}/draft)
execute_process(COMMAND ${CMAKE_C_COMPILER} -print-resource-dir OUTPUT_VARIABLE CLANG_RESOURCE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

# ---- generated sources (the draft itself, the data image, the import stubs)
add_custom_target(generate
    COMMAND python3 tools/draft_fix.py
    COMMAND python3 tools/draft_data.py
    COMMAND python3 tools/native_stubs.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Regenerating the draft from the IDA export")

file(GLOB DRAFT_SOURCES CONFIGURE_DEPENDS ${DRAFT}/build/src/*.c)
set(NATIVE_SOURCES
    ${DRAFT}/build/native/generated_stubs.c
    ${DRAFT}/native/native_platform.c
    ${DRAFT}/native/native_main.c
    ${DRAFT}/native/native_input.c
    ${DRAFT}/native/native_trace.c)
# every game function reports its entry to the crash recorder (draft/native/native_trace.c)
if(NOT ILB_FAST)
    set_source_files_properties(${DRAFT_SOURCES} PROPERTIES COMPILE_OPTIONS -finstrument-functions)
endif()

# the OpenGL renderer (hand-written; the rendering layer calls it where the original called Direct3D)
file(GLOB PORT_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/port/*.c)

# The play variant is optimised, with the assumptions an optimiser may not make about decompiled code switched
# off (pointers of different types do alias, integers do wrap, null is dereferenced on purpose in places), and
# with every local variable starting at zero: where the decompiler lost a value, the result is then at least
# the same on every run and every machine.
if(ILB_FAST)
    set(ILB_OPTIMISE -O1 -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks -ftrivial-auto-var-init=zero
        -enable-trivial-auto-var-init-zero-knowing-it-will-be-removed-from-clang)   # the long flag is what clang 14 asks for
else()
    set(ILB_OPTIMISE -O0)
endif()

add_executable(ilb ${DRAFT_SOURCES} ${NATIVE_SOURCES} ${PORT_SOURCES})
set_target_properties(ilb PROPERTIES OUTPUT_NAME ILB_${ILB_VARIANT})
set_source_files_properties(${DRAFT}/build/src/_image.c PROPERTIES OBJECT_DEPENDS ${DRAFT}/build/ILB_image.bin)

# the draft is IDA output made compilable: Microsoft extensions, 16-bit wchar_t, its own declarations of
# everything (no system headers), and no warnings worth reading
target_compile_options(ilb PRIVATE
    -m32 -std=gnu2x -fms-extensions -fshort-wchar -msse2 -mfpmath=sse -g ${ILB_OPTIMISE} -fno-pie -Wno-everything
    # type sizes as on Windows, which the decompiled stack layouts assume: long double is 8 bytes there, 12 here
    # ... and double / 64-bit integers are aligned to 8 bytes inside structures there, to 4 here
    -mlong-double-64 -malign-double
    -nostdinc -isystem ${CLANG_RESOURCE_DIR}/include)
target_include_directories(ilb PRIVATE ${DRAFT}/include ${DRAFT}/build ${CMAKE_SOURCE_DIR}/port)
target_link_options(ilb PRIVATE -m32 -no-pie -Wl,--wrap=assert_failed)
target_link_libraries(ilb PRIVATE m)

if(ILB_MOD_SEASON_OUTFITS)
    target_compile_definitions(ilb PRIVATE ILB_MOD_SEASON_OUTFITS)
endif()
if(ILB_MOD_SEASON_EVENTS)
    target_compile_definitions(ilb PRIVATE ILB_MOD_SEASON_EVENTS)
endif()
if(ILB_MOD_SEASON_BUILDINGS)
    target_compile_definitions(ilb PRIVATE ILB_MOD_SEASON_BUILDINGS)
endif()
if(ILB_MOD_LOCAL_COOP)
    target_compile_definitions(ilb PRIVATE ILB_MOD_LOCAL_COOP)
endif()
if(ILB_FIX_STUCK_PLAYER)
    target_compile_definitions(ilb PRIVATE ILB_FIX_STUCK_PLAYER)
endif()
if(ILB_MOD_COOP_SPAWN)
    target_compile_definitions(ilb PRIVATE ILB_MOD_COOP_SPAWN)
endif()
target_compile_definitions(ilb PRIVATE ILB_MOD_PER_STEP)    # the once-per-step hook the port modules share
if(ILB_MOD_NET_MENU)
    target_compile_definitions(ilb PRIVATE ILB_MOD_NET_MENU)
endif()
if(ILB_MOD_START_FLOW)
    target_compile_definitions(ilb PRIVATE ILB_MOD_START_FLOW)
endif()
if(ILB_CHEATS)
    target_compile_definitions(ilb PRIVATE ILB_CHEATS)
endif()

if(ILB_SANITIZE AND NOT ILB_FAST)
    # the checks left out are ones the decompiled code violates by design (indexing past one global into
    # the next, for example)
    set(SANITIZE -fsanitize=address,undefined
        -fno-sanitize=alignment,shift,signed-integer-overflow,pointer-overflow,bounds,function,bool,enum
        -fno-omit-frame-pointer)
    target_compile_options(ilb PRIVATE ${SANITIZE})
    target_link_options(ilb PRIVATE ${SANITIZE})
endif()

# the TCP link between two copies of the game: built on its own with the system's C headers
add_library(net_link STATIC ${CMAKE_SOURCE_DIR}/port/net/net_link.c ${CMAKE_SOURCE_DIR}/port/net/net_lobby.c
    ${CMAKE_SOURCE_DIR}/port/net/net_session.c ${CMAKE_SOURCE_DIR}/port/net/sha256.c)
target_compile_options(net_link PRIVATE -m32 -O1 -w)
target_link_libraries(ilb PRIVATE net_link)
target_compile_definitions(ilb PRIVATE ILB_NET_INPUT)

if(ILB_VARIANT STREQUAL "native" OR ILB_VARIANT STREQUAL "play")
    # Ogg Vorbis decoder for the music (public domain, one file); built on its own with the normal C headers
    add_library(stb_vorbis STATIC ${CMAKE_SOURCE_DIR}/third_party/stb_vorbis.c)
    target_compile_options(stb_vorbis PRIVATE -m32 -O2 -w)
    target_compile_definitions(stb_vorbis PRIVATE STB_VORBIS_NO_STDIO STB_VORBIS_NO_PUSHDATA_API)
    target_link_libraries(ilb PRIVATE stb_vorbis)
    target_compile_definitions(ilb PRIVATE ILB_WINDOW)
    target_link_options(ilb PRIVATE -Wl,--wrap=SDL_RWFromFile,--wrap=SDL_PollEvent,--wrap=SDL_CreateWindow
        -Wl,--wrap=SDL_GetWindowSurface,--wrap=SDL_FillRect,--wrap=SDL_UpdateWindowSurface
        -Wl,--wrap=SDL_GetMouseState,--wrap=SDL_GetKeyboardState,--wrap=SDL_SetWindowPosition
        -Wl,--wrap=SDL_NumJoysticks,--wrap=SDL_IsGameController,--wrap=SDL_GameControllerOpen
        -Wl,--wrap=SDL_GameControllerGetJoystick,--wrap=SDL_JoystickInstanceID,--wrap=SDL_GameControllerGetVendor
        -Wl,--wrap=SDL_GameControllerName,--wrap=SDL_GameControllerGetAxis,--wrap=SDL_GameControllerClose)
    # 32-bit runtime libraries by full name: no -dev packages are needed, the game has its own declarations
    target_link_libraries(ilb PRIVATE /usr/lib/i386-linux-gnu/libSDL2-2.0.so.0 /usr/lib/i386-linux-gnu/libGL.so.1)
endif()
