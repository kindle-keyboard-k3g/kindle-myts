#!/bin/sh
# Launcher for kindle-myts on Kindle Keyboard (Kindle 3 / K3)
# Stops the Amazon Java framework and watchdog before running myts-ng,
# and restores both daemons upon exit.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Discover modern myts-ng binary in order of preference
find_myts_binary() {
    if [ -x "$SCRIPT_DIR/myts-ng" ]; then
        echo "$SCRIPT_DIR/myts-ng"
    elif [ -x "$SCRIPT_DIR/myts-ng-kindle" ]; then
        echo "$SCRIPT_DIR/myts-ng-kindle"
    elif [ -x "$SCRIPT_DIR/myts-ng-kindle-dbg" ]; then
        echo "$SCRIPT_DIR/myts-ng-kindle-dbg"
    elif [ -x "$SCRIPT_DIR/myts-ng-dbg" ]; then
        echo "$SCRIPT_DIR/myts-ng-dbg"
    else
        echo ""
    fi
}

restore_kindle() {
    echo "[myts] Restoring Kindle Java framework and watchdog..."
    /etc/init.d/framework start 2>/dev/null || true
    /etc/init.d/pmond start 2>/dev/null || true
}

trap restore_kindle EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

echo "[myts] Stopping Kindle watchdog and Java framework..."
/etc/init.d/pmond stop 2>/dev/null || true
/etc/init.d/framework stop 2>/dev/null || true
sleep 1

cd "$SCRIPT_DIR" || exit 1

MYTS_BIN="$(find_myts_binary)"

if [ $# -eq 0 ]; then
    if [ -z "$MYTS_BIN" ]; then
        echo "[myts] Error: No myts-ng binary found in $SCRIPT_DIR!" >&2
        exit 1
    fi
    echo "[myts] Launching default terminal ($MYTS_BIN)..."
    "$MYTS_BIN" /dev/fb0 ter-u12n.hex
elif [ "${1#-}" != "$1" ]; then
    # First argument is a flag (e.g. --debug, --rows 66)
    if [ -z "$MYTS_BIN" ]; then
        echo "[myts] Error: No myts-ng binary found in $SCRIPT_DIR!" >&2
        exit 1
    fi
    echo "[myts] Launching $MYTS_BIN with options: $@"
    "$MYTS_BIN" "$@" /dev/fb0 ter-u12n.hex
else
    # First argument is a command (e.g. env SHELL=... or custom program)
    echo "[myts] Executing command: $@"
    "$@"
fi

exit_code=$?
echo "[myts] Exited with code $exit_code."
exit $exit_code
