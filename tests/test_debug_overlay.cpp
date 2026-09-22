/**
 * @file test_debug_overlay.cpp
 * @brief Unit tests for visual debug overlay renderer (graphics/debug_overlay.hpp)
 */

#include "test_framework.h"
#include "graphics/debug_overlay.hpp"
#include "graphics/pixmap.hpp"
#include "graphics/font_renderer.hpp"
#include "core/metrics.hpp"

using namespace myts::graphics;
using namespace myts::core;

static const char* kMockHexFont =
    "0020:000000000000000000000000\n" // space
    "0041:003844447c44444400000000\n" // 'A'
    "0030:0038444c5464443800000000\n" // '0'
    "0031:001030101010103800000000\n"; // '1'

static int test_debug_overlay_render_banner() {
    OwnedPixmap canvas(600, 800);
    canvas.clear(0xFF); // White background

    FontRenderer font(8, 12);
    ASSERT_TRUE(font.load_hex_data(kMockHexFont));

    MetricsCollector metrics;
    metrics.record_refresh(Rect{0, 0, 100, 50}, false);
    metrics.record_pty_read(10);
    metrics.record_input_event();

    DebugOverlay overlay;
    Rect dirty = overlay.render(canvas, font, metrics.snapshot(), 0, 0);

    // Banner should be at top-left, height = font.glyph_height()
    ASSERT_EQ(dirty.x, 0);
    ASSERT_EQ(dirty.y, 0);
    ASSERT_EQ(dirty.height, 12);
    ASSERT_TRUE(dirty.width > 0);

    // Pixels inside overlay banner should be modified (not all white)
    bool has_dark_pixels = false;
    for (int y = 0; y < 12; ++y) {
        for (int x = 0; x < dirty.width; ++x) {
            if (canvas.get_pixel(x, y) < 0x0F) {
                has_dark_pixels = true;
                break;
            }
        }
        if (has_dark_pixels) break;
    }
    ASSERT_TRUE(has_dark_pixels);
    return 0;
}

static int test_debug_overlay_clipping_bounds() {
    OwnedPixmap small_canvas(30, 20);
    small_canvas.clear(0xFF);

    FontRenderer font(8, 12);
    ASSERT_TRUE(font.load_hex_data(kMockHexFont));

    MetricsCollector metrics;
    DebugOverlay overlay;

    // Render near canvas edge
    Rect dirty = overlay.render(small_canvas, font, metrics.snapshot(), 10, 10);

    ASSERT_EQ(dirty.x, 10);
    ASSERT_EQ(dirty.y, 10);
    ASSERT_TRUE(dirty.right() <= small_canvas.width());
    ASSERT_TRUE(dirty.bottom() <= small_canvas.height());
    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_debug_overlay_render_banner);
    RUN_TEST(test_debug_overlay_clipping_bounds);
TEST_MAIN_END()
