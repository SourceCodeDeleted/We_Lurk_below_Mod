#!/bin/sh
# Two copies of the game playing together over a network connection (first working version; see
# draft/native/native_input.c, "two copies over a network").
#
#   tools/net_game.sh host [port]              player 1: keyboard and mouse. Start this one first.
#   tools/net_game.sh join [address[:port]]    player 2: controller. Default address: this machine.
#
# Each copy shows its own player's view at full window size (ILB_NET_SPLIT=1 shows the split screen on both
# instead). Each runs in its own folder under
# draft/build/native/net/ with a COPY of the current saves, so the real saves are not touched - and both
# copies must start from the same saves, which on one machine this guarantees. In the menus, player 1
# presses F5 and sets up the two-player game as in local co-op; player 2's choices come from the controller
# on the joining copy.
cd "$(dirname "$0")/.."
root=$(pwd)
role="${1:?usage: tools/net_game.sh host [port] | join [address[:port]]}"
NET_ARG="$2"
[ -x build/play/ILB_play ] || tools/build_native.sh play || exit 1
run=$root/draft/build/native/net/$role
rm -rf "$run"
mkdir -p "$run"
ln -sf "${ILB_GAME:-$PWD/game}/ILB.gbp" "$run/ILB.gbp"
[ -d $root/draft/build/native/run/mods ] && ln -sf $root/draft/build/native/run/mods "$run/mods"
[ -d $root/draft/build/native/run/native_appdata ] && cp -a $root/draft/build/native/run/native_appdata "$run/native_appdata"
# Two windows on one screen: the narrowest the game accepts (1024 wide), side by side, each labelled; the
# right one overlaps the left a little on a 1920 screen. Both copies must use the same window
# size (the interface layout is part of the game state), so the same size is set for both; ILB_NET_SIZE
# overrides it ("1920 1080" for two machines).
config="$run/native_appdata/Graybeard Games/It Lurks Below/ilb_config.txt"
width=$(echo "${ILB_NET_SIZE:-1024 1080}" | cut -d' ' -f1); height=$(echo "${ILB_NET_SIZE:-1024 1080}" | cut -d' ' -f2)
if [ -f "$config" ]; then
    sed -i "s/^fullscreen .*/fullscreen 0/; s/^borderless .*/borderless 0/; s/^window_width .*/window_width $width/; s/^window_height .*/window_height $height/" "$config"
fi
export ILB_WINDOW_Y=0
if [ "$role" = host ]; then
    export ILB_WINDOW_X=0 ILB_WINDOW_TITLE="It Lurks Below - PLAYER 1 (host): keyboard and mouse go HERE"
else
    export ILB_WINDOW_X=${ILB_NET_X:-896} ILB_WINDOW_TITLE="It Lurks Below - PLAYER 2 (joined): controller"
fi
cd "$run"
if [ "$role" = host ]; then
    ILB_NET_HOST="${NET_ARG:-27960}" ILB_CHECKSUM="$run/checksum.txt" "$root/build/play/ILB_play" 2>&1 | tee game.log | grep -v "^native: frame"
else
    # the controller must keep working while the other copy's window has the keyboard
    SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS=1 ILB_NET_JOIN="${NET_ARG:-127.0.0.1:27960}" ILB_CHECKSUM="$run/checksum.txt" \
        "$root/build/play/ILB_play" 2>&1 | tee game.log | grep -v "^native: frame"
fi
other=$root/draft/build/native/net/$( [ "$role" = host ] && echo join || echo host )/checksum.txt
[ -f "$other" ] && python3 "$root/tools/compare_checksums.py" "$run/checksum.txt" "$other"
