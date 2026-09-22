#!/usr/bin/env sh
# Matrix-style digital rain animation for kindle-myts.
# Works in any POSIX / BusyBox /bin/sh shell without bash extensions.
#
# If a compiled binary 'matrix' exists in the script directory or PATH,
# it executes that binary for maximum 60fps performance with 0% CPU.
# Otherwise, it runs a pure POSIX shell rain loop.
#
# Usage:
#   sh tools/matrix-anim.sh [--frames N] [--delay MS]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Check if native compiled binary is available
if [ -x "$SCRIPT_DIR/matrix" ]; then
    exec "$SCRIPT_DIR/matrix" "$@"
fi

if [ -x "$SCRIPT_DIR/matrix-kindle" ]; then
    exec "$SCRIPT_DIR/matrix-kindle" "$@"
fi

# Fallback: Pure POSIX shell implementation
ROWS=24
COLS=70
FRAMES=100
DELAY=0.10

# Parse simple arguments
while [ $# -gt 0 ]; do
    case "$1" in
        --frames)
            FRAMES="$2"
            shift 2
            ;;
        --delay)
            # convert ms to fractional seconds if needed
            DELAY="0.$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

cleanup() {
    printf '\033[2J\033[1;1H\033[?25h'
    exit 0
}

trap cleanup INT TERM EXIT

# Set random characters pool using positional parameters (zero-fork access)
set -- 0 1 2 3 4 5 6 7 8 9 \
       A B C D E F G H I J K L M N O P Q R S T U V W X Y Z \
       a b c d e f g h i j k l m n o p q r s t u v w x y z \
       @ # $ % & * + - = < > : ; | ~ ! ?
NUM_CHARS=$#

# Clear screen and hide cursor
printf '\033[2J\033[1;1H\033[?25l'

# Initialize 15 rain streams
STREAMS=15
s=1
while [ "$s" -le "$STREAMS" ]; do
    eval "col_$s=\$(( (s * 4 + 3) % (COLS - 2) + 2 ))"
    eval "head_$s=\$(( s % 12 + 1 ))"
    eval "len_$s=\$(( s % 7 + 5 ))"
    eval "spd_$s=\$(( s % 2 + 1 ))"
    eval "tick_$s=0"
    s=$(( s + 1 ))
done

frame=1
seed=12345

while [ "$frame" -le "$FRAMES" ]; do
    s=1
    out=""
    while [ "$s" -le "$STREAMS" ]; do
        eval "t=\$tick_$s"
        eval "spd=\$spd_$s"
        t=$(( t + 1 ))
        if [ "$t" -ge "$spd" ]; then
            t=0
            eval "h=\$head_$s"
            eval "l=\$len_$s"
            eval "c=\$col_$s"

            # Advance head
            h=$(( h + 1 ))
            eval "head_$s=\$h"

            # PRNG
            seed=$(( (seed * 1103515245 + 12345) % 2147483647 ))
            if [ "$seed" -lt 0 ]; then seed=$(( -seed )); fi
            idx=$(( (seed % NUM_CHARS) + 1 ))
            eval "char=\${$idx}"

            # Print head
            if [ "$h" -ge 1 ] && [ "$h" -le "$ROWS" ]; then
                printf '\033[%d;%dH%s' "$h" "$c" "$char"
            fi

            # Erase tail
            tail=$(( h - l ))
            if [ "$tail" -ge 1 ] && [ "$tail" -le "$ROWS" ]; then
                printf '\033[%d;%dH ' "$tail" "$c"
            fi

            # Respawn when fallen off screen
            if [ "$tail" -gt "$ROWS" ]; then
                seed=$(( (seed * 1103515245 + 12345) % 2147483647 ))
                if [ "$seed" -lt 0 ]; then seed=$(( -seed )); fi
                new_col=$(( (seed % (COLS - 4)) + 2 ))
                new_len=$(( (seed % 8) + 5 ))
                eval "col_$s=\$new_col"
                eval "len_$s=\$new_len"
                eval "head_$s=1"
            fi
        fi
        eval "tick_$s=\$t"
        s=$(( s + 1 ))
    done

    frame=$(( frame + 1 ))
    sleep "$DELAY"
done
