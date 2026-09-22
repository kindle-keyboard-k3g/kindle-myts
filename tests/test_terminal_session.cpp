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

static int test_dirty_rows_initial_all_dirty() {
    TerminalSession term(4, 10);
    auto dirty = term.take_dirty_rows();
    ASSERT_EQ(static_cast<int>(dirty.size()), 4);
    for (int r = 0; r < 4; ++r) {
        ASSERT_TRUE(dirty[r]);
    }
    // After take, everything should be clean
    auto clean = term.take_dirty_rows();
    for (int r = 0; r < 4; ++r) {
        ASSERT_FALSE(clean[r]);
    }
    return 0;
}

static int test_dirty_rows_print_char_marks_row() {
    TerminalSession term(4, 10);
    (void)term.take_dirty_rows(); // consume initial
    term.feed_input("A");
    auto dirty = term.take_dirty_rows();
    ASSERT_TRUE(dirty[0]);
    ASSERT_FALSE(dirty[1]);
    ASSERT_FALSE(dirty[2]);
    ASSERT_FALSE(dirty[3]);
    return 0;
}

static int test_dirty_rows_newline_marks_new_row() {
    TerminalSession term(4, 10);
    (void)term.take_dirty_rows();
    term.feed_input("A\r\nB");
    auto dirty = term.take_dirty_rows();
    ASSERT_TRUE(dirty[0]);
    ASSERT_TRUE(dirty[1]);
    ASSERT_FALSE(dirty[2]);
    return 0;
}

static int test_dirty_rows_erase_display_marks_all() {
    TerminalSession term(4, 10);
    (void)term.take_dirty_rows();
    term.feed_input("\033[2J");
    auto dirty = term.take_dirty_rows();
    for (int r = 0; r < 4; ++r) {
        ASSERT_TRUE(dirty[r]);
    }
    return 0;
}

static int test_dirty_rows_cursor_move_marks_prev_row() {
    TerminalSession term(4, 10);
    (void)term.take_dirty_rows();
    term.feed_input("A");         // cursor at row 0
    (void)term.take_dirty_rows();       // clear
    term.feed_input("\033[2;1H"); // move to row 1
    auto dirty = term.take_dirty_rows();
    ASSERT_TRUE(dirty[0]); // old cursor row marked
    ASSERT_TRUE(dirty[1]); // new cursor row marked
    return 0;
}

static int test_dirty_rows_scroll_marks_all() {
    TerminalSession term(3, 5);
    (void)term.take_dirty_rows();
    // Fill all rows then cause a scroll
    term.feed_input("1\r\n2\r\n3\r\n4"); // 4th line forces scroll
    auto dirty = term.take_dirty_rows();
    for (int r = 0; r < 3; ++r) {
        ASSERT_TRUE(dirty[r]);
    }
    return 0;
}

static int test_render_with_dirty_hint_skips_clean_rows() {
    TerminalSession term(4, 10);
    FontRenderer font(8, 12);
    const char* sample_hex =
        "0020:000000000000000000000000\n"
        "0041:0000003c66667e6666000000\n";
    ASSERT_TRUE(font.load_hex_data(sample_hex));

    term.feed_input("A");

    OwnedPixmap screen(80, 48);
    screen.clear(0xFF);

    std::vector<bool> dirty_hint(4, false);
    dirty_hint[0] = true; // only render row 0
    term.render(screen, font, /*show_cursor=*/false, &dirty_hint);

    // Row 0 should be rendered (pixel at x=2,y=3 is fg for 'A')
    ASSERT_EQ(screen.get_pixel(2, 3), 0x00);
    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_terminal_session_basic_typing);
    RUN_TEST(test_terminal_session_scrolling);
    RUN_TEST(test_terminal_session_ansi_cursor_and_clear);
    RUN_TEST(test_terminal_session_render);
    RUN_TEST(test_dirty_rows_initial_all_dirty);
    RUN_TEST(test_dirty_rows_print_char_marks_row);
    RUN_TEST(test_dirty_rows_newline_marks_new_row);
    RUN_TEST(test_dirty_rows_erase_display_marks_all);
    RUN_TEST(test_dirty_rows_cursor_move_marks_prev_row);
    RUN_TEST(test_dirty_rows_scroll_marks_all);
    RUN_TEST(test_render_with_dirty_hint_skips_clean_rows);
TEST_MAIN_END()
