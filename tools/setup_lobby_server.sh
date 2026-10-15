#!/bin/sh
# Install (or update) the lobby server as a service on a Linux machine: AlmaLinux / Rocky / RHEL / CentOS
# Stream, also Fedora and Debian/Ubuntu. Run as root from a copy of this repository (only tools/ and server/
# are needed). Usage: tools/setup_lobby_server.sh [port]          (default 27960)
#
# It builds server/ there (Rust is installed with rustup if missing; the build needs no network beyond that,
# the server has no dependencies), installs it as /opt/ilb-lobby/ilb-lobby, runs it under systemd as the
# service ilb-lobby, and opens the port in firewalld. Running it again after an update rebuilds and restarts.
# A firewall in front of the machine (the hosting provider's panel) has to allow the TCP port as well.
set -e
[ "$(id -u)" = 0 ] || { echo "run this as root (sudo $0)"; exit 1; }
port="${1:-27960}"
case "$port" in *[!0-9]*|"") echo "the port must be a number"; exit 1;; esac
source_dir="$(cd "$(dirname "$0")/../server" && pwd)"
[ -f "$source_dir/Cargo.toml" ] || { echo "no server/ next to tools/ (copy both)"; exit 1; }

# ---- a compiler and Rust
if ! command -v cc > /dev/null 2>&1; then
    if command -v dnf > /dev/null 2>&1; then dnf install -y gcc curl
    elif command -v apt-get > /dev/null 2>&1; then apt-get update && apt-get install -y gcc curl
    else echo "install gcc first"; exit 1
    fi
fi
PATH="$HOME/.cargo/bin:$PATH"
if ! command -v cargo > /dev/null 2>&1; then
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal
fi

# ---- build and test
cd "$source_dir"
cargo test --release
cargo build --release

# ---- install and run it
systemctl stop ilb-lobby 2> /dev/null || true
install -D -m 755 target/release/ilb-lobby /opt/ilb-lobby/ilb-lobby
command -v restorecon > /dev/null 2>&1 && restorecon -R /opt/ilb-lobby
cat > /etc/systemd/system/ilb-lobby.service <<EOF
[Unit]
Description=It Lurks Below lobby server
After=network-online.target
Wants=network-online.target

[Service]
ExecStart=/opt/ilb-lobby/ilb-lobby --port $port
Restart=always
RestartSec=3
DynamicUser=yes
NoNewPrivileges=yes
ProtectSystem=strict
ProtectHome=yes
PrivateTmp=yes
MemoryMax=256M

[Install]
WantedBy=multi-user.target
EOF
systemctl daemon-reload
systemctl enable --now ilb-lobby

# ---- the firewall
if systemctl is-active --quiet firewalld; then
    firewall-cmd --permanent --add-port="$port/tcp"
    firewall-cmd --reload
    echo "firewalld: port $port/tcp open"
elif command -v ufw > /dev/null 2>&1 && ufw status | grep -q active; then
    ufw allow "$port/tcp"
else
    echo "no active firewalld or ufw found: make sure TCP port $port is reachable"
fi

sleep 1
systemctl --no-pager --lines=5 status ilb-lobby
echo
echo "Listening:"; ss -ltn "sport = :$port"
echo "Logs: journalctl -u ilb-lobby -f"
