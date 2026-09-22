#ifndef MYTS_HELP_HELP_RENDERER_HPP
#define MYTS_HELP_HELP_RENDERER_HPP

#include "graphics/font_renderer.hpp"
#include "graphics/geometry.hpp"
#include "graphics/pixmap.hpp"
#include "help/help_key_catalog.hpp"
#include "help/help_types.hpp"
#include <algorithm>
#include <string_view>

namespace myts {
namespace help {

/**
 * @brief Stateless rasterizer for interactive help screen and keyboard layout.
 */
class HelpRenderer {
public:
    constexpr HelpRenderer() noexcept = default;

    graphics::Rect render_full(graphics::OwnedPixmap& canvas,
                              const graphics::FontRenderer& font,
                              const HelpNavigationState& state) const noexcept {
        if (canvas.width() <= 0 || canvas.height() <= 0) {
            return graphics::Rect(0, 0, 0, 0);
        }
        canvas.clear(0xFF);
        int font_h = font.glyph_height();
        draw_tabs(canvas, font, state.page);
        draw_hline(canvas, font_h + 4);
        draw_page_content(canvas, font, state);
        int status_y = canvas.height() - font_h - 4;
        draw_hline(canvas, status_y - 2);
        draw_status_bar(canvas, font, state, status_y);
        return graphics::Rect(0, 0, canvas.width(), canvas.height());
    }

    graphics::Rect render_delta(graphics::OwnedPixmap& canvas,
                               const graphics::FontRenderer& font,
                               const HelpNavigationState& state) const noexcept {
        if (canvas.width() <= 0 || canvas.height() <= 0) {
            return graphics::Rect(0, 0, 0, 0);
        }
        int font_h = font.glyph_height();
        int status_y = canvas.height() - font_h - 4;
        clear_rect(canvas, 0, status_y, canvas.width(), font_h + 4);
        draw_status_bar(canvas, font, state, status_y);
        int dirty_h = canvas.height() - (status_y - 2);
        return graphics::Rect(0, status_y - 2, canvas.width(), dirty_h);
    }

private:
    static void draw_string(graphics::OwnedPixmap& canvas, const graphics::FontRenderer& font,
                           int x, int y, std::string_view str,
                           uint8_t fg = 0x00, uint8_t bg = 0x0F) noexcept {
        int cur_x = x;
        int font_w = font.glyph_width();
        for (char c : str) {
            if (cur_x + font_w > canvas.width()) break;
            font.draw_char(canvas, cur_x, y, static_cast<uint8_t>(c), fg, bg);
            cur_x += font_w;
        }
    }

    static void draw_hline(graphics::OwnedPixmap& canvas, int y, uint8_t color = 0x00) noexcept {
        if (y < 0 || y >= canvas.height()) return;
        for (int x = 0; x < canvas.width(); ++x) {
            canvas.set_pixel(x, y, color);
        }
    }

    static void clear_rect(graphics::OwnedPixmap& canvas, int rx, int ry, int rw, int rh) noexcept {
        int max_x = std::min(canvas.width(), rx + rw);
        int max_y = std::min(canvas.height(), ry + rh);
        for (int y = std::max(0, ry); y < max_y; ++y) {
            for (int x = std::max(0, rx); x < max_x; ++x) {
                canvas.set_pixel(x, y, 0xFF);
            }
        }
    }

    static void draw_tabs(graphics::OwnedPixmap& canvas, const graphics::FontRenderer& font,
                         HelpPage current) noexcept {
        int font_w = font.glyph_width();
        draw_single_tab(canvas, font, 8, 2, "[1] Overview", current == HelpPage::Overview);
        draw_single_tab(canvas, font, 8 + font_w * 15, 2, "[2] Keypad", current == HelpPage::Keypad);
        draw_single_tab(canvas, font, 8 + font_w * 28, 2, "[3] Sym", current == HelpPage::Sym);
        draw_single_tab(canvas, font, 8 + font_w * 38, 2, "[4] Fn", current == HelpPage::Fn);
    }

    static void draw_single_tab(graphics::OwnedPixmap& canvas, const graphics::FontRenderer& font,
                               int x, int y, std::string_view label, bool selected) noexcept {
        uint8_t fg = selected ? 0x0F : 0x00;
        uint8_t bg = selected ? 0x00 : 0x0F;
        draw_string(canvas, font, x, y, label, fg, bg);
    }

    static void draw_page_content(graphics::OwnedPixmap& canvas,
                                 const graphics::FontRenderer& font,
                                 const HelpNavigationState& state) noexcept {
        int start_y = font.glyph_height() + 10;
        if (state.page == HelpPage::Overview) {
            draw_overview(canvas, font, start_y);
            return;
        }
        if (state.page == HelpPage::Keypad) {
            draw_keypad(canvas, font, start_y, state.last_key);
            return;
        }
        if (state.page == HelpPage::Sym) {
            draw_sym_table(canvas, font, start_y);
            return;
        }
        draw_fn_table(canvas, font, start_y);
    }

    static void draw_overview(graphics::OwnedPixmap& canvas, const graphics::FontRenderer& font,
                             int y) noexcept {
        int font_h = font.glyph_height();
        draw_string(canvas, font, 10, y, "=== KINDLE TERMINAL NAVIGATION & SHORTCUTS ===");
        draw_string(canvas, font, 10, y + font_h * 2, "Menu or Shift+H  : Open this Help screen");
        draw_string(canvas, font, 10, y + font_h * 3, "Type 'help' + Ret: Open this Help screen");
        draw_string(canvas, font, 10, y + font_h * 4, "1..4 / D-Pad L/R : Switch tabs (Keypad, Sym, Fn)");
        draw_string(canvas, font, 10, y + font_h * 5, "Back / Right> / q: Exit Help (Restore Terminal)");
        draw_string(canvas, font, 10, y + font_h * 7, "=== MODIFIER KEYS ===");
        draw_string(canvas, font, 10, y + font_h * 8, "aA               : Ctrl key (e.g. aA + C = Ctrl+C)");
        draw_string(canvas, font, 10, y + font_h * 9, "Sym              : Punctuation / symbols layer");
        draw_string(canvas, font, 10, y + font_h * 10, "Shift            : Uppercase & shift digits");
        draw_string(canvas, font, 10, y + font_h * 12, "=== HARDWARE BUTTONS ===");
        draw_string(canvas, font, 10, y + font_h * 13, "Right< (Page Fwd): Page Up / Forward");
        draw_string(canvas, font, 10, y + font_h * 14, "Right> (Page Turn): Escape / Exit");
        draw_string(canvas, font, 10, y + font_h * 15, "5-Way Center     : Return / Select");
    }

    static void draw_keypad(graphics::OwnedPixmap& canvas, const graphics::FontRenderer& font,
                           int y, const KeySnapshot& last_key) noexcept {
        int font_h = font.glyph_height();
        draw_string(canvas, font, 10, y, "=== PHYSICAL KEYBOARD MATRIX ===");
        draw_keypad_row(canvas, font, 10, y + font_h * 2, HelpKeyCatalog::ROW1, HelpKeyCatalog::ROW1_COUNT, last_key);
        draw_keypad_row(canvas, font, 10, y + font_h * 4, HelpKeyCatalog::ROW2, HelpKeyCatalog::ROW2_COUNT, last_key);
        draw_keypad_row(canvas, font, 10, y + font_h * 6, HelpKeyCatalog::ROW3, HelpKeyCatalog::ROW3_COUNT, last_key);
        draw_string(canvas, font, 10, y + font_h * 8, "[SHIFT] [CTRL]   [      SPACE      ]   [SYM] [MENU]");
    }

    static void draw_keypad_row(graphics::OwnedPixmap& canvas, const graphics::FontRenderer& font,
                               int x, int y, const PhysicalKey* row, size_t count,
                               const KeySnapshot& last_key) noexcept {
        int font_w = font.glyph_width();
        int cur_x = x;
        for (size_t i = 0; i < count; ++i) {
            bool active = (row[i].code == last_key.code);
            uint8_t fg = active ? 0x0F : 0x00;
            uint8_t bg = active ? 0x00 : 0x0F;
            draw_string(canvas, font, cur_x, y, "[", 0x00, 0x0F);
            draw_string(canvas, font, cur_x + font_w, y, row[i].label, fg, bg);
            draw_string(canvas, font, cur_x + font_w * 4, y, "]", 0x00, 0x0F);
            cur_x += font_w * 5;
        }
    }

    static void draw_sym_table(graphics::OwnedPixmap& canvas, const graphics::FontRenderer& font,
                              int y) noexcept {
        int font_h = font.glyph_height();
        int font_w = font.glyph_width();
        draw_string(canvas, font, 10, y, "=== SYM LAYER MAPPINGS ===");
        draw_string(canvas, font, 10, y + font_h, "Key Sym  Desc              Key Sym  Desc");
        for (size_t i = 0; i < 14 && i < HelpKeyCatalog::SYM_COUNT; ++i) {
            int line_y = y + font_h * (2 + static_cast<int>(i));
            draw_sym_entry(canvas, font, 10, line_y, HelpKeyCatalog::SYM_ENTRIES[i]);
            if (i + 14 < HelpKeyCatalog::SYM_COUNT) {
                draw_sym_entry(canvas, font, 10 + font_w * 26, line_y, HelpKeyCatalog::SYM_ENTRIES[i + 14]);
            }
        }
    }

    static void draw_sym_entry(graphics::OwnedPixmap& canvas, const graphics::FontRenderer& font,
                              int x, int y, const SymEntry& entry) noexcept {
        int font_w = font.glyph_width();
        char buf[8]{' ', entry.key, ' ', ' ', entry.symbol, ' ', '\0'};
        draw_string(canvas, font, x, y, buf);
        draw_string(canvas, font, x + font_w * 7, y, entry.desc);
    }

    static void draw_fn_table(graphics::OwnedPixmap& canvas, const graphics::FontRenderer& font,
                             int y) noexcept {
        int font_h = font.glyph_height();
        int font_w = font.glyph_width();
        draw_string(canvas, font, 10, y, "=== FUNCTION KEYS (MENU/FN LAYER) ===");
        draw_string(canvas, font, 10, y + font_h, "Key Fn   Description");
        for (size_t i = 0; i < HelpKeyCatalog::FN_COUNT; ++i) {
            int line_y = y + font_h * (2 + static_cast<int>(i));
            char buf[8]{' ', HelpKeyCatalog::FN_ENTRIES[i].key, ' ', ' ', '\0'};
            draw_string(canvas, font, 10, line_y, buf);
            draw_string(canvas, font, 10 + font_w * 4, line_y, HelpKeyCatalog::FN_ENTRIES[i].f_label);
            draw_string(canvas, font, 10 + font_w * 9, line_y, HelpKeyCatalog::FN_ENTRIES[i].desc);
        }
    }

    static void draw_status_bar(graphics::OwnedPixmap& canvas, const graphics::FontRenderer& font,
                               const HelpNavigationState& state, int y) noexcept {
        int font_w = font.glyph_width();
        draw_string(canvas, font, 8, y, "Key: ");
        const char* name = HelpKeyCatalog::keycode_to_name(state.last_key.code);
        if (name != nullptr) {
            draw_string(canvas, font, 8 + font_w * 5, y, name, 0x0F, 0x00);
        } else if (state.last_key.character >= 32) {
            char buf[2]{state.last_key.character, '\0'};
            draw_string(canvas, font, 8 + font_w * 5, y, buf, 0x0F, 0x00);
        } else {
            draw_string(canvas, font, 8 + font_w * 5, y, "---", 0x00, 0x0F);
        }
        draw_string(canvas, font, 8 + font_w * 16, y, "| Press Back/Right>/q/Enter to Exit");
    }
};

} // namespace help
} // namespace myts

#endif // MYTS_HELP_HELP_RENDERER_HPP
