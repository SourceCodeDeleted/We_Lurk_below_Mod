# ilb-lobby

Lobby and relay server for two-player games of the It Lurks Below rebuild. A game copy connects, either
hosts a game (optionally with a password) or lists the games and joins one, and from then on the server only
passes the two players' bytes to each other. Players therefore need no open ports of their own.

No dependencies; Rust 1.70 or newer.

    cargo test                 # protocol, password check, relay, limits - through real sockets
    cargo build --release
    target/release/ilb-lobby --port 27960

It listens on all addresses, TCP only. Open that one port in the firewall. It keeps nothing on disk and
prints one line per event (who hosted, who joined, how many bytes a game relayed) to standard output.

## Running it as a service (systemd)

    [Unit]
    Description=It Lurks Below lobby
    After=network.target

    [Service]
    ExecStart=/opt/ilb-lobby/ilb-lobby --port 27960
    Restart=on-failure
    DynamicUser=yes
    NoNewPrivileges=yes
    ProtectSystem=strict
    ProtectHome=yes
    MemoryMax=256M

    [Install]
    WantedBy=multi-user.target

## What it protects against, and what it does not

- Passwords are never sent: the host registers `sha256(salt + password)`, a joiner answers a fresh random
  challenge with `sha256(challenge + that)`. The server stores only the hashed form, in memory.
- Limits: 200 games, 8 connections per address, 512-byte lines, 32-character names without control
  characters, 90 seconds of silence in the lobby, 120 seconds of silence in a game.
- Only copies of the same game build are shown to and matched with each other.
- The traffic is **not encrypted**. Someone who can read the network between a player and the server can
  see the game's input and the save files exchanged at the start, and could mount an offline guess at a
  weak password from a recorded join. Use it to keep strangers out of a game, not to keep secrets.
- The relay never interprets game data, so a broken or hostile game copy can only harm the one it is
  matched with, not the server.

The protocol is described at the top of `src/protocol.rs`.
