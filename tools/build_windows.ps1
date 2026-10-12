# Build the Windows program on Windows with CMake (32-bit, SDL2 window + OpenGL, the same code as the Linux builds).
#   tools\build_windows.ps1 [-Variant play|native] [-Cheats] [-Game <folder>]
#     play    optimised                                              -> build\windows\ILB_windows.exe
#     native  not optimised, records the functions entered (slow):   -> build\windows-native\ILB_windows_native.exe
#             for finding a crash
#     -Cheats with the testing shortcuts of port/cheats.c (god mode and the like). Without it they are left
#             out, also when the build folder had them before.
#     -Game   your copy of the game (SDL2.dll and ILB.gbp are taken from it); default game\ or $env:ILB_GAME
# The program runs in its build folder: saves go to native_appdata\ there and the log to ILB_windows.log.
# Needs: LLVM (clang, lld-link), CMake, Ninja ("winget install LLVM.LLVM Kitware.CMake Ninja-build.Ninja") and
# the C++ build tools of Visual Studio with a Windows SDK, for their libraries. The game code must exist
# (draft\build\src).
param(
    [ValidateSet('play', 'native')] [string]$Variant = 'play',
    [switch]$Cheats,
    [string]$Game
)
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
if (-not (Test-Path draft\build\src)) { Write-Host 'draft\build\src is missing'; exit 1 }

# tools installed a moment ago are not on this session's PATH yet
$ninja = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\Ninja-build.Ninja_*\ninja.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
foreach ($dir in "$env:ProgramFiles\LLVM\bin", "$env:ProgramFiles\CMake\bin", "$env:LOCALAPPDATA\Microsoft\WinGet\Links", $ninja.DirectoryName) {
    if (-not $dir) { continue }
    if ((Test-Path $dir) -and ($env:PATH -notlike "*$dir*")) { $env:PATH = "$dir;$env:PATH" }
}
foreach ($tool in 'clang', 'lld-link', 'cmake', 'ninja') {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { Write-Host "$tool is not installed (see the top of this script)"; exit 1 }
}

$out = if ($Variant -eq 'play') { 'build\windows' } else { 'build\windows-native' }
$exe = if ($Variant -eq 'play') { 'ILB_windows.exe' } else { 'ILB_windows_native.exe' }
$cheatsValue = if ($Cheats) { 'ON' } else { 'OFF' }
$options = @('-G', 'Ninja', '-B', $out, "-DILB_VARIANT=$Variant", "-DILB_CHEATS=$cheatsValue")
if ($Game) { $options += "-DILB_GAME=$Game" }

New-Item -ItemType Directory -Force build | Out-Null
$ErrorActionPreference = 'Continue'     # (the tools write notes to the error stream)
cmake @options > "$out.cmake.log" 2>&1
if ($LASTEXITCODE -ne 0) { Get-Content "$out.cmake.log" -Tail 20; exit 1 }
Remove-Item "$out.cmake.log"
cmake --build $out 2>&1 | Select-String -Pattern 'error|FAILED|undefined symbol|duplicate symbol|ninja: build stopped|Linking|no work' | Select-Object -First 30
if ($LASTEXITCODE -ne 0 -or -not (Test-Path "$out\$exe")) { exit 1 }
Get-Item "$out\$exe" | Select-Object Length, LastWriteTime, FullName | Format-List
