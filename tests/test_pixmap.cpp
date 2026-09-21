#include "tests/test_framework.h"
#include "graphics/geometry.hpp"
#include "graphics/pixmap.hpp"

using namespace myts::graphics;

int test_geometry_clipping(void)
{
    Rect screen(0, 0, 600, 800);
    ASSERT_FALSE(screen.is_empty());

    // Inside screen: unchanged
    Rect r1(50, 50, 100, 100);
    Rect c1 = r1.clip(screen);
    ASSERT_EQ(c1.x, 50);
    ASSERT_EQ(c1.y, 50);
    ASSERT_EQ(c1.width, 100);
    ASSERT_EQ(c1.height, 100);

    // Negative offset: clipped at 0 and width reduced
    Rect r2(-20, 10, 100, 50);
    Rect c2 = r2.clip(screen);
    ASSERT_EQ(c2.x, 0);
    ASSERT_EQ(c2.y, 10);
    ASSERT_EQ(c2.width, 80);
    ASSERT_EQ(c2.height, 50);

    // Overflowing right/bottom: clipped at screen bound
    Rect r3(550, 750, 100, 100);
    Rect c3 = r3.clip(screen);
    ASSERT_EQ(c3.x, 550);
    ASSERT_EQ(c3.y, 750);
    ASSERT_EQ(c3.width, 50);
    ASSERT_EQ(c3.height, 50);

    // Completely outside
    Rect r4(650, 10, 50, 50);
    Rect c4 = r4.clip(screen);
    ASSERT_TRUE(c4.is_empty());

    return 0;
}

int test_owned_pixmap_blit(void)
{
    OwnedPixmap dst(16, 16);
    ASSERT_EQ(dst.width(), 16);
    ASSERT_EQ(dst.height(), 16);
    ASSERT_EQ(dst.stride(), 8); // 16 pixels / 2 = 8 bytes per row

    // Fill dst with 0
    dst.clear(0x00);
    ASSERT_EQ(dst.data()[0], 0x00);

    // Create 4x4 src pixmap filled with 0xFF (white pixels)
    OwnedPixmap src(4, 4);
    src.clear(0xFF);

    // Blit into dst at (2, 2)
    ASSERT_TRUE(dst.blit(src.view(), Point(2, 2), false));

    // Verify blitted pixels (in row 2, offset 2, packed nibbles)
    // row 2 starts at index 2 * 8 = 16.
    // x = 2 is at byte index 16 + 1 = 17.
    ASSERT_EQ(dst.data()[17], 0xFF);

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_geometry_clipping);
    RUN_TEST(test_owned_pixmap_blit);
TEST_MAIN_END()
