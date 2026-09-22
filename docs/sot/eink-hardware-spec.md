# Source of Truth: Kindle E-Ink Hardware & Framebuffer Specification

Authoritative specifications for the Kindle 3 (Kindle Keyboard) and Kindle DX display subsystems.

---

## 1. Physical Device Parameters

| Device | Controller | CPU | Resolution | BPP | Memory Size | Framebuffer Path |
|---|---|---|---|---|---|---|
| Kindle 3 (K3) | Epson Broadsheet | Freescale i.MX353 (532 MHz) | 600 × 800 | 4 bpp | 240,000 bytes | `/dev/fb0` |
| Kindle DX | Epson Broadsheet | Freescale i.MX353 (532 MHz) | 824 × 1200 | 4 bpp | 494,400 bytes | `/dev/fb0` |

---

## 2. 4bpp Packed Pixel Alignment Rules

1. **Nibble Order**:
   - `x & 1 == 0` (Even column) $\rightarrow$ High nibble (bits 7..4).
   - `x & 1 == 1` (Odd column) $\rightarrow$ Low nibble (bits 3..0).
2. **Horizontal Bounds**:
   - Update bounding box `x1` and `x2` must be even integers.
   - Coordinate transformation: `x1_aligned = x1 & ~1`, `x2_aligned = (x2 + 1) & ~1`.
3. **Grayscale Levels**:
   - `0x0` = Full Black.
   - `0xF` = Full White.

---

## 3. Kernel Ioctl Reference (`linux/einkfb.h`)

```c
#define FBIO_EINK_UPDATE_DISPLAY_AREA   0x46dc
#define FBIO_EINK_UPDATE_DISPLAY        0x46db

typedef enum {
    fx_update_partial   = 0,
    fx_update_full      = 1,
    fx_update_flash     = 20,
    fx_update_invert    = 21,
} fx_type;
```
