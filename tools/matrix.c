/**
 * @file matrix.c
 * @brief Matrix-style digital rain animation tuned for Kindle E-Ink displays.
 *
 * Uses only standard ANSI VT100 sequences (CUP: \033[row;colH, ED2: \033[2J)
 * compatible with kindle-myts. Buffers entire frames to minimize PTY reads
 * and leverage dirty-row tracking for fast partial e-ink updates.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include <sys/select.h>

#define DEFAULT_ROWS 24
#define DEFAULT_COLS 70
#define MAX_DROPS 35
#define DEFAULT_DELAY_MS 100

static volatile sig_atomic_t g_running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    g_running = 0;
}

typedef struct {
    int col;      // 1-indexed column
    int head_y;   // Current row of rain head
    int len;      // Length of rain trail
    int speed;    // Move every N ticks (1 = fast, 2 = medium, 3 = slow)
    int tick;     // Tick counter
} Drop;

static const char GLYPHS[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "@#$%&*+-=<>:;|~!?";

#define NUM_GLYPHS (sizeof(GLYPHS) - 1)

static inline char random_glyph(void) {
    return GLYPHS[rand() % NUM_GLYPHS];
}

static void init_drop(Drop* d, int rows, int cols) {
    d->col = 1 + (rand() % cols);
    d->head_y = -(rand() % 10); // Start slightly above screen for staggered arrival
    d->len = 4 + (rand() % 10); // Trail length 4..13
    d->speed = 1 + (rand() % 2); // 1 or 2
    d->tick = 0;
}

static int check_keypress(void) {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == 'q' || c == 'Q' || c == 27 || c == 3) {
                return 1;
            }
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    int rows = DEFAULT_ROWS;
    int cols = DEFAULT_COLS;
    int delay_ms = DEFAULT_DELAY_MS;
    int max_frames = -1; // -1 = infinite until 'q' or signal

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--rows") == 0 && i + 1 < argc) {
            rows = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--cols") == 0 && i + 1 < argc) {
            cols = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--delay") == 0 && i + 1 < argc) {
            delay_ms = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            max_frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [--rows N] [--cols N] [--delay MS] [--frames N]\n", argv[0]);
            printf("  --rows N     Terminal rows (default: %d)\n", DEFAULT_ROWS);
            printf("  --cols N     Terminal columns (default: %d)\n", DEFAULT_COLS);
            printf("  --delay MS   Frame delay in ms (default: %d)\n", DEFAULT_DELAY_MS);
            printf("  --frames N   Max frames to run (default: infinite)\n");
            return 0;
        }
    }

    if (rows < 5) rows = 5;
    if (cols < 10) cols = 10;

    // Seed PRNG
    srand((unsigned int)time(NULL));

    // Trap signals for clean exit
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Save terminal settings and set non-canonical raw mode
    struct termios orig_termios, raw_termios;
    int is_tty = isatty(STDIN_FILENO);
    if (is_tty) {
        tcgetattr(STDIN_FILENO, &orig_termios);
        raw_termios = orig_termios;
        raw_termios.c_lflag &= ~(ICANON | ECHO);
        raw_termios.c_cc[VMIN] = 0;
        raw_termios.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios);
    }

    // Set large frame output buffer (8 KB) to send complete frames at once
    char frame_buf[8192];
    setvbuf(stdout, frame_buf, _IOFBF, sizeof(frame_buf));

    // Clear screen and hide cursor
    printf("\033[2J\033[1;1H\033[?25l");
    fflush(stdout);

    // Initialize rain drops
    Drop drops[MAX_DROPS];
    for (int i = 0; i < MAX_DROPS; ++i) {
        init_drop(&drops[i], rows, cols);
        // Pre-advance some drops so the rain starts populated
        drops[i].head_y = rand() % rows;
    }

    int frame_count = 0;
    while (g_running) {
        if (max_frames > 0 && frame_count >= max_frames) {
            break;
        }

        if (is_tty && check_keypress()) {
            break;
        }

        // Update and draw each drop
        for (int i = 0; i < MAX_DROPS; ++i) {
            Drop* d = &drops[i];
            d->tick++;
            if (d->tick < d->speed) {
                continue;
            }
            d->tick = 0;

            // 1. Advance head
            d->head_y++;

            // If head is on screen, draw random leading character
            if (d->head_y >= 1 && d->head_y <= rows) {
                printf("\033[%d;%dH%c", d->head_y, d->col, random_glyph());
            }

            // 2. Occasionally mutate a character inside the falling trail
            if (d->head_y > 1 && (rand() % 3 == 0)) {
                int trail_row = d->head_y - 1 - (rand() % (d->len > 1 ? d->len - 1 : 1));
                if (trail_row >= 1 && trail_row <= rows) {
                    printf("\033[%d;%dH%c", trail_row, d->col, random_glyph());
                }
            }

            // 3. Erase tail
            int tail_y = d->head_y - d->len;
            if (tail_y >= 1 && tail_y <= rows) {
                printf("\033[%d;%dH ", tail_y, d->col);
            }

            // 4. Respawn drop when entire trail falls off the bottom
            if (tail_y > rows) {
                init_drop(d, rows, cols);
            }
        }

        // Flush entire frame buffer to terminal in one go
        fflush(stdout);

        frame_count++;
        usleep(delay_ms * 1000);
    }

    // Clean exit: clear screen, show cursor, restore terminal
    printf("\033[2J\033[1;1H\033[?25h");
    fflush(stdout);

    if (is_tty) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    }

    return 0;
}
