#include "tests/test_framework.h"
#include "input/input_manager.hpp"
#include <linux/input.h>

using namespace myts;
using namespace myts::input;

static struct input_event make_event(uint16_t type, uint16_t code, int32_t value) {
    struct input_event ev{};
    ev.type = type;
    ev.code = code;
    ev.value = value;
    return ev;
}

static int test_input_manager_letter_and_shift() {
    InputManager im;

    // Press 'a' (code 30)
    std::string_view s1 = im.process_event(make_event(EV_KEY, 30, 1));
    ASSERT_STR_EQ(s1.data(), "a");

    // Release 'a' (value 0)
    std::string_view s_rel = im.process_event(make_event(EV_KEY, 30, 0));
    ASSERT_TRUE(s_rel.empty());

    // Press Shift (code 42)
    im.process_event(make_event(EV_KEY, 42, 1));

    // Press 'a' with Shift held -> 'A'
    std::string_view s2 = im.process_event(make_event(EV_KEY, 30, 1));
    ASSERT_STR_EQ(s2.data(), "A");

    // Release Shift
    im.process_event(make_event(EV_KEY, 42, 0));

    // Press 'a' -> back to 'a'
    std::string_view s3 = im.process_event(make_event(EV_KEY, 30, 1));
    ASSERT_STR_EQ(s3.data(), "a");

    return 0;
}

static int test_input_manager_control_chars() {
    InputManager im;

    // Press Ctrl (code 29)
    im.process_event(make_event(EV_KEY, 29, 1));

    // Press 'c' (code 46) -> Ctrl+C = 0x03
    std::string_view s = im.process_event(make_event(EV_KEY, 46, 1));
    ASSERT_EQ(s.size(), 1);
    ASSERT_EQ(s[0], '\x03');

    // Release Ctrl
    im.process_event(make_event(EV_KEY, 29, 0));

    return 0;
}

static int test_input_manager_arrows_and_enter() {
    InputManager im;

    // Enter (28) -> \r
    std::string_view s_enter = im.process_event(make_event(EV_KEY, 28, 1));
    ASSERT_STR_EQ(s_enter.data(), "\r");

    // Up (103) -> \033[A
    std::string_view s_up = im.process_event(make_event(EV_KEY, 103, 1));
    ASSERT_STR_EQ(s_up.data(), "\033[A");

    // Down (108) -> \033[B
    std::string_view s_down = im.process_event(make_event(EV_KEY, 108, 1));
    ASSERT_STR_EQ(s_down.data(), "\033[B");

    // Left (105) -> \033[D
    std::string_view s_left = im.process_event(make_event(EV_KEY, 105, 1));
    ASSERT_STR_EQ(s_left.data(), "\033[D");

    // Right (106) -> \033[C
    std::string_view s_right = im.process_event(make_event(EV_KEY, 106, 1));
    ASSERT_STR_EQ(s_right.data(), "\033[C");

    // Shift + Up -> \033[5~ (PageUp)
    im.process_event(make_event(EV_KEY, 42, 1));
    std::string_view s_pgup = im.process_event(make_event(EV_KEY, 103, 1));
    ASSERT_STR_EQ(s_pgup.data(), "\033[5~");

    // Shift + Down -> \033[6~ (PageDown)
    std::string_view s_pgdown = im.process_event(make_event(EV_KEY, 108, 1));
    ASSERT_STR_EQ(s_pgdown.data(), "\033[6~");

    return 0;
}

static int test_input_manager_del_and_space() {
    InputManager im;

    // Del / Backspace (14) -> \x7f
    std::string_view s_del = im.process_event(make_event(EV_KEY, 14, 1));
    ASSERT_EQ(s_del.size(), 1);
    ASSERT_EQ(s_del[0], '\x7f');

    // Space (57) -> " "
    std::string_view s_sp = im.process_event(make_event(EV_KEY, 57, 1));
    ASSERT_STR_EQ(s_sp.data(), " ");

    return 0;
}

static int test_input_manager_kindle_modifiers() {
    InputManager im;

    // Press Kindle 3 aA key (190) -> sets ctrl modifier
    im.process_event(make_event(EV_KEY, 190, 1));
    ASSERT_TRUE(im.modifiers().ctrl);

    // Press 'c' (46) with aA active -> emits Ctrl+C (\x03)
    std::string_view s_ctrl_c = im.process_event(make_event(EV_KEY, 46, 1));
    ASSERT_EQ(s_ctrl_c.size(), 1);
    ASSERT_EQ(s_ctrl_c[0], '\x03');

    // Release Kindle 3 aA key (190)
    im.process_event(make_event(EV_KEY, 190, 0));
    ASSERT_FALSE(im.modifiers().ctrl);

    // Press Kindle DX aA key (90) -> sets ctrl modifier
    im.process_event(make_event(EV_KEY, 90, 1));
    ASSERT_TRUE(im.modifiers().ctrl);
    im.process_event(make_event(EV_KEY, 90, 0));
    ASSERT_FALSE(im.modifiers().ctrl);

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_input_manager_letter_and_shift);
    RUN_TEST(test_input_manager_control_chars);
    RUN_TEST(test_input_manager_arrows_and_enter);
    RUN_TEST(test_input_manager_del_and_space);
    RUN_TEST(test_input_manager_kindle_modifiers);
TEST_MAIN_END()
