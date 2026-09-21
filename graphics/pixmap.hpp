#ifndef MYTS_GRAPHICS_PIXMAP_HPP
#define MYTS_GRAPHICS_PIXMAP_HPP

#include "graphics/geometry.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace myts {
namespace graphics {

/**
 * @brief Non-owning view of a 4-bit per pixel (4bpp) packed raster pixmap.
 *
 * Each byte contains two packed 4-bit grayscale pixels (16 gray levels, high nibble left).
 */
struct PixmapView {
    int width{0};
    int height{0};
    int stride{0};
    const uint8_t* data{nullptr};

    constexpr PixmapView() noexcept = default;
    constexpr PixmapView(int w, int h, int s, const uint8_t* d) noexcept
        : width(w), height(h), stride(s), data(d) {}
};

/**
 * @brief Heap-allocated, owning 4bpp packed raster surface.
 *
 * Encapsulates pixel memory allocation, clearing, and coordinate-clipped blitting.
 */
class OwnedPixmap {
public:
    /**
     * @brief Constructs an empty surface.
     */
    OwnedPixmap() = default;

    /**
     * @brief Allocates a 4bpp pixmap of specified width and height.
     * @param w Width in pixels.
     * @param h Height in pixels.
     */
    OwnedPixmap(int w, int h) : width_(w), height_(h) {
        stride_ = (w + 1) / 2;
        storage_.resize(static_cast<size_t>(stride_ * height_), 0);
    }

    /**
     * @brief Clears entire surface to uniform byte value.
     * @param fill Byte pattern (e.g. 0x00 black, 0xFF white).
     */
    void clear(uint8_t fill = 0x00) noexcept {
        std::memset(storage_.data(), fill, storage_.size());
    }

    /**
     * @brief Sets a 4-bit pixel value at (x, y).
     * @param x Horizontal pixel coordinate.
     * @param y Vertical pixel coordinate.
     * @param color 4-bit pixel value (0x00 to 0x0F).
     */
    void set_pixel(int x, int y, uint8_t color) noexcept {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) {
            return;
        }
        size_t byte_idx = static_cast<size_t>(y * stride_ + (x / 2));
        uint8_t val = color & 0x0F;
        if ((x & 1) == 0) {
            // Even column: high nibble
            storage_[byte_idx] = (storage_[byte_idx] & 0x0F) | (val << 4);
        } else {
            // Odd column: low nibble
            storage_[byte_idx] = (storage_[byte_idx] & 0xF0) | val;
        }
    }

    /**
     * @brief Reads a 4-bit pixel value at (x, y).
     * @param x Horizontal pixel coordinate.
     * @param y Vertical pixel coordinate.
     * @return 4-bit pixel value (0x00 to 0x0F), or 0 if out of bounds.
     */
    [[nodiscard]] uint8_t get_pixel(int x, int y) const noexcept {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) {
            return 0;
        }
        size_t byte_idx = static_cast<size_t>(y * stride_ + (x / 2));
        uint8_t byte_val = storage_[byte_idx];
        if ((x & 1) == 0) {
            return (byte_val >> 4) & 0x0F;
        }
        return byte_val & 0x0F;
    }

    /**
     * @brief Blits a source pixmap into this pixmap at destination point.
     * @param src Source pixmap view.
     * @param dest Top-left point where blit should occur.
     * @param bg_or If true, pixel values are bitwise-ORed instead of copied.
     * @return true if blit executed, false if completely clipped out.
     */
    bool blit(const PixmapView& src, Point dest, bool bg_or = false) noexcept {
        if (src.data == nullptr || src.width <= 0 || src.height <= 0) {
            return false;
        }

        Rect screen(0, 0, width_, height_);
        Rect target(dest.x, dest.y, src.width, src.height);
        Rect clipped = target.clip(screen);
        if (clipped.is_empty()) {
            return false;
        }

        int sx_offset = clipped.x - dest.x;
        int sy_offset = clipped.y - dest.y;

        for (int row = 0; row < clipped.height; ++row) {
            int dst_y = clipped.y + row;
            int src_y = sy_offset + row;

            uint8_t* dst_row = storage_.data() + (dst_y * stride_);
            const uint8_t* src_row = src.data + (src_y * src.stride);

            // Row-wise byte blit (supports even aligned offsets)
            for (int col = 0; col < clipped.width; col += 2) {
                int dst_byte_idx = (clipped.x + col) / 2;
                int src_byte_idx = (sx_offset + col) / 2;
                uint8_t s_val = src_row[src_byte_idx];
                if (bg_or) {
                    dst_row[dst_byte_idx] |= s_val;
                } else {
                    dst_row[dst_byte_idx] = s_val;
                }
            }
        }
        return true;
    }

    /**
     * @brief Width in pixels.
     */
    [[nodiscard]] int width() const noexcept { return width_; }

    /**
     * @brief Height in pixels.
     */
    [[nodiscard]] int height() const noexcept { return height_; }

    /**
     * @brief Stride in bytes per row.
     */
    [[nodiscard]] int stride() const noexcept { return stride_; }

    /**
     * @brief Mutable pointer to underlying raster data.
     */
    [[nodiscard]] uint8_t* data() noexcept { return storage_.data(); }

    /**
     * @brief Const pointer to underlying raster data.
     */
    [[nodiscard]] const uint8_t* data() const noexcept { return storage_.data(); }

    /**
     * @brief Total byte size of raster buffer.
     */
    [[nodiscard]] size_t size() const noexcept { return storage_.size(); }

    /**
     * @brief Creates a non-owning PixmapView of this pixmap.
     */
    [[nodiscard]] PixmapView view() const noexcept {
        return PixmapView(width_, height_, stride_, storage_.data());
    }

private:
    int width_{0};
    int height_{0};
    int stride_{0};
    std::vector<uint8_t> storage_;
};

} // namespace graphics
} // namespace myts

#endif // MYTS_GRAPHICS_PIXMAP_HPP
