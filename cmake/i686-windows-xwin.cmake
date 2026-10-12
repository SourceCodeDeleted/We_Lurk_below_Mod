# Toolchain file: build the Windows program on Linux.
#
#   cmake -G Ninja -B build/windows -DILB_VARIANT=play --toolchain cmake/i686-windows-xwin.cmake
#   cmake --build build/windows
#
# clang compiles for Windows, lld-link links, and the Windows SDK and C runtime come from xwin
# (https://github.com/Jake-Shadle/xwin: "xwin --arch x86 splat --output ~/xwin"). XWIN=<dir> if elsewhere.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

if(DEFINED ENV{XWIN})
    set(XWIN "$ENV{XWIN}")
else()
    set(XWIN "$ENV{HOME}/xwin")
endif()
if(NOT IS_DIRECTORY "${XWIN}/crt/lib/x86")
    message(FATAL_ERROR "no Windows libraries in ${XWIN} (see the top of cmake/i686-windows-xwin.cmake)")
endif()

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET i686-pc-windows-msvc)
find_program(ILB_LLD_LINK NAMES lld-link lld-link-14 REQUIRED)
find_program(CMAKE_RC_COMPILER NAMES llvm-rc llvm-rc-14)
find_program(CMAKE_AR NAMES llvm-ar llvm-ar-14 REQUIRED)
set(CMAKE_LINKER "${ILB_LLD_LINK}")

# The game code declares everything itself and must not see the Windows headers, so they are not on the
# general command line: CMakeLists.txt gives them only to the few files written against them.
set(ILB_SYSTEM_INCLUDES "${XWIN}/crt/include" "${XWIN}/sdk/include/ucrt" "${XWIN}/sdk/include/um" "${XWIN}/sdk/include/shared")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=${ILB_LLD_LINK} -L${XWIN}/crt/lib/x86 -L${XWIN}/sdk/lib/um/x86 -L${XWIN}/sdk/lib/ucrt/x86")
# the checks CMake makes of the compiler only compile: its test programs would need the headers above
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
