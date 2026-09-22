#ifndef MYTS_GRAPHICS_GEOMETRY_HPP
#define MYTS_GRAPHICS_GEOMETRY_HPP

namespace myts {
namespace graphics {

/**
 * @brief 2D discrete coordinate point.
 */
struct Point {
    int x{0};
    int y{0};

    constexpr Point() noexcept = default;
    constexpr Point(int px, int py) noexcept : x(px), y(py) {}
};

/**
 * @brief 2D bounding box rectangle with inclusive top-left and width/height.
 */
struct Rect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};

    constexpr Rect() noexcept = default;
    constexpr Rect(int rx, int ry, int rw, int rh) noexcept
        : x(rx), y(ry), width(rw), height(rh) {}

    /**
     * @brief Right edge coordinate (x + width).
     */
    [[nodiscard]] constexpr int right() const noexcept {
        return x + width;
    }

    /**
     * @brief Bottom edge coordinate (y + height).
     */
    [[nodiscard]] constexpr int bottom() const noexcept {
        return y + height;
    }

    /**
     * @brief Checks if rectangle has non-positive area.
     * @return true if width <= 0 or height <= 0.
     */
    [[nodiscard]] constexpr bool is_empty() const noexcept {
        return width <= 0 || height <= 0;
    }

    /**
     * @brief Clips this rectangle against bounding bounds.
     * @param bounds Outer bounding rectangle.
     * @return Clipped rectangle within bounds.
     */
    [[nodiscard]] Rect clip(const Rect& bounds) const noexcept {
        if (is_empty() || bounds.is_empty()) {
            return Rect(0, 0, 0, 0);
        }
        int nx = x;
        int ny = y;
        int nw = width;
        int nh = height;

        // Left clipping
        if (nx < bounds.x) {
            nw -= (bounds.x - nx);
            nx = bounds.x;
        }
        // Top clipping
        if (ny < bounds.y) {
            nh -= (bounds.y - ny);
            ny = bounds.y;
        }
        // Right clipping
        int max_x = bounds.x + bounds.width;
        if (nx + nw > max_x) {
            nw = max_x - nx;
        }
        // Bottom clipping
        int max_y = bounds.y + bounds.height;
        if (ny + nh > max_y) {
            nh = max_y - ny;
        }

        if (nw <= 0 || nh <= 0) {
            return Rect(0, 0, 0, 0);
        }
        return Rect(nx, ny, nw, nh);
    }
};

} // namespace graphics
} // namespace myts

#endif // MYTS_GRAPHICS_GEOMETRY_HPP
