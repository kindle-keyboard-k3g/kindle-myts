#include "tests/test_framework.h"
#include "graphics/eink_display.hpp"

using namespace myts::graphics;

class MockEinkDriver : public IEinkDriver {
public:
    int full_refresh_count{0};
    int partial_refresh_count{0};
    Rect last_partial_area{0, 0, 0, 0};

    bool update_display_full() override {
        ++full_refresh_count;
        return true;
    }

    bool update_display_area(const Rect& area) override {
        ++partial_refresh_count;
        last_partial_area = area;
        return true;
    }
};

int test_eink_display_dirty_tracking(void)
{
    MockEinkDriver driver;
    EinkDisplay display(600, 800, driver);

    // Initial state has no dirty region
    ASSERT_FALSE(display.has_dirty());

    // Mark dirty box: x=15 (odd), y=20, w=50, h=30
    display.mark_dirty(Rect{15, 20, 50, 30});
    ASSERT_TRUE(display.has_dirty());

    // Flush dirty area to driver
    display.flush();

    // Verify 4bpp alignment (even x start and even width)
    ASSERT_EQ(driver.partial_refresh_count, 1);
    ASSERT_EQ(driver.last_partial_area.x, 14); // 15 rounded down to even
    ASSERT_FALSE(display.has_dirty());

    return 0;
}

int test_eink_display_full_refresh_threshold(void)
{
    MockEinkDriver driver;
    EinkDisplay display(600, 800, driver);

    // Trigger full refresh
    display.refresh_full();
    ASSERT_EQ(driver.full_refresh_count, 1);

    return 0;
}

int test_anti_ghosting_keystroke_limit(void)
{
    MockEinkDriver driver;
    EinkDisplay display(600, 800, driver);

    // Configure refresh policy: escalate to full GC16 flash after 3 partial updates
    RefreshConfig cfg;
    cfg.partial_limit = 3;
    cfg.auto_full_refresh = true;
    display.set_refresh_config(cfg);

    ASSERT_EQ(display.partial_updates_count(), 0);

    // Update 1: partial
    display.mark_dirty(Rect{10, 10, 20, 20});
    display.flush();
    ASSERT_EQ(driver.partial_refresh_count, 1);
    ASSERT_EQ(driver.full_refresh_count, 0);
    ASSERT_EQ(display.partial_updates_count(), 1);

    // Update 2: partial
    display.mark_dirty(Rect{10, 10, 20, 20});
    display.flush();
    ASSERT_EQ(driver.partial_refresh_count, 2);
    ASSERT_EQ(driver.full_refresh_count, 0);
    ASSERT_EQ(display.partial_updates_count(), 2);

    // Update 3: reaches limit (3) -> escalates to full GC16 refresh
    display.mark_dirty(Rect{10, 10, 20, 20});
    display.flush();
    ASSERT_EQ(driver.partial_refresh_count, 2);
    ASSERT_EQ(driver.full_refresh_count, 1);
    // Counter resets after full refresh
    ASSERT_EQ(display.partial_updates_count(), 0);

    return 0;
}

int test_anti_ghosting_dirty_area_threshold(void)
{
    MockEinkDriver driver;
    EinkDisplay display(600, 800, driver);

    RefreshConfig cfg;
    cfg.partial_limit = 100;
    cfg.dirty_ratio_threshold = 0.5f;
    cfg.auto_full_refresh = true;
    display.set_refresh_config(cfg);

    // Small dirty area (< 50% screen): partial refresh
    display.mark_dirty(Rect{0, 0, 200, 200}); // 40,000 / 480,000 = 8.3%
    display.flush();
    ASSERT_EQ(driver.partial_refresh_count, 1);
    ASSERT_EQ(driver.full_refresh_count, 0);

    // Large dirty area (>= 50% screen): escalates to full GC16 refresh
    display.mark_dirty(Rect{0, 0, 600, 450}); // 270,000 / 480,000 = 56.25%
    display.flush();
    ASSERT_EQ(driver.partial_refresh_count, 1);
    ASSERT_EQ(driver.full_refresh_count, 1);
    ASSERT_EQ(display.partial_updates_count(), 0);

    return 0;
}

int test_manual_full_refresh_resets_counter(void)
{
    MockEinkDriver driver;
    EinkDisplay display(600, 800, driver);

    display.mark_dirty(Rect{10, 10, 20, 20});
    display.flush();
    ASSERT_EQ(display.partial_updates_count(), 1);

    display.refresh_full();
    ASSERT_EQ(driver.full_refresh_count, 1);
    ASSERT_EQ(display.partial_updates_count(), 0);

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_eink_display_dirty_tracking);
    RUN_TEST(test_eink_display_full_refresh_threshold);
    RUN_TEST(test_anti_ghosting_keystroke_limit);
    RUN_TEST(test_anti_ghosting_dirty_area_threshold);
    RUN_TEST(test_manual_full_refresh_resets_counter);
TEST_MAIN_END()
