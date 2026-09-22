#include "tests/test_framework.h"
#include "display/hardware_eink_driver.hpp"
#include "app/application.hpp"
#include <linux/input.h>

using namespace myts;
using namespace myts::display;
using namespace myts::app;

static int test_hardware_eink_driver_fallback() {
    HardwareEinkDriver driver(/*fb_path=*/"/dev/nonexistent_fb_test_device");

    ASSERT_FALSE(driver.is_hardware());
    ASSERT_EQ(driver.width(), 600);
    ASSERT_EQ(driver.height(), 800);
    ASSERT_NOT_NULL(driver.surface_data());
    ASSERT_EQ(driver.update_count(), 0);

    bool full_ok = driver.update_display_full();
    ASSERT_TRUE(full_ok);
    ASSERT_EQ(driver.update_count(), 1);

    bool partial_ok = driver.update_display_area(graphics::Rect(0, 0, 100, 100));
    ASSERT_TRUE(partial_ok);
    ASSERT_EQ(driver.update_count(), 2);

    return 0;
}

static int test_application_lifecycle() {
    Application app(/*fb_path=*/"/dev/nonexistent_fb_test_device",
                    /*font_path=*/"ter-u12n.hex",
                    /*rows=*/24, /*cols=*/80);

    ASSERT_TRUE(app.init());

    // Feed synthetic terminal output
    app.feed_terminal_output("Modern C++ kindle-myts initialized.\r\n");
    ASSERT_EQ(app.session().char_at(0, 0), 'M');
    ASSERT_EQ(app.session().char_at(0, 1), 'o');
    ASSERT_EQ(app.session().char_at(0, 2), 'd');

    // Trigger render frame
    app.render_frame();
    ASSERT_TRUE(app.driver().update_count() > 0);

    // Simulate user pressing 'q' (keycode 16)
    struct input_event ev{};
    ev.type = EV_KEY;
    ev.code = 16; // 'q'
    ev.value = 1; // press

    std::string_view key = app.handle_input_event(ev);
    ASSERT_STR_EQ(key.data(), "q");

    return 0;
}

static int test_application_debug_mode() {
    Application app(/*fb_path=*/"/dev/nonexistent_fb_test_device",
                    /*font_path=*/"ter-u12n.hex",
                    /*rows=*/24, /*cols=*/80);

    ASSERT_TRUE(app.init());

    DebugConfig cfg;
    cfg.enable_debug = true;
    cfg.enable_overlay = true;
    cfg.level = core::LogLevel::Debug;

    app.configure_debug(cfg);
    ASSERT_TRUE(app.is_debug_enabled());

    // Feed terminal output & render
    app.feed_terminal_output("Testing Debug Mode Telemetry\r\n");
    app.render_frame();

    const auto& metrics = app.metrics().snapshot();
    ASSERT_TRUE(metrics.partial_refreshes > 0 || metrics.full_refreshes > 0);

    return 0;
}

static int test_application_help_lifecycle() {
    Application app(/*fb_path=*/"/dev/nonexistent_fb_test_device",
                    /*font_path=*/"ter-u12n.hex",
                    /*rows=*/24, /*cols=*/80);
    ASSERT_TRUE(app.init());

    // Initially help is not active
    ASSERT_FALSE(app.is_help_active());

    // Press Menu key (code 139) -> opens help
    struct input_event ev_menu{};
    ev_menu.type = EV_KEY;
    ev_menu.code = 139;
    ev_menu.value = 1;
    app.handle_input_event(ev_menu);
    ASSERT_TRUE(app.is_help_active());

    size_t updates_during_help = app.driver().update_count();

    // While help is active, keys are consumed and not sent to PTY
    struct input_event ev_a{};
    ev_a.type = EV_KEY;
    ev_a.code = 30; // 'a'
    ev_a.value = 1;
    std::string_view res = app.handle_input_event(ev_a);
    ASSERT_TRUE(res.empty());
    ASSERT_TRUE(app.is_help_active());

    // Background terminal data buffers without redrawing canvas
    app.feed_terminal_output("bg text\r\n");
    ASSERT_EQ(app.session().char_at(0, 0), 'b');

    // Exit help by pressing 'q' (code 16)
    struct input_event ev_q{};
    ev_q.type = EV_KEY;
    ev_q.code = 16;
    ev_q.value = 1;
    app.handle_input_event(ev_q);
    ASSERT_FALSE(app.is_help_active());

    // Full screen refresh triggered on exit
    ASSERT_TRUE(app.driver().update_count() > updates_during_help);

    return 0;
}

static int test_application_help_command_trigger() {
    Application app(/*fb_path=*/"/dev/nonexistent_fb_test_device",
                    /*font_path=*/"ter-u12n.hex",
                    /*rows=*/24, /*cols=*/80);
    ASSERT_TRUE(app.init());

    ASSERT_FALSE(app.is_help_active());

    // Type "help\r" using keycodes:
    // 'h' = 35, 'e' = 18, 'l' = 38, 'p' = 25, Enter = 28
    uint16_t keys[] = {35, 18, 38, 25, 28};
    for (uint16_t k : keys) {
        struct input_event ev{};
        ev.type = EV_KEY;
        ev.code = k;
        ev.value = 1;
        app.handle_input_event(ev);
        ev.value = 0; // release
        app.handle_input_event(ev);
    }

    ASSERT_TRUE(app.is_help_active());

    // Exit via Back key (158)
    struct input_event ev_back{};
    ev_back.type = EV_KEY;
    ev_back.code = 158;
    ev_back.value = 1;
    app.handle_input_event(ev_back);
    ASSERT_FALSE(app.is_help_active());

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_hardware_eink_driver_fallback);
    RUN_TEST(test_application_lifecycle);
    RUN_TEST(test_application_debug_mode);
    RUN_TEST(test_application_help_lifecycle);
    RUN_TEST(test_application_help_command_trigger);
TEST_MAIN_END()
