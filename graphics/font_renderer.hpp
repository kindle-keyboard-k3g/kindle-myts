#ifndef MYTS_GRAPHICS_FONT_RENDERER_HPP
#define MYTS_GRAPHICS_FONT_RENDERER_HPP

#include "graphics/pixmap.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <vector>
#include <fcntl.h>
#include <unistd.h>

namespace myts {
namespace graphics {

/**
 * @brief Glyph bitmap information and pixel extraction.
 */
struct GlyphBitmap {
    int width{0};
    int height{0};
    int bytes_per_row{0};
    const uint8_t* data{nullptr};
};

/**
 * @brief High-performance BDF / Hex font loader and 4bpp surface renderer.
 *
 * Designed for low-latency terminal rendering on Kindle E-Ink displays.
 * Parses standard GNU unifont / BDF-derived hex font files (such as ter-u12n.hex).
 * Supports embedded profiles with -fno-exceptions and -fno-rtti.
 */
class FontRenderer {
public:
    /**
     * @brief Constructs a font renderer with expected glyph dimensions.
     * @param glyph_width Width in pixels (default 8).
     * @param glyph_height Height in pixels (default 12).
     */
    explicit FontRenderer(int glyph_width = 8, int glyph_height = 12) noexcept
        : width_(glyph_width),
          height_(glyph_height),
          bytes_per_row_((glyph_width + 7) / 8),
          glyph_bytes_(height_ * bytes_per_row_),
          glyph_offsets_(65536, 0) {}

    /**
     * @brief Gets font glyph width.
     */
    [[nodiscard]] int glyph_width() const noexcept { return width_; }

    /**
     * @brief Gets font glyph height.
     */
    [[nodiscard]] int glyph_height() const noexcept { return height_; }

    /**
     * @brief Checks if a Unicode codepoint has a valid glyph loaded.
     * @param codepoint Unicode codepoint (0x0000 - 0xFFFF).
     */
    [[nodiscard]] bool has_glyph(uint32_t codepoint) const noexcept {
        if (codepoint >= glyph_offsets_.size()) {
            return false;
        }
        return glyph_offsets_[codepoint] != 0;
    }

    /**
     * @brief Retrieves the glyph descriptor for a given codepoint.
     * @param codepoint Unicode codepoint.
     * @return GlyphBitmap if found, or empty GlyphBitmap.
     */
    [[nodiscard]] GlyphBitmap get_glyph(uint32_t codepoint) const noexcept {
        if (!has_glyph(codepoint)) {
            return GlyphBitmap{0, 0, 0, nullptr};
        }
        size_t offset = static_cast<size_t>(glyph_offsets_[codepoint] - 1);
        return GlyphBitmap{width_, height_, bytes_per_row_, glyph_storage_.data() + offset};
    }

    /**
     * @brief Loads hex font data from an in-memory string view.
     *
     * Format per line: `<hex-codepoint>:<hex-bitmap-bytes>`
     * @param hex_content String view containing hex font data.
     * @return true if at least one glyph was successfully loaded.
     */
    bool load_hex_data(std::string_view hex_content) {
        size_t pos = 0;
        size_t total_loaded = 0;

        while (pos < hex_content.size()) {
            // Find end of line
            size_t eol = hex_content.find('\n', pos);
            if (eol == std::string_view::npos) {
                eol = hex_content.size();
            }

            std::string_view line = hex_content.substr(pos, eol - pos);
            pos = eol + 1;

            if (line.empty() || line[0] == '#') {
                continue;
            }

            if (parse_hex_line(line)) {
                ++total_loaded;
            }
        }

        return total_loaded > 0;
    }

    /**
     * @brief Loads hex font data from a filesystem path.
     * @param filepath Path to the .hex font file.
     * @return true on success, false if file could not be read.
     */
    bool load_hex_file(const char* filepath) {
        if (filepath == nullptr) {
            return false;
        }
        int fd = ::open(filepath, O_RDONLY);
        if (fd < 0) {
            return false;
        }

        std::vector<char> buffer;
        constexpr size_t chunk_size = 65536;
        char chunk[chunk_size];
        ssize_t bytes_read = 0;

        while ((bytes_read = ::read(fd, chunk, chunk_size)) > 0) {
            buffer.insert(buffer.end(), chunk, chunk + bytes_read);
        }
        ::close(fd);

        if (buffer.empty()) {
            return false;
        }

        return load_hex_data(std::string_view(buffer.data(), buffer.size()));
    }

    /**
     * @brief Draws a single glyph onto a target 4bpp surface.
     * @param dst Destination OwnedPixmap.
     * @param x Top-left destination column.
     * @param y Top-left destination row.
     * @param codepoint Unicode codepoint.
     * @param fg Foreground 4-bit gray level (0x00=black).
     * @param bg Background 4-bit gray level (0x0F=white).
     * @return true if glyph was drawn, false if glyph missing.
     */
    bool draw_char(OwnedPixmap& dst, int x, int y, uint32_t codepoint,
                   uint8_t fg = 0x00, uint8_t bg = 0x0F) const noexcept {
        GlyphBitmap glyph = get_glyph(codepoint);
        if (glyph.data == nullptr) {
            return false;
        }

        for (int r = 0; r < height_; ++r) {
            int dst_y = y + r;
            const uint8_t* row_bytes = glyph.data + (r * bytes_per_row_);

            for (int c = 0; c < width_; ++c) {
                int byte_idx = c / 8;
                int bit_idx = 7 - (c % 8);
                bool is_fg = (row_bytes[byte_idx] & (1 << bit_idx)) != 0;
                dst.set_pixel(x + c, dst_y, is_fg ? fg : bg);
            }
        }

        return true;
    }

private:
    static constexpr int hex_nibble(char c) noexcept {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    bool parse_hex_line(std::string_view line) {
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string_view::npos || colon_pos == 0) {
            return false;
        }

        // Parse codepoint
        uint32_t codepoint = 0;
        for (size_t i = 0; i < colon_pos; ++i) {
            int nibble = hex_nibble(line[i]);
            if (nibble < 0) {
                return false;
            }
            codepoint = (codepoint << 4) | static_cast<uint32_t>(nibble);
        }

        if (codepoint >= 65536) {
            return false;
        }

        // Parse bitmap hex string
        std::string_view hex_data = line.substr(colon_pos + 1);
        while (!hex_data.empty() && (hex_data.back() == '\r' || hex_data.back() == ' ')) {
            hex_data.remove_suffix(1);
        }

        size_t expected_hex_len = static_cast<size_t>(height_ * bytes_per_row_ * 2);
        if (hex_data.size() < expected_hex_len) {
            return false;
        }

        size_t store_offset = glyph_storage_.size();
        glyph_storage_.resize(store_offset + glyph_bytes_);

        for (size_t b = 0; b < glyph_bytes_; ++b) {
            int hi = hex_nibble(hex_data[b * 2]);
            int lo = hex_nibble(hex_data[b * 2 + 1]);
            if (hi < 0 || lo < 0) {
                glyph_storage_.resize(store_offset);
                return false;
            }
            glyph_storage_[store_offset + b] = static_cast<uint8_t>((hi << 4) | lo);
        }

        glyph_offsets_[codepoint] = static_cast<uint32_t>(store_offset + 1);
        return true;
    }

    int width_{8};
    int height_{12};
    int bytes_per_row_{1};
    size_t glyph_bytes_{12};
    std::vector<uint8_t> glyph_storage_;
    std::vector<uint32_t> glyph_offsets_;
};

} // namespace graphics
} // namespace myts

#endif // MYTS_GRAPHICS_FONT_RENDERER_HPP
