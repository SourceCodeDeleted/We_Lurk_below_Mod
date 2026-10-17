# It Lurks Below - rebuilt from the decompiled draft.
#
#   cmake -G Ninja -B build/headless -DILB_VARIANT=headless      debug build: no window, sanitizers
#   cmake -G Ninja -B build/native   -DILB_VARIANT=native        playable build: SDL2 window + OpenGL
#   cmake -G Ninja -B build/play     -DILB_VARIANT=play          the same without the debugging aids: fast
#   cmake --build build/<variant>
#
# The game code is draft/build/src/*.c, generated from the IDA export by tools/draft_fix.py and
# tools/draft_data.py ("cmake --build ... --target generate" re-runs them). It assumes 4-byte pointers, so
# every variant is a 32-bit build.
#
# Windows (the play and native variants; SDL2.dll and ILB.gbp are taken from ILB_GAME, by default game/):
#   on Windows, with clang and the libraries of Visual Studio and the Windows SDK (tools/build_windows.ps1):
#     cmake -G Ninja -B build/windows -DILB_VARIANT=play
#   on Linux, with the libraries fetched by xwin:
#     cmake -G Ninja -B build/windows -DILB_VARIANT=play --toolchain cmake/i686-windows-xwin.cmake
cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER clang CACHE STRING "")
if(CMAKE_HOST_WIN32 AND NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_C_COMPILER_TARGET i686-pc-windows-msvc CACHE STRING "")
endif()
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

if(WIN32)
    if(ILB_VARIANT STREQUAL "headless")
        message(FATAL_ERROR "The Windows program is built as -DILB_VARIANT=play or -DILB_VARIANT=native; headless is a Linux build")
    endif()
    # the C runtime every Windows machine has (ucrtbase.dll, vcruntime140.dll), never the debug one that only
    # comes with Visual Studio, whatever the build type
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
    # no libraries but the ones named below: the game has its own stand-ins for much of the Windows API
    set(CMAKE_C_STANDARD_LIBRARIES "-lkernel32 -luser32 -lws2_32 -loldnames")
    set(ILB_M32 "")
else()
    set(ILB_M32 -m32)
endif()

# your copy of the game: nothing of it is in the repository, the build takes what it needs from there
if(DEFINED ENV{ILB_GAME})
    set(ILB_GAME_DEFAULT "$ENV{ILB_GAME}")
else()
    set(ILB_GAME_DEFAULT "${CMAKE_SOURCE_DIR}/game")
endif()
set(ILB_GAME "${ILB_GAME_DEFAULT}" CACHE PATH "Your copy of the game: ILB.exe (for the data image), and for the Windows program SDL2.dll and ILB.gbp")
file(TO_CMAKE_PATH "${ILB_GAME}" ILB_GAME_DIR)

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
if(NOT DRAFT_SOURCES OR NOT EXISTS ${DRAFT}/build/native/generated_stubs.c)
    message(FATAL_ERROR "The game code (draft/build/src, draft/build/*.h, draft/build/native/generated_stubs.c) is missing; see \"Working on the decompilation\" in README.md")
endif()
# The data image is the one piece of the game the program is built from that is not in the repository: it
# is made from your own ILB.exe (tools/make_image.py; tools/draft_data.py makes it too when it regenerates).
if(NOT EXISTS ${DRAFT}/build/ILB_image.bin)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/make_image.py "${ILB_GAME_DIR}/ILB.exe" RESULT_VARIABLE IMAGE_STATUS)
    if(NOT IMAGE_STATUS EQUAL 0)
        message(FATAL_ERROR "draft/build/ILB_image.bin could not be made from ${ILB_GAME_DIR}/ILB.exe (-DILB_GAME=<folder of your copy of the game>)")
    endif()
endif()
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
    set(ILB_OPTIMISE -O1 -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks -ftrivial-auto-var-init=zero)
    if(CMAKE_C_COMPILER_VERSION VERSION_LESS 16)
        # the long flag is what clang 14 asks for; later versions do not need it and the newest do not know it
        list(APPEND ILB_OPTIMISE -enable-trivial-auto-var-init-zero-knowing-it-will-be-removed-from-clang)
    endif()
else()
    set(ILB_OPTIMISE -O0)
endif()

add_executable(ilb ${DRAFT_SOURCES} ${NATIVE_SOURCES} ${PORT_SOURCES})
set_target_properties(ilb PROPERTIES OUTPUT_NAME ILB_${ILB_VARIANT})
set_source_files_properties(${DRAFT}/build/src/_image.c PROPERTIES OBJECT_DEPENDS ${DRAFT}/build/ILB_image.bin)

# the draft is IDA output made compilable: Microsoft extensions, 16-bit wchar_t, its own declarations of
# everything (no system headers), and no warnings worth reading
if(WIN32)
    # the type sizes, wchar_t and structure layout are the target's own here. Functions the platform layer
    # stands in front of are renamed by port/windows/wrap.h, which takes the place of the linker's --wrap.
    target_compile_options(ilb PRIVATE
        -std=gnu2x -fms-extensions -msse2 -mfpmath=sse -g -gcodeview ${ILB_OPTIMISE} -Wno-everything
        -nostdinc -isystem ${CLANG_RESOURCE_DIR}/include -include ${CMAKE_SOURCE_DIR}/port/windows/wrap.h)
    set_source_files_properties(${DRAFT}/build/src/ec_assert.c PROPERTIES COMPILE_DEFINITIONS ILB_DEFINES_ASSERT_FAILED)
    target_sources(ilb PRIVATE ${CMAKE_SOURCE_DIR}/port/windows/real_thunks.c)
    if(ILB_FAST)
        set_target_properties(ilb PROPERTIES OUTPUT_NAME ILB_windows)
    else()
        set_target_properties(ilb PROPERTIES OUTPUT_NAME ILB_windows_native)
    endif()
    # (the map is what tools/windows_where.py reads; /base is the address crash reports are given for)
    target_link_options(ilb PRIVATE -g LINKER:/subsystem:windows LINKER:/entry:mainCRTStartup LINKER:/safeseh:no
        LINKER:/largeaddressaware LINKER:/stack:8388608 LINKER:/base:0x400000 LINKER:/debug
        LINKER:/map:$<TARGET_FILE_DIR:ilb>/$<TARGET_FILE_BASE_NAME:ilb>.map)

    # what the platform layer needs from the Windows C library and the crash report: built with the system's headers
    add_library(windows_shim OBJECT ${CMAKE_SOURCE_DIR}/port/windows/windows_shim.c)
    target_compile_options(windows_shim PRIVATE -O1 -w -g -gcodeview)
    target_compile_definitions(windows_shim PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_include_directories(windows_shim SYSTEM PRIVATE ${ILB_SYSTEM_INCLUDES})
    target_sources(ilb PRIVATE $<TARGET_OBJECTS:windows_shim>)

    # import library for SDL2.dll, made from the list of what that SDL2.dll offers: the game's own, or with
    # -DILB_SDL2_DLL=<file> another 32-bit SDL2.dll (a release from libsdl.org; the automatic builds use one)
    set(ILB_SDL2_DLL "${ILB_GAME_DIR}/SDL2.dll" CACHE FILEPATH "The 32-bit SDL2.dll the Windows program is linked against and runs with")
    if(NOT EXISTS "${ILB_SDL2_DLL}")
        message(FATAL_ERROR "${ILB_SDL2_DLL} is missing: SDL2.dll of your copy of the game is needed (-DILB_GAME=<folder>), or -DILB_SDL2_DLL=<file>")
    endif()
    get_filename_component(ILB_LLVM_BIN "${CMAKE_C_COMPILER}" DIRECTORY)
    find_program(ILB_READOBJ NAMES llvm-readobj llvm-readobj-14 HINTS "${ILB_LLVM_BIN}" REQUIRED)
    find_program(ILB_DLLTOOL NAMES llvm-dlltool llvm-dlltool-14 HINTS "${ILB_LLVM_BIN}" REQUIRED)
    execute_process(COMMAND ${ILB_READOBJ} --coff-exports "${ILB_SDL2_DLL}" OUTPUT_VARIABLE SDL2_EXPORTS RESULT_VARIABLE SDL2_STATUS)
    string(REGEX MATCHALL "Name: SDL_[A-Za-z0-9_]+" SDL2_NAMES "${SDL2_EXPORTS}")
    list(TRANSFORM SDL2_NAMES REPLACE "Name: " "")
    list(LENGTH SDL2_NAMES SDL2_COUNT)
    if(NOT SDL2_STATUS EQUAL 0 OR SDL2_COUNT LESS 100)
        message(FATAL_ERROR "${ILB_SDL2_DLL} could not be read as SDL2 (${SDL2_COUNT} functions found)")
    endif()
    list(JOIN SDL2_NAMES "\n" SDL2_DEF)
    file(WRITE ${CMAKE_BINARY_DIR}/SDL2.def "LIBRARY SDL2.dll\nEXPORTS\n${SDL2_DEF}\n")
    execute_process(COMMAND ${ILB_DLLTOOL} -m i386 -k -d ${CMAKE_BINARY_DIR}/SDL2.def -l ${CMAKE_BINARY_DIR}/SDL2.lib -D SDL2.dll RESULT_VARIABLE SDL2_STATUS)
    if(NOT SDL2_STATUS EQUAL 0)
        message(FATAL_ERROR "no import library for SDL2.dll could be made")
    endif()
    target_link_libraries(ilb PRIVATE ${CMAKE_BINARY_DIR}/SDL2.lib)

    # the program runs where it is built: SDL2 next to it, and the game's data where a copy of the game is at hand
    set(ILB_RUN_FILES "${ILB_SDL2_DLL}")
    if(EXISTS "${ILB_GAME_DIR}/ILB.gbp")
        list(APPEND ILB_RUN_FILES "${ILB_GAME_DIR}/ILB.gbp")
    else()
        message(STATUS "No ILB.gbp in ${ILB_GAME_DIR}: put the one of your copy of the game next to the program before starting it")
    endif()
    add_custom_command(TARGET ilb POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ILB_RUN_FILES} $<TARGET_FILE_DIR:ilb> VERBATIM)
else()
target_compile_options(ilb PRIVATE
    -m32 -std=gnu2x -fms-extensions -fshort-wchar -msse2 -mfpmath=sse -g ${ILB_OPTIMISE} -fno-pie -Wno-everything
    # type sizes as on Windows, which the decompiled stack layouts assume: long double is 8 bytes there, 12 here
    # ... and double / 64-bit integers are aligned to 8 bytes inside structures there, to 4 here
    -mlong-double-64 -malign-double
    -nostdinc -isystem ${CLANG_RESOURCE_DIR}/include)
target_link_options(ilb PRIVATE -m32 -no-pie -Wl,--wrap=assert_failed)
target_link_libraries(ilb PRIVATE m)
endif()
target_include_directories(ilb PRIVATE ${DRAFT}/include ${DRAFT}/build ${CMAKE_SOURCE_DIR}/port)

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
# sin, cos and atan2 are the game's own (port/ilb_math.c, from musl), so that Linux and Windows copies of a
# network game compute the same: the two C libraries round these differently in the last bit
target_compile_definitions(ilb PRIVATE sin=ilb_sin cos=ilb_cos atan2=ilb_atan2)
# ... and memory from malloc starts as zeros on every platform (port/ilb_alloc.c)
target_compile_definitions(ilb PRIVATE malloc=ilb_malloc)
if(ILB_MOD_NET_MENU)
    target_compile_definitions(ilb PRIVATE ILB_MOD_NET_MENU)
endif()
if(ILB_MOD_START_FLOW)
    target_compile_definitions(ilb PRIVATE ILB_MOD_START_FLOW)
endif()
if(ILB_CHEATS)
    target_compile_definitions(ilb PRIVATE ILB_CHEATS)
endif()

if(ILB_SANITIZE AND NOT ILB_FAST AND NOT WIN32)
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
target_compile_options(net_link PRIVATE ${ILB_M32} -O1 -w)
if(WIN32)
    target_compile_definitions(net_link PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_include_directories(net_link SYSTEM PRIVATE ${ILB_SYSTEM_INCLUDES})
endif()
target_link_libraries(ilb PRIVATE net_link)
target_compile_definitions(ilb PRIVATE ILB_NET_INPUT)

# Which copies the lobby matches with each other (port/net/net_lobby.c): the same version of the source and
# the same modules switched on, whatever the platform. The version is what git calls this commit (a tag such
# as v0.2 for a release; "-dirty" when there are changes not yet committed).
find_package(Git QUIET)
set(ILB_SOURCE_VERSION "unknown")
if(GIT_FOUND AND EXISTS ${CMAKE_SOURCE_DIR}/.git)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --match "v[0-9]*" --always --dirty WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE ILB_SOURCE_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    foreach(ref HEAD index packed-refs)         # configure again when a commit or tag is made
        if(EXISTS ${CMAKE_SOURCE_DIR}/.git/${ref})
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/.git/${ref})
        endif()
    endforeach()
endif()
set(ILB_GAME_VERSION "${ILB_SOURCE_VERSION}")
foreach(module ILB_MOD_SEASON_OUTFITS ILB_MOD_SEASON_EVENTS ILB_MOD_SEASON_BUILDINGS ILB_MOD_LOCAL_COOP ILB_FIX_STUCK_PLAYER
               ILB_MOD_COOP_SPAWN ILB_MOD_NET_MENU ILB_MOD_START_FLOW ILB_CHEATS)
    if(${module})
        string(APPEND ILB_GAME_VERSION "+${module}")
    endif()
endforeach()
message(STATUS "Game version for the lobby: ${ILB_GAME_VERSION}")
target_compile_definitions(net_link PRIVATE "ILB_GAME_VERSION=\"${ILB_GAME_VERSION}\"")

if(ILB_VARIANT STREQUAL "native" OR ILB_VARIANT STREQUAL "play")
    # Ogg Vorbis decoder for the music (public domain, one file); built on its own with the normal C headers
    add_library(stb_vorbis STATIC ${CMAKE_SOURCE_DIR}/third_party/stb_vorbis.c)
    target_compile_options(stb_vorbis PRIVATE ${ILB_M32} -O2 -w)
    target_compile_definitions(stb_vorbis PRIVATE STB_VORBIS_NO_STDIO STB_VORBIS_NO_PUSHDATA_API)
    if(WIN32)
        target_compile_definitions(stb_vorbis PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_include_directories(stb_vorbis SYSTEM PRIVATE ${ILB_SYSTEM_INCLUDES})
    endif()
    target_link_libraries(ilb PRIVATE stb_vorbis)
    target_compile_definitions(ilb PRIVATE ILB_WINDOW)
endif()
if(NOT WIN32 AND (ILB_VARIANT STREQUAL "native" OR ILB_VARIANT STREQUAL "play"))
    # (the same list as port/windows/wrap.h, which does this for the Windows program)
    target_link_options(ilb PRIVATE -Wl,--wrap=SDL_RWFromFile,--wrap=SDL_PollEvent,--wrap=SDL_CreateWindow
        -Wl,--wrap=SDL_GetWindowSurface,--wrap=SDL_FillRect,--wrap=SDL_UpdateWindowSurface
        -Wl,--wrap=SDL_GetMouseState,--wrap=SDL_GetKeyboardState,--wrap=SDL_SetWindowPosition
        -Wl,--wrap=SDL_NumJoysticks,--wrap=SDL_IsGameController,--wrap=SDL_GameControllerOpen
        -Wl,--wrap=SDL_GameControllerGetJoystick,--wrap=SDL_JoystickInstanceID,--wrap=SDL_GameControllerGetVendor
        -Wl,--wrap=SDL_GameControllerName,--wrap=SDL_GameControllerGetAxis,--wrap=SDL_GameControllerClose)
    # 32-bit runtime libraries by full name: no -dev packages are needed, the game has its own declarations
    target_link_libraries(ilb PRIVATE /usr/lib/i386-linux-gnu/libSDL2-2.0.so.0 /usr/lib/i386-linux-gnu/libGL.so.1)
endif()
