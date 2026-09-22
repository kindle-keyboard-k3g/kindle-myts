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

TEST_MAIN_BEGIN()
    RUN_TEST(test_hardware_eink_driver_fallback);
    RUN_TEST(test_application_lifecycle);
    RUN_TEST(test_application_debug_mode);
TEST_MAIN_END()
