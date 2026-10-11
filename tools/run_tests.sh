#!/bin/sh
# Build the reconstructed engine-core files natively and run tests/. Windows and SDL calls are faked by
# the tests; MSVC-only spellings are mapped with -D. This checks behaviour, not the Windows build.
#   GAME_DIR  folder holding the original ILB.gbp (default: the GOG extract)
# The pack file test also needs dump/gbp (tools/gbp_unpack.py --extract dump/gbp); it is skipped without it.
cd "$(dirname "$0")/.."
set -e
out="${TMPDIR:-/tmp}/ilb_tests"
mkdir -p "$out"
GAME_DIR="${GAME_DIR:-$PWD/game}"
CXX="clang++ -std=c++17 -O1 -fms-extensions -w -D__FUNCSIG__=__PRETTY_FUNCTION__ -D__debugbreak()=__builtin_trap() \
    -D__stdcall= -D_stricmp=strcasecmp -include tests/native_decls.h -Isrc -isystem tools/stub_include/platform"
CORE="src/ec_assert.cpp src/ec_strings.cpp src/ec_memory.cpp src/ec_debug_log.cpp src/ec_fileio.cpp src/ec_config.cpp src/ec_controller.cpp src/ec_cursor.cpp"

$CXX tests/test_ec_core.cpp $CORE -o "$out/test_ec_core"

$CXX tests/test_appdata.cpp $CORE -o "$out/test_appdata"
"$out/test_appdata"

$CXX tests/test_config.cpp $CORE -o "$out/test_config"
"$out/test_config"

$CXX tests/test_controller.cpp $CORE -o "$out/test_controller"
"$out/test_controller"

$CXX tests/test_math.cpp $CORE src/ec_matrix.cpp src/ec_spline.cpp src/ec_random.cpp -o "$out/test_math"
"$out/test_math"
"$out/test_ec_core"

if [ -f "$GAME_DIR/ILB.gbp" ] && [ -d dump/gbp ]; then
    $CXX tests/test_pack_file.cpp $CORE -o "$out/test_pack_file"
    "$out/test_pack_file" "$GAME_DIR" dump/gbp
    $CXX -D_atoi64=atoll tests/test_excel.cpp $CORE src/ec_excel.cpp -o "$out/test_excel"
    "$out/test_excel" "$GAME_DIR" dump/gbp
    [ -f dump/gs3_manifest.txt ] || python3 tools/gs3_to_png.py --manifest dump/gbp dump/gs3_manifest.txt
    $CXX tests/test_gs3.cpp $CORE src/gge_gs3.cpp -o "$out/test_gs3"
    "$out/test_gs3" "$GAME_DIR" dump/gs3_manifest.txt
    GFX="src/gge_gs3.cpp src/gge_textures.cpp src/gge_materials.cpp src/gge_models.cpp"
    $CXX tests/test_textures.cpp $CORE $GFX -o "$out/test_textures"
    "$out/test_textures" "$GAME_DIR"
    $CXX tests/test_materials.cpp $CORE $GFX src/gge_material_types.cpp src/gge_mat_*.cpp -o "$out/test_materials"
    "$out/test_materials" "$GAME_DIR"
    $CXX tests/test_models.cpp $CORE $GFX -o "$out/test_models"
    "$out/test_models" "$GAME_DIR"
    $CXX tests/test_gge_main.cpp $CORE $GFX src/gge_material_types.cpp src/gge_mat_*.cpp src/gge_main.cpp src/gge_view.cpp -o "$out/test_gge_main"
    "$out/test_gge_main" "$GAME_DIR"
    wav=$(ls dump/gbp/audio | head -1); ogg=$(ls dump/gbp/music | head -1)
    $CXX tests/test_sound.cpp $CORE src/ec_fmod.cpp -o "$out/test_sound"
    "$out/test_sound" "$GAME_DIR" "${wav%.wav}" "${ogg%.ogg}"
else
    echo "pack file and excel tests skipped (needs ILB.gbp and dump/gbp)"
fi
