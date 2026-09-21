#ifndef MYTS_GRAPHICS_EINK_DISPLAY_HPP
#define MYTS_GRAPHICS_EINK_DISPLAY_HPP

#include "graphics/geometry.hpp"
#include <algorithm>
#include <optional>
#include <cstdint>

namespace myts {
namespace graphics {

/**
 * @brief Abstract driver interface for Kindle E-Ink hardware controllers.
 */
class IEinkDriver {
public:
    virtual ~IEinkDriver() = default;

    /**
     * @brief Performs a full-screen GC16 waveform refresh to eliminate ghosting.
     */
    virtual bool update_display_full() = 0;

    /**
     * @brief Performs a fast partial update on the specified bounding box.
     */
    virtual bool update_display_area(const Rect& area) = 0;
};

/**
 * @brief Manages E-Ink display surface, partial update batching, and sub-byte alignment.
 *
 * Kindle Pearl 4bpp displays require 2 pixels per byte, meaning horizontal coordinates
 * must be aligned to even pixel boundaries to prevent nibble tearing or driver glitches.
 */
class EinkDisplay {
public:
    /**
     * @brief Constructs an EinkDisplay manager.
     * @param width Display width in pixels (typically 600).
     * @param height Display height in pixels (typically 800).
     * @param driver Underlying hardware or mock EPD driver.
     */
    EinkDisplay(int width, int height, IEinkDriver& driver) noexcept
        : bounds_{0, 0, width, height}, driver_(driver), dirty_region_(std::nullopt) {}

    /**
     * @brief Accumulates a dirty rectangular region into the pending refresh area.
     * @param box Dirty region to invalidate.
     */
    void mark_dirty(const Rect& box) noexcept {
        Rect clipped = box.clip(bounds_);
        if (clipped.is_empty()) {
            return;
        }

        // Align X coordinate down to even pixel for 4bpp nibble boundary
        int x1 = clipped.x & ~1;
        int x2 = (clipped.x + clipped.width + 1) & ~1;
        int y1 = clipped.y;
        int y2 = clipped.y + clipped.height;

        Rect aligned_box{x1, y1, x2 - x1, y2 - y1};

        if (!dirty_region_.has_value()) {
            dirty_region_ = aligned_box;
        } else {
            int ux1 = std::min(dirty_region_->x, aligned_box.x);
            int uy1 = std::min(dirty_region_->y, aligned_box.y);
            int ux2 = std::max(dirty_region_->x + dirty_region_->width, aligned_box.x + aligned_box.width);
            int uy2 = std::max(dirty_region_->y + dirty_region_->height, aligned_box.y + aligned_box.height);
            dirty_region_ = Rect{ux1, uy1, ux2 - ux1, uy2 - uy1};
        }
    }

    /**
     * @brief Checks if there are pending un-flushed dirty regions.
     */
    [[nodiscard]] bool has_dirty() const noexcept {
        return dirty_region_.has_value();
    }

    /**
     * @brief Flushes pending dirty area to the hardware controller via partial refresh.
     * @return true if hardware update succeeded or if no region was dirty.
     */
    bool flush() {
        if (!dirty_region_.has_value()) {
            return true;
        }
        Rect area = *dirty_region_;
        dirty_region_ = std::nullopt;
        return driver_.update_display_area(area);
    }

    /**
     * @brief Forces an immediate full-screen waveform refresh.
     */
    bool refresh_full() {
        dirty_region_ = std::nullopt;
        return driver_.update_display_full();
    }

    /**
     * @brief Returns the overall display boundaries.
     */
    [[nodiscard]] const Rect& bounds() const noexcept {
        return bounds_;
    }

private:
    Rect bounds_;
    IEinkDriver& driver_;
    std::optional<Rect> dirty_region_;
};

} // namespace graphics
} // namespace myts

#endif // MYTS_GRAPHICS_EINK_DISPLAY_HPP
