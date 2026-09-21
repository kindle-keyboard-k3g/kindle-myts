#include "tests/test_framework.h"
#include "terminal/ansi_parser.hpp"

using namespace myts::terminal;

struct MockTerminalHandler : public IAnsiHandler {
    int cursor_row{0};
    int cursor_col{0};
    bool display_erased{false};
    std::string printed_chars;

    void on_cursor_move(int row, int col) override {
        cursor_row = row;
        cursor_col = col;
    }

    void on_erase_display(int mode) override {
        (void)mode;
        display_erased = true;
    }

    void on_print_char(char c) override {
        printed_chars.push_back(c);
    }
};

int test_ansi_cursor_and_print(void)
{
    MockTerminalHandler handler;
    AnsiParser parser(handler);

    // Normal characters
    parser.feed("Hello");
    ASSERT_STR_EQ(handler.printed_chars.c_str(), "Hello");

    // Cursor position escape sequence: ESC [ 10 ; 20 H
    parser.feed("\033[10;20H");
    ASSERT_EQ(handler.cursor_row, 10);
    ASSERT_EQ(handler.cursor_col, 20);

    // Erase display sequence: ESC [ 2 J
    parser.feed("\033[2J");
    ASSERT_TRUE(handler.display_erased);

    return 0;
}

int test_ansi_split_sequence(void)
{
    MockTerminalHandler handler;
    AnsiParser parser(handler);

    // Escape sequence split across two feed chunks: "\033[5" then ";15H"
    parser.feed("\033[5");
    ASSERT_EQ(handler.cursor_row, 0); // Not yet executed

    parser.feed(";15H");
    ASSERT_EQ(handler.cursor_row, 5);
    ASSERT_EQ(handler.cursor_col, 15);

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_ansi_cursor_and_print);
    RUN_TEST(test_ansi_split_sequence);
TEST_MAIN_END()
