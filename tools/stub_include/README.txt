Minimal stand-ins for the C runtime, Windows and SDL headers. They exist only so the files in src/ can be
syntax-checked on this Linux machine with:

    tools/check_src.sh      (crt/ + platform/ stubs, MSVC target, syntax only)
    tools/run_tests.sh      (platform/ stubs only, native build, runs tests/)

They declare just what src/ uses. A real build uses the real headers (Windows SDK, SDL2, FMOD).
