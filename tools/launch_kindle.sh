#!/bin/sh
# Launcher for kindle-myts on Kindle Keyboard (Kindle 3 / K3)
# Stops the Amazon Java framework before running and restarts it on exit.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

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

if [ $# -eq 0 ]; then
    echo "[myts] Launching myts-ng-kindle-dbg..."
    ./myts-ng-kindle-dbg --debug --metrics /dev/fb0 ter-u12n.hex
else
    echo "[myts] Executing: $@"
    "$@"
fi

exit_code=$?
echo "[myts] Exited with code $exit_code."
exit $exit_code
