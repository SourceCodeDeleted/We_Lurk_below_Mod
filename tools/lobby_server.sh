#!/bin/sh
# Build (first time) and run the lobby server on this machine, for testing. Usage: tools/lobby_server.sh [port]
# The game looks for it at 127.0.0.1:27960 unless ILB_LOBBY=<address[:port]> is set or the file
# draft/build/native/run/lobby.txt holds an address. For a real server see server/README.md.
cd "$(dirname "$0")/../server" || exit 1
[ -x target/release/ilb-lobby ] || cargo build --release || exit 1
exec target/release/ilb-lobby --port "${1:-27960}"
