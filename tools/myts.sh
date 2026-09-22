#!/bin/sh
# Compatibility wrapper for legacy Launchpad configurations invoking myts.sh.
# Prevents FIFO deadlocks on /var/tmp/myts.special by redirecting to modern myts.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Clean up stale named pipe if present to avoid pipe_wait deadlocks
rm -f /var/tmp/myts.special 2>/dev/null || true

if [ "$1" = "kill" ]; then
    killall myts-ng myts-ng-kindle myts-ng-kindle-dbg myts 2>/dev/null || true
    /etc/init.d/framework start >/dev/null 2>&1 || true
    /etc/init.d/pmond start >/dev/null 2>&1 || true
    exit 0
fi

if [ -x "$SCRIPT_DIR/myts" ]; then
    exec "$SCRIPT_DIR/myts" "$@"
elif [ -x "$SCRIPT_DIR/launch_kindle.sh" ]; then
    exec "$SCRIPT_DIR/launch_kindle.sh" "$@"
fi

echo "[myts] Error: Could not locate myts launcher in $SCRIPT_DIR" >&2
exit 1
