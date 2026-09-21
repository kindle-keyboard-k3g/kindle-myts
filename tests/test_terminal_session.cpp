#include "tests/test_framework.h"
#include "terminal/terminal_session.hpp"
#include "graphics/font_renderer.hpp"
#include "graphics/pixmap.hpp"

using namespace myts;
using namespace myts::terminal;
using namespace myts::graphics;

static int test_terminal_session_basic_typing() {
    TerminalSession term(4, 10);

    term.feed_input("Hello\r\nWorld");

    ASSERT_EQ(term.cursor_row(), 1);
    ASSERT_EQ(term.cursor_col(), 5);

    ASSERT_EQ(term.char_at(0, 0), 'H');
    ASSERT_EQ(term.char_at(0, 1), 'e');
    ASSERT_EQ(term.char_at(0, 2), 'l');
    ASSERT_EQ(term.char_at(0, 3), 'l');
    ASSERT_EQ(term.char_at(0, 4), 'o');
    ASSERT_EQ(term.char_at(0, 5), ' ');

    ASSERT_EQ(term.char_at(1, 0), 'W');
    ASSERT_EQ(term.char_at(1, 1), 'o');
    ASSERT_EQ(term.char_at(1, 2), 'r');
    ASSERT_EQ(term.char_at(1, 3), 'l');
    ASSERT_EQ(term.char_at(1, 4), 'd');

    return 0;
}

static int test_terminal_session_scrolling() {
    TerminalSession term(3, 5); // 3 rows, 5 cols

    term.feed_input("1\r\n2\r\n3\r\n4");

    // Row 0 was "1", then scrolled:
    // After "2\r\n": row 0="1", row 1="2"
    // After "3\r\n": row 0="2", row 1="3", row 2 empty
    // After "4": row 2="4"
    ASSERT_EQ(term.char_at(0, 0), '2');
    ASSERT_EQ(term.char_at(1, 0), '3');
    ASSERT_EQ(term.char_at(2, 0), '4');
    ASSERT_EQ(term.cursor_row(), 2);
    ASSERT_EQ(term.cursor_col(), 1);

    return 0;
}

static int test_terminal_session_ansi_cursor_and_clear() {
    TerminalSession term(4, 10);

    term.feed_input("ABCDE");
    ASSERT_EQ(term.char_at(0, 0), 'A');
    ASSERT_EQ(term.char_at(0, 1), 'B');
    ASSERT_EQ(term.char_at(0, 2), 'C');

    // ANSI CUP: 1-indexed row 1, col 2 -> (0, 1) in 0-indexed coordinates
    term.feed_input("\033[1;2HZ");
    ASSERT_EQ(term.char_at(0, 1), 'Z');
    ASSERT_EQ(term.char_at(0, 2), 'C');

    // ANSI ED: 2 = erase entire screen
    term.feed_input("\033[2J");
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 10; ++c) {
            ASSERT_EQ(term.char_at(r, c), ' ');
        }
    }

    return 0;
}

static int test_terminal_session_render() {
    TerminalSession term(4, 10);
    FontRenderer font(8, 12);

    const char* sample_hex =
        "0020:000000000000000000000000\n"
        "0041:0000003c66667e6666000000\n";
    ASSERT_TRUE(font.load_hex_data(sample_hex));

    term.feed_input("A");

    OwnedPixmap screen(80, 48); // 10 cols * 8px, 4 rows * 12px
    screen.clear(0xFF);

    term.render(screen, font, /*show_cursor=*/false);

    // Character 'A' is at (0, 0). Check row 3 (0-indexed) at col 2: should be fg (0x00)
    ASSERT_EQ(screen.get_pixel(2, 3), 0x00);
    ASSERT_EQ(screen.get_pixel(3, 3), 0x00);
    ASSERT_EQ(screen.get_pixel(0, 3), 0x0F);

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_terminal_session_basic_typing);
    RUN_TEST(test_terminal_session_scrolling);
    RUN_TEST(test_terminal_session_ansi_cursor_and_clear);
    RUN_TEST(test_terminal_session_render);
TEST_MAIN_END()
