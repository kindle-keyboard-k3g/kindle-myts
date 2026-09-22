#include "tests/test_framework.h"
#include "help/help_command_tracker.hpp"

using namespace myts::help;

static int test_tracker_exact_help_trigger() {
    HelpCommandTracker tracker;

    // Feed characters one by one
    ASSERT_FALSE(tracker.feed("h"));
    ASSERT_FALSE(tracker.feed("e"));
    ASSERT_FALSE(tracker.feed("l"));
    ASSERT_FALSE(tracker.feed("p"));
    ASSERT_TRUE(tracker.feed("\r"));

    // Verify buffer was reset after triggering
    ASSERT_TRUE(tracker.current_text().empty());

    // Feed entire string at once
    ASSERT_TRUE(tracker.feed("help\r"));
    ASSERT_TRUE(tracker.current_text().empty());

    // Newline \n should also work
    ASSERT_TRUE(tracker.feed("help\n"));

    return 0;
}

static int test_tracker_non_triggers() {
    HelpCommandTracker tracker;

    // "helper\r" -> false
    ASSERT_FALSE(tracker.feed("helper\r"));

    // "ahelp\r" -> false
    ASSERT_FALSE(tracker.feed("ahelp\r"));

    // "echo help\r" -> false
    ASSERT_FALSE(tracker.feed("echo help\r"));

    // Incomplete "hel\r" -> false
    ASSERT_FALSE(tracker.feed("hel\r"));

    // "help me\r" -> false
    ASSERT_FALSE(tracker.feed("help me\r"));

    return 0;
}

static int test_tracker_backspace_and_control_resets() {
    HelpCommandTracker tracker;

    // Type "helpx", backspace 'x', press enter -> triggers help!
    ASSERT_FALSE(tracker.feed("help"));
    ASSERT_FALSE(tracker.feed("x"));
    ASSERT_EQ(tracker.current_text().size(), 5);
    ASSERT_FALSE(tracker.feed("\x7f")); // Backspace / Del
    ASSERT_EQ(tracker.current_text().size(), 4);
    ASSERT_TRUE(tracker.feed("\r"));

    // Backspace on empty buffer
    tracker.reset();
    ASSERT_FALSE(tracker.feed("\x7f"));
    ASSERT_FALSE(tracker.feed("\b"));
    ASSERT_TRUE(tracker.current_text().empty());

    // Escape sequence resets tracker
    ASSERT_FALSE(tracker.feed("help"));
    ASSERT_FALSE(tracker.feed("\033[A")); // Up arrow ANSI sequence
    ASSERT_FALSE(tracker.feed("\r")); // Should NOT trigger

    // Ctrl+C (0x03) resets tracker
    ASSERT_FALSE(tracker.feed("help"));
    ASSERT_FALSE(tracker.feed("\x03"));
    ASSERT_FALSE(tracker.feed("\r"));

    // Ctrl+U (0x15) resets tracker
    ASSERT_FALSE(tracker.feed("help"));
    ASSERT_FALSE(tracker.feed("\x15"));
    ASSERT_FALSE(tracker.feed("\r"));

    return 0;
}

static int test_tracker_buffer_boundary() {
    HelpCommandTracker tracker;

    // Feed very long sequence
    const char* long_str = "abcdefghijklmnopqrstuvwxyz0123456789";
    ASSERT_FALSE(tracker.feed(long_str));
    ASSERT_FALSE(tracker.feed("\r"));
    ASSERT_TRUE(tracker.current_text().empty());

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_tracker_exact_help_trigger);
    RUN_TEST(test_tracker_non_triggers);
    RUN_TEST(test_tracker_backspace_and_control_resets);
    RUN_TEST(test_tracker_buffer_boundary);
TEST_MAIN_END()
