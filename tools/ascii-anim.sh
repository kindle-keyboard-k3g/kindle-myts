#!/usr/bin/env sh
# Exercises dirty-rect partial updates, scroll, and cursor on kindle-myts PTY.
# Run inside the myts-ng shell (ssh or directly in the PTY).
# Uses only CUP (ESC[row;colH) and ED2 (ESC[2J) — the only CSI sequences
# supported by ansi_parser.hpp — so it works with the current parser.
#
# Usage: sh /mnt/us/myts/tools/ascii-anim.sh

DELAY=0.08
FRAMES=80
COLS=70  # stay within 600px / 8px cell width

# ---- Local-cell test: move a single '#' across one row (minimal dirty rect) ----
# Each frame diffs as exactly 2 glyph boxes once dirty tracking is fixed.
local_dot_test() {
    old_col=1
    i=1
    while [ "$i" -le "$FRAMES" ]; do
        new_col=$(( i % COLS + 1 ))
        printf '\033[12;%dH ' "$old_col"
        printf '\033[12;%dH#' "$new_col"
        old_col=$new_col
        i=$(( i + 1 ))
        sleep "$DELAY"
    done
}

# ---- Full-screen stress: redraws 24 lines per frame via ED2 + CUP ----
# Confirms ratio escalation threshold works correctly.
full_screen_test() {
    i=1
    while [ "$i" -le 30 ]; do
        printf '\033[2J\033[1;1H'
        printf "Frame %03d/%03d  e-ink dirty-rect stress test\n" "$i" 30

        spin='-\|/'
        c=$(( i % 4 + 1 ))
        sp=$(echo "$spin" | cut -c"$c")
        printf "  Spinner: %s\n" "$sp"

        filled=$(( i * COLS / 30 ))
        bar=""
        j=0
        while [ "$j" -lt "$COLS" ]; do
            if [ "$j" -lt "$filled" ]; then
                bar="${bar}#"
            else
                bar="${bar}."
            fi
            j=$(( j + 1 ))
        done
        printf "  [%s]\n" "$bar"

        r=4
        while [ "$r" -le 23 ]; do
            printf "  Line %02d: frame=%03d tick\n" "$r" "$i"
            r=$(( r + 1 ))
        done

        i=$(( i + 1 ))
        sleep 0.3
    done
}

# ---- Scroll test: print lines until terminal scrolls ----
scroll_test() {
    printf '\033[2J\033[1;1H'
    i=1
    while [ "$i" -le 60 ]; do
        printf "Scroll line %03d -- testing e-ink scroll dirty tracking\n" "$i"
        i=$(( i + 1 ))
        sleep "$DELAY"
    done
}

printf '\033[2J\033[1;1HStarting local-dot test (minimal dirty rect)...\n'
sleep 0.5
local_dot_test

printf '\033[2J\033[1;1HStarting full-screen stress test...\n'
sleep 0.5
full_screen_test

printf '\033[2J\033[1;1HStarting scroll test...\n'
sleep 0.5
scroll_test

printf '\033[2J\033[1;1HDone. Check partial vs full refresh counts in debug overlay.\n'
