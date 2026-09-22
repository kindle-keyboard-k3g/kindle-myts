/**
 * @file debug_overlay.hpp
 * @brief On-screen framebuffer telemetry overlay for diagnostic builds.
 *
 * Renders live frame refresh counts, PTY I/O rates, dirty bounds, and input stats
 * directly onto a 4bpp display canvas using FontRenderer.
 */

#pragma once

#include "graphics/pixmap.hpp"
#include "graphics/font_renderer.hpp"
#include "graphics/geometry.hpp"
#include "core/metrics.hpp"
#include "core/string_builder.hpp"

namespace myts::graphics {

/**
 * @class DebugOverlay
 * @brief Renders monospace diagnostic telemetry banners into raster surfaces.
 */
class DebugOverlay {
public:
    constexpr DebugOverlay() noexcept = default;

    /**
     * @brief Render formatted metrics banner onto a 4bpp pixmap.
     *
     * @param canvas Target raster surface.
     * @param font Font renderer used for glyph rendering.
     * @param metrics Snapshot of performance metrics.
     * @param x Destination column coordinate.
     * @param y Destination row coordinate.
     * @return Bounding rectangle of the modified overlay area.
     */
    Rect render(OwnedPixmap& canvas, const FontRenderer& font,
                const core::MetricsSnapshot& metrics, int x = 0, int y = 0) {
        if (canvas.width() <= 0 || canvas.height() <= 0) {
            return Rect(0, 0, 0, 0);
        }

        // Format short telemetry banner string
        core::StringBuilder sb;
        sb.reserve(128);
        sb.append("DBG: F=");
        sb.append_decimal(static_cast<int>(metrics.full_refreshes));
        sb.append(" P=");
        sb.append_decimal(static_cast<int>(metrics.partial_refreshes));
        sb.append(" PTY=");
        sb.append_decimal(static_cast<int>(metrics.pty_bytes_read));
        sb.append("/");
        sb.append_decimal(static_cast<int>(metrics.pty_bytes_written));
        sb.append(" IN=");
        sb.append_decimal(static_cast<int>(metrics.input_events));
        sb.append(" D=");
        sb.append_decimal(metrics.dirty_bounding_box.width);
        sb.append("x");
        sb.append_decimal(metrics.dirty_bounding_box.height);

        std::string_view text = sb.view();
        int font_w = font.glyph_width();
        int font_h = font.glyph_height();

        int max_chars_fit = (canvas.width() - x) / font_w;
        if (max_chars_fit <= 0) {
            return Rect(0, 0, 0, 0);
        }

        size_t chars_to_draw = std::min(text.size(), static_cast<size_t>(max_chars_fit));
        int total_w = static_cast<int>(chars_to_draw) * font_w;
        int total_h = std::min(font_h, canvas.height() - y);

        // Render characters: black text on white background (inverted / high contrast)
        for (size_t i = 0; i < chars_to_draw; ++i) {
            int cur_x = x + static_cast<int>(i) * font_w;
            uint32_t cp = static_cast<uint8_t>(text[i]);
            font.draw_char(canvas, cur_x, y, cp, 0x00 /* black fg */, 0x0E /* near-white bg */);
        }

        Rect banner_rect(x, y, total_w, total_h);
        return banner_rect.clip(Rect(0, 0, canvas.width(), canvas.height()));
    }
};

} // namespace myts::graphics
