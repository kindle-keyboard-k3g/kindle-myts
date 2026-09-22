#include "tests/test_framework.h"
#include "help/help_renderer.hpp"
#include "graphics/pixmap.hpp"
#include "graphics/font_renderer.hpp"

using namespace myts;
using namespace myts::graphics;
using namespace myts::help;

static const char* kMockHexFont =
    "0020:000000000000000000000000\n" // space
    "0021:001010101010001000000000\n" // '!'
    "0028:000810202020100800000000\n" // '('
    "0029:002010080808102000000000\n" // ')'
    "0031:001030101010103800000000\n" // '1'
    "0032:003844040810207c00000000\n" // '2'
    "0033:003844041804443800000000\n" // '3'
    "0034:00081828487c080800000000\n" // '4'
    "0041:003844447c44444400000000\n" // 'A'
    "004f:003844444444443800000000\n" // 'O'
    "0051:00384444444c443a00000000\n" // 'Q'
    "005b:003c20202020203c00000000\n" // '['
    "005d:003c04040404043c00000000\n"; // ']'

static int test_help_renderer_full_pages() {
    OwnedPixmap canvas(600, 800);
    FontRenderer font(8, 12);
    ASSERT_TRUE(font.load_hex_data(kMockHexFont));

    HelpRenderer renderer;
    HelpNavigationState state{};

    // Test Overview page (0)
    state.page = HelpPage::Overview;
    Rect r0 = renderer.render_full(canvas, font, state);
    ASSERT_EQ(r0.x, 0);
    ASSERT_EQ(r0.y, 0);
    ASSERT_EQ(r0.width, 600);
    ASSERT_EQ(r0.height, 800);

    // Test Keypad page (1)
    state.page = HelpPage::Keypad;
    state.last_key = KeySnapshot{16, 'Q', 0}; // Q pressed
    Rect r1 = renderer.render_full(canvas, font, state);
    ASSERT_EQ(r1.width, 600);
    ASSERT_EQ(r1.height, 800);

    // Test Sym page (2)
    state.page = HelpPage::Sym;
    Rect r2 = renderer.render_full(canvas, font, state);
    ASSERT_EQ(r2.width, 600);
    ASSERT_EQ(r2.height, 800);

    // Test Fn page (3)
    state.page = HelpPage::Fn;
    Rect r3 = renderer.render_full(canvas, font, state);
    ASSERT_EQ(r3.width, 600);
    ASSERT_EQ(r3.height, 800);

    // Verify canvas has non-white pixels
    bool has_dark_pixels = false;
    for (int y = 0; y < 800; ++y) {
        for (int x = 0; x < 600; ++x) {
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

static int test_help_renderer_delta() {
    OwnedPixmap canvas(600, 800);
    FontRenderer font(8, 12);
    ASSERT_TRUE(font.load_hex_data(kMockHexFont));

    HelpRenderer renderer;
    HelpNavigationState state{};
    state.page = HelpPage::Keypad;
    renderer.render_full(canvas, font, state);

    // Delta update with a new key
    state.last_key = KeySnapshot{30, 'A', 0};
    Rect dirty = renderer.render_delta(canvas, font, state);

    // Delta dirty area must be smaller than full canvas
    ASSERT_TRUE(dirty.width > 0);
    ASSERT_TRUE(dirty.height > 0);
    ASSERT_TRUE(dirty.right() <= 600);
    ASSERT_TRUE(dirty.bottom() <= 800);

    return 0;
}

static int test_help_renderer_clipping_bounds() {
    OwnedPixmap tiny_canvas(50, 40);
    FontRenderer font(8, 12);
    ASSERT_TRUE(font.load_hex_data(kMockHexFont));

    HelpRenderer renderer;
    HelpNavigationState state{};

    Rect r = renderer.render_full(tiny_canvas, font, state);
    ASSERT_TRUE(r.right() <= tiny_canvas.width());
    ASSERT_TRUE(r.bottom() <= tiny_canvas.height());

    Rect delta = renderer.render_delta(tiny_canvas, font, state);
    ASSERT_TRUE(delta.right() <= tiny_canvas.width());
    ASSERT_TRUE(delta.bottom() <= tiny_canvas.height());

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_help_renderer_full_pages);
    RUN_TEST(test_help_renderer_delta);
    RUN_TEST(test_help_renderer_clipping_bounds);
TEST_MAIN_END()
