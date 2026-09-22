#include "tests/test_framework.h"
#include "graphics/font_renderer.hpp"
#include "graphics/pixmap.hpp"

using namespace myts;
using namespace myts::graphics;

static int test_font_renderer_load_hex_string() {
    FontRenderer font(8, 12);

    // Sample hex font data for 'A' (0x41) and 'B' (0x42), 12 rows of 1 byte each (2 hex digits per row = 24 chars)
    const char* sample_hex =
        "0041:0000003c66667e6666000000\n"
        "0042:0000007c66667c66667c0000\n";

    bool loaded = font.load_hex_data(sample_hex);
    ASSERT_TRUE(loaded);
    ASSERT_EQ(font.glyph_height(), 12);
    ASSERT_EQ(font.glyph_width(), 8);
    ASSERT_TRUE(font.has_glyph(0x41));
    ASSERT_TRUE(font.has_glyph(0x42));
    ASSERT_FALSE(font.has_glyph(0x43));

    return 0;
}

static int test_font_renderer_draw_char() {
    FontRenderer font(8, 12);

    // Glyph 0x41 has row 3 (0-indexed) = 0x3c (00111100 in binary)
    // and row 0 = 0x00
    const char* sample_hex = "0041:0000003c66667e6666000000\n";
    ASSERT_TRUE(font.load_hex_data(sample_hex));

    OwnedPixmap canvas(16, 16);
    canvas.clear(0xFF); // White background

    // Draw 'A' at (0, 0), fg=0x0 (black), bg=0xF (white)
    bool drawn = font.draw_char(canvas, 0, 0, 0x41, 0x00, 0x0F);
    ASSERT_TRUE(drawn);

    // Check row 0: all bits are 0 -> should be bg (0xF)
    ASSERT_EQ(canvas.get_pixel(0, 0), 0x0F);
    ASSERT_EQ(canvas.get_pixel(1, 0), 0x0F);

    // Row 3 is 0x3c -> bits 2, 3, 4, 5 are set (0-indexed from left: 7-bit is col 0, 0-bit is col 7)
    // 0x3c = 0011 1100:
    // col 0 = 0 (bg: 0x0F)
    // col 1 = 0 (bg: 0x0F)
    // col 2 = 1 (fg: 0x00)
    // col 3 = 1 (fg: 0x00)
    // col 4 = 1 (fg: 0x00)
    // col 5 = 1 (fg: 0x00)
    // col 6 = 0 (bg: 0x0F)
    // col 7 = 0 (bg: 0x0F)
    ASSERT_EQ(canvas.get_pixel(0, 3), 0x0F);
    ASSERT_EQ(canvas.get_pixel(1, 3), 0x0F);
    ASSERT_EQ(canvas.get_pixel(2, 3), 0x00);
    ASSERT_EQ(canvas.get_pixel(3, 3), 0x00);
    ASSERT_EQ(canvas.get_pixel(4, 3), 0x00);
    ASSERT_EQ(canvas.get_pixel(5, 3), 0x00);
    ASSERT_EQ(canvas.get_pixel(6, 3), 0x0F);
    ASSERT_EQ(canvas.get_pixel(7, 3), 0x0F);

    return 0;
}

static int test_font_renderer_load_hex_file() {
    FontRenderer font(8, 12);
    bool loaded = font.load_hex_file("ter-u12n.hex");
    ASSERT_TRUE(loaded);

    // '!' is 0x21, 'A' is 0x41, '0' is 0x30
    ASSERT_TRUE(font.has_glyph(0x21));
    ASSERT_TRUE(font.has_glyph(0x30));
    ASSERT_TRUE(font.has_glyph(0x41));

    return 0;
}

static int test_font_renderer_draw_char_bold() {
    FontRenderer font(8, 12);

    // Glyph 0x41 row 3 = 0x3c (00111100)
    // Bold via row |= (row >> 1): 0x3c | 0x1e = 0x3e (00111110)
    // Col 1 becomes fg (was bg in normal render)
    const char* sample_hex = "0041:0000003c66667e6666000000\n";
    ASSERT_TRUE(font.load_hex_data(sample_hex));

    OwnedPixmap canvas(16, 16);
    canvas.clear(0xFF);

    bool drawn = font.draw_char_bold(canvas, 0, 0, 0x41, 0x00, 0x0F);
    ASSERT_TRUE(drawn);

    // Normal 'A' row 3 col 1 is bg (0x0F); bold should make it fg (0x00)
    // 0x3c = 00111100; bold = 0x3c | 0x1e = 0x3e = 00111110
    // col 1 (bit 6): 0x3e & 0x40 = 0 -> still bg; col 2 (bit 5): 0x3e & 0x20 = 1 -> fg
    // Actually: 0x3e = 0011 1110
    // col 0=0 (bg), col 1=0 (bg), col 2=1(fg), col 3=1(fg), col 4=1(fg), col 5=1(fg), col 6=1(fg), col 7=0(bg)
    // Bold adds col 6 which was bg in normal (0x3c col6=0 -> 0x3e col6=1)
    ASSERT_EQ(canvas.get_pixel(6, 3), 0x00); // col 6 is now fg in bold
    ASSERT_EQ(canvas.get_pixel(7, 3), 0x0F); // col 7 still bg
    ASSERT_EQ(canvas.get_pixel(2, 3), 0x00); // col 2 still fg

    // Missing glyph returns false
    bool not_drawn = font.draw_char_bold(canvas, 0, 0, 0x99, 0x00, 0x0F);
    ASSERT_FALSE(not_drawn);

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_font_renderer_load_hex_string);
    RUN_TEST(test_font_renderer_draw_char);
    RUN_TEST(test_font_renderer_load_hex_file);
    RUN_TEST(test_font_renderer_draw_char_bold);
TEST_MAIN_END()
