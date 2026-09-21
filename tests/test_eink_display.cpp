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

TEST_MAIN_BEGIN()
    RUN_TEST(test_eink_display_dirty_tracking);
    RUN_TEST(test_eink_display_full_refresh_threshold);
TEST_MAIN_END()
