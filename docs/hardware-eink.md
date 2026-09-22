# E-Ink Hardware & Display Subsystem

This document provides technical details on the Kindle E-Ink display subsystem, including Linux framebuffer architecture, 4bpp packed pixel encoding, Broadsheet controller ioctls, and refresh algorithms.

---

## 1. Framebuffer Memory Layout

Kindle devices (Kindle Keyboard / K3, Kindle DX) use a Freescale i.MX353 processor coupled to an Epson Broadsheet EPD controller driving an electronic paper display (EPD).

### 1.1 Resolution and Bit Depth
- **Resolution**: Typically 600 × 800 pixels (Kindle 3) or 824 × 1200 pixels (Kindle DX).
- **Pixel Format**: 4 bits per pixel (4bpp), representing 16 levels of grayscale (0 = black, 15 = white).
- **Packing**: Two pixels are packed into each byte:
  - **Even X (high nibble)**: Bits 7..4
  - **Odd X (low nibble)**: Bits 3..0
- **Byte Stride**: `width / 2` bytes per row.
- **Total Framebuffer Size**: `(width * height) / 2` bytes (e.g. 240,000 bytes for 600×800).

```
Byte at index (y * stride + x / 2):
+-----------------------+----------------------+
| Bit 7 | 6 | 5 | Bit 4 | Bit 3 | 2 | 1 | Bit 0|
+-----------------------+----------------------+
|     Pixel (X, Y)      |    Pixel (X+1, Y)    |
|       (Even X)        |       (Odd X)        |
+-----------------------+----------------------+
```

### 1.2 Nibble Operations in C++ (`graphics/pixmap.hpp`)

```cpp
void set_pixel(int x, int y, uint8_t color) noexcept {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    size_t byte_idx = static_cast<size_t>(y * stride_ + (x / 2));
    uint8_t val = color & 0x0F;
    if ((x & 1) == 0) {
        storage_[byte_idx] = (storage_[byte_idx] & 0x0F) | (val << 4);
    } else {
        storage_[byte_idx] = (storage_[byte_idx] & 0xF0) | val;
    }
}

[[nodiscard]] uint8_t get_pixel(int x, int y) const noexcept {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return 0;
    size_t byte_idx = static_cast<size_t>(y * stride_ + (x / 2));
    uint8_t byte_val = storage_[byte_idx];
    return (x & 1) == 0 ? ((byte_val >> 4) & 0x0F) : (byte_val & 0x0F);
}
```

---

## 2. Broadsheet EPD Controller Interface (`linux/einkfb.h`)

Unlike standard LCD framebuffers where writing to memory immediately reflects on screen, E-Ink screens are bistable. Modifying framebuffer memory does not trigger a physical display refresh until an explicit `ioctl` command is issued to the controller.

### 2.1 Ioctl Definitions

- `FBIO_EINK_UPDATE_DISPLAY (0x46db)`: Triggers a full display refresh.
- `FBIO_EINK_UPDATE_DISPLAY_AREA (0x46dc)`: Refreshes a specified bounding rectangle.

### 2.2 Update Area Structure (`update_area_t`)

```c
typedef struct {
    int x1;             /* Left coordinate (must be even) */
    int y1;             /* Top coordinate */
    int x2;             /* Right coordinate (must be even) */
    int y2;             /* Bottom coordinate */
    fx_type which_fx;   /* Waveform refresh mode */
    u8 *buffer;         /* Pointer to custom buffer (NULL for /dev/fb0) */
} update_area_t;
```

### 2.3 Refresh Waveform Modes (`fx_type`)
- `fx_update_partial (0)`: Fast 1-bit or 2-bit monochrome update. Does not flash the screen; ideal for typing and cursor movement.
- `fx_update_full (1)`: Full 16-level grayscale update.
- `fx_update_flash (20)`: Flashes inverted waveform to clear ghosting particles from E-Ink capsules.

---

## 3. Even Coordinate Alignment Constraint

Because two 4bpp pixels share a single byte, hardware controllers require horizontal coordinates (`x1`, `x2`) to be aligned on even pixel boundaries.

In `graphics/eink_display.hpp`, coordinate alignment is enforced automatically:

```cpp
// Ensure left bound is even (round down)
int aligned_x = dirty_.x & ~1;
// Ensure right bound is even (round up)
int aligned_x2 = (dirty_.x + dirty_.width + 1) & ~1;
Rect aligned_rect(aligned_x, dirty_.y, aligned_x2 - aligned_x, dirty_.height);
```

---

## 4. Hardware Driver & Mock Fallback

`display/HardwareEinkDriver` automatically abstracts hardware differences:
1. When running on a physical Kindle:
   - Opens `/dev/fb0` with `O_RDWR`.
   - Queries `FBIOGET_VSCREENINFO` for geometry.
   - Maps memory via `core::MemoryMapping` (`mmap`).
   - Dispatches `FBIO_EINK_UPDATE_DISPLAY_AREA` via `ioctl`.
2. When running on a host / CI workstation:
   - Gracefully detects `/dev/fb0` absence.
   - Allocates an in-memory `mock_buffer_` (600×800 4bpp).
   - Simulates all refresh calls without errors, enabling unit tests under AddressSanitizer.
