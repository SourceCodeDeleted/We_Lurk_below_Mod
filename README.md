# It Lurks Below - decompilation and Linux/Windows port

A work-in-progress decompilation of **It Lurks Below** (Graybeard Games; GOG build of 2022-01-09, 32-bit
Windows) and a rebuild of it that runs natively on Linux and on Windows with SDL2 and OpenGL instead of
Direct3D 11 and FMOD. The goal is a working game, not a byte-identical executable.

**No game data is included.** You need your own copy of the game, from
[GOG](https://www.gog.com/game/it_lurks_below) or
[Steam](https://store.steampowered.com/app/697550/It_Lurks_Below/): its data file `ILB.gbp` cannot be
distributed with this project.

## State

- The whole game runs from the rebuilt program: title, character creation, the world, saving and loading.
- Renderer: hand-written OpenGL 3.3 backend replacing the game's Direct3D 11 layer. Sound: hand-written
  mixer on SDL audio replacing FMOD (music is Ogg Vorbis, decoded by stb_vorbis).
- Additions that are not in the original, each a separate module that can be switched off (see below):
  online two-player games through a small lobby server, a changed story start, seasonal vendor outfits and
  drops, test cheats.
- Known limits: 32-bit only. Around 200 functions still carry values the decompiler lost, so gameplay bugs
  remain. The Windows build is new and has had far less testing than the Linux one. Players on Linux and
  Windows should not expect to stay in step with each other in a network game (the two C libraries round
  differently); same-platform games are what is tested.

`PROGRESS.md` lists every original source file with how many of its functions are named and rewritten.

## Layout

| Path | What it is |
|---|---|
| `tools/` | The pipeline: IDA scripts (`tools/ida/`), the generator that makes IDA's output compilable (`draft_fix.py`, `draft_data.py`), build and run scripts, pack file and shader tools |
| `db/` | Recovered knowledge: function and global names, types, enums, per-function prototype fixes |
| `draft/include`, `draft/glue`, `draft/native` | Headers for the generated code, and the platform layer under it (window, input, files, record/replay, network input) |
| `port/` | Hand-written code: OpenGL renderer, sound mixer, network link, the optional modules (`mod_*.c`, `fix_*.c`, `cheats.c`) |
| `server/` | Lobby and relay server for network games (Rust, no dependencies); see `server/README.md` |
| `src/`, `tests/` | Hand-rewritten C++ of the engine core (`ec_*`, `gge_*`) with tests; not yet what the game is built from |
| `docs/SOURCE_NOTES.md` | Notes on the original source files (structures, constants, file layout) taken from a public developer stream |
| `third_party/` | stb_vorbis (public domain) |

Not in the repository (see `.gitignore`): `game/` (your copy of the game) and `build/` (what you build).

## What you need

- Your own copy of the game, from [GOG](https://www.gog.com/game/it_lurks_below) or
  [Steam](https://store.steampowered.com/app/697550/It_Lurks_Below/). Put or link the game's folder as
  `game/` at the top of this repository, or set `ILB_GAME=<folder>`. The program needs `ILB.gbp` from it to
  run, and the Windows build takes `SDL2.dll` from it. The game code in `draft/build/src` and its data image
  `draft/build/ILB_image.bin` were made from the GOG build of 2022-01-09 (where the image is missing,
  `tools/make_image.py` makes it again from that build's `ILB.exe`; this needs Python 3). Development and
  testing use the GOG build.
- Linux (developed on Ubuntu 22.04): clang 14 or newer, CMake 3.20+, Ninja, Python 3, `gcc-multilib`,
  and the 32-bit SDL2 and OpenGL runtime libraries (`libsdl2-2.0-0:i386`, `libgl1:i386`).
- For the Windows build on Windows: LLVM (clang, lld-link), CMake and Ninja
  (`winget install LLVM.LLVM Kitware.CMake Ninja-build.Ninja`), and the C++ build tools of Visual Studio with
  a Windows SDK, which supply the libraries.
- For the Windows build on Linux, additionally to the Linux tools: `lld-link`, `llvm-dlltool`, `llvm-readobj`,
  `llvm-objcopy`, and the Windows SDK and C runtime libraries fetched with
  [xwin](https://github.com/Jake-Shadle/xwin) into `~/xwin` (`xwin --arch x86 splat --output ~/xwin`).
- For the lobby server: Rust 1.70 or newer.

## Building

### Linux

    tools/build_native.sh play        # optimised, SDL2 window + OpenGL  -> build/play/ILB_play
    tools/build_native.sh native      # the same with sanitizers and crash reports (slow)
    tools/build_native.sh headless    # no window or sound, sanitizers: for automated tests

    tools/play_game.sh                # builds if needed and starts the play build

or directly: `cmake -G Ninja -B build/play -DILB_VARIANT=play && cmake --build build/play`.

The game runs in `draft/build/native/run/`; saves are in `native_appdata/` there, and every start through
`play_game.sh` keeps a copy of that folder (the last 15) in `draft/build/native/save_backups/`.

### Windows

The same code as the Linux play build, built by the same `CMakeLists.txt`.

On Windows:

    tools\build_windows.ps1                   # optimised -> build\windows\ILB_windows.exe
    tools\build_windows.ps1 -Variant native   # not optimised, for finding a crash -> build\windows-native\

`ILB.gbp` and `SDL2.dll` are copied next to the program, which runs where it is built.

On Linux, either of:

    tools/build_windows.sh            # the older script -> build/windows/ILB_windows.exe
    cmake -G Ninja -B build/windows -DILB_VARIANT=play --toolchain cmake/i686-windows-xwin.cmake && cmake --build build/windows

(the CMake route from Linux is new and has not been run yet; the script is the tested one.) With the script,
copy `ILB_windows.exe` into a folder with the game's `ILB.gbp` and `SDL2.dll` and start it there.
`tools/run_windows.sh` starts it on Linux under Proton's Wine for testing.

Saves go to `native_appdata\` in the folder the program is started in and the log to `ILB_windows.log` there
(in the temporary folder if that one cannot be written to; `ILB_LOG_CONSOLE=1` sends it to the console
instead). A crash is written to the end of the log with the addresses involved, and
`tools/windows_where.py <address> ...` names the functions behind them from the link map. `ILB_NO_CRASH_BOX=1`
leaves out the message box that otherwise says so.

The older `tools/draft_symbols.sh` + `tools/link_draft.sh` pair links the unported Direct3D 11 / FMOD
draft as a Windows program; it predates the port and is kept for comparison with the original.

### Automatic builds

`.github/workflows/build.yml` builds the Linux and the Windows program and the lobby server on GitHub for
every push and pull request; the results are the artifacts of the run (Actions tab). They hold no game
data: put `ILB.gbp` of your copy of the game next to the program. The Windows one comes with an SDL2.dll
released by libsdl.org; the Linux one needs the 32-bit SDL2 and OpenGL libraries named above.

### Options

CMake options, on by default unless noted (`-D<name>=OFF` to build without):

| Option | Effect |
|---|---|
| `ILB_MOD_NET_MENU` | Play asks: single player, local multiplayer, host a game, join a game |
| `ILB_MOD_LOCAL_COOP` | F5 on the title menu opens the game's own local two-player set-up |
| `ILB_MOD_COOP_SPAWN` | A joining player starts at the Homestone, or at the world's start if there is none |
| `ILB_FIX_STUCK_PLAYER` | A player who loads inside solid ground is lifted out |
| `ILB_MOD_START_FLOW` | Changed story start: Homestone with the home base quest, Learning Potion from Commander Vile |
| `ILB_MOD_SEASON_OUTFITS` | Vendor outfits follow the in-game season (needs art built by `tools/make_season_npcs.py`) |
| `ILB_MOD_SEASON_EVENTS` | Pumpkins drop in Autumn |
| `ILB_MOD_SEASON_BUILDINGS` | Christmas lights, tree and Jolly Ol' Graybeard only in Winter; pumpkins on the furniture in Autumn (art from the same tool) |
| `ILB_CHEATS` | **Off by default.** Test shortcuts: `` ` `` level-up points, F1 gold and orbs, F2 survival points, F3 crafting materials, F4 god mode |

The cheats are only built in when asked for: `tools/build_native.sh play --cheats`, `tools/build_windows.sh --cheats`,
or `-DILB_CHEATS=ON` with CMake directly. A build without the flag never has them, even in a build folder that
had them before.

`tools/build_windows.sh` builds with all the other options; `ILB_WINDOWS_DEFS` adds further `-D` flags.

## Network games

Two players, each with their own copy and their own character; the host's world is used. Both copies run
the whole game and exchange only their input, so they must be the same build.

The game finds the public lobby server at `welurkbelow.com:27960`. `ILB_LOBBY=<address[:port]>`, or a file
`lobby.txt` next to `ILB.gbp` holding the address, points it at another one; a build made with
`-DILB_DEFAULT_LOBBY=\"address:port\"` in its compile flags uses that by default. For testing on one machine:

    tools/lobby_server.sh             # lobby server on this machine, port 27960
    ILB_LOBBY=127.0.0.1               # ... and each copy of the game pointed at it

Then choose Play, Host a game on one copy and Play, Join a
game on the other. A game can have a password; it is never sent in clear, but the traffic is not
encrypted. Running a public server is described in `server/README.md`.

## Working on the decompilation

Not needed to build or play: the generated game code is in the repository (`draft/build/src`, the three
headers in `draft/build` and `draft/build/native/generated_stubs.c`), and is committed again after it has
been regenerated. The game code in `draft/build/src/` is generated: IDA Pro (with the Hex-Rays
x86 decompiler) exports the program using the names and types in `db/`, and `tools/draft_fix.py` followed by
`tools/draft_data.py` turns that export into compilable C. The order of the IDA scripts is given at the top
of `tools/run_offline.sh`. Always run `draft_data.py` after `draft_fix.py`: the first one empties
`draft/build/src`.

## Tests and debugging

- `tools/run_tests.sh` builds and runs the tests of the rewritten engine core; `cargo test` in `server/`
  tests the lobby server through real sockets.
- `tools/run_game.sh` runs the debug build, which reports failed asserts and crashes with the original
  function names.
- `tools/record_game.sh <name>` records a session's input and `tools/replay_game.sh <name>` plays it back;
  the simulation is deterministic, and `tools/compare_checksums.py` compares two runs tick by tick.
- The platform layer reads a number of `ILB_*` environment variables for scripted runs (character slot,
  frame limit, scripted key presses, checksums); they are documented where they are read, in
  `draft/native/native_main.c` and `draft/native/native_input.c`.

## Legal

It Lurks Below is the property of Graybeard Games. This project is not affiliated with or endorsed by
them. Nothing here lets you play without owning the game.
