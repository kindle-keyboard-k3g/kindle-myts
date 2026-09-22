# BDF & Hex Font Rendering

This document explains font handling, glyph storage, and 4bpp packed raster rendering in `kindle-myts`.

---

## 1. Font Format: GNU Unifont / BDF Hex

`kindle-myts` uses the simple hexadecimal font format (such as `ter-u12n.hex` from Terminus Font or GNU Unifont):

```
0041:0000003c66667e6666000000
```

### 1.1 Line Structure
- **Codepoint**: 4 or 6 hex digits preceding the colon (`0041` for ASCII `'A'`).
- **Colon (`:`)**: Field separator.
- **Bitmap Data**: Even number of hexadecimal characters encoding the bitmap rows.
  - For an 8×12 font, each row is 8 pixels (1 byte = 2 hex digits).
  - 12 rows × 2 hex digits = 24 hex characters.
  - A set bit (`1`) indicates a foreground pixel; an unset bit (`0`) indicates background.

---

## 2. In-Memory Glyph Cache (`graphics/font_renderer.hpp`)

Glyphs are cached in fixed-size structures without heap fragmentation:

```cpp
struct GlyphBitmap {
    uint32_t codepoint{0};
    uint8_t data[32]{}; // Up to 32 rows of 8-pixel glyph data
};
```

Glyphs are indexed using an internal lookup vector or direct array. Common ASCII codepoints (`0x20` to `0x7E`) provide $O(1)$ constant-time lookup.

---

## 3. 4bpp Rasterization Algorithm

When drawing a character into a 4bpp packed surface:

```cpp
bool draw_char(OwnedPixmap& dst, int x, int y, uint32_t codepoint,
               uint8_t fg = 0x00, uint8_t bg = 0x0F) const noexcept {
    const GlyphBitmap* g = find_glyph(codepoint);
    if (!g) return false;

    for (int row = 0; row < glyph_height_; ++row) {
        uint8_t bits = g->data[row];
        for (int col = 0; col < glyph_width_; ++col) {
            bool pixel_on = (bits & (0x80 >> col)) != 0;
            dst.set_pixel(x + col, y + row, pixel_on ? fg : bg);
        }
    }
    return true;
}
```

### 3.1 Clipping Safety
Coordinates are clipped automatically at pixel boundaries by `dst.set_pixel()`. Drawing characters partially off-screen or against screen margins never causes out-of-bounds memory writes.
