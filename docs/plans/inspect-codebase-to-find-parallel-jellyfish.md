# Comprehensive Kindle 3 / Kindle 3G Hardware Capabilities, Limitations & Modernization Plan for `kindle-myts`

## 1. Kindle 3 / Kindle 3G Hardware Profile & Edge Cases

### 1.1 CPU & Compute Architecture
- **SoC:** Freescale (NXP) i.MX353 / i.MX35 processor.
- **Core:** ARM1136JF-S @ **532 MHz** (ARMv6 architecture with VFP floating-point unit).
- **Instruction Set:** ARMv6 (requires `-march=armv6j -mtune=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp`).
- **Memory Footprint:** 
  - Total RAM: **128 MB** Mobile DDR (or 256 MB on select board revisions).
  - Stock Amazon Java framework consumes ~120 MB RAM when running (`/etc/init.d/framework stop` frees up memory for native tools).
  - Native processes must target a tight RSS footprint (< 5–10 MB) to prevent invocation of the Linux OOM-killer.

### 1.2 E-Ink Display & Controller Architecture
- **Display Panel:** 6-inch E-Ink Pearl, 600 × 800 resolution (167 PPI), 16 levels of gray (4 bpp).
- **Controller Hardware:**
  - Epson Broadsheet EPD controller (S1D13522), interfacing via `broadsheetfb.ko` / `einkfb.h`.
  - Framebuffer interface: `/dev/fb0` mapped as `600 * 800 * 4 / 8 = 240,000 bytes` (or 480 KB double-buffered / 8bpp emulated depending on driver mode).
- **Custom `ioctl` Interface (`linux/einkfb.h`):**
  - Magic Number: `'F'` (`0x46`).
  - `FBIO_EINK_UPDATE_DISPLAY` (`0x46db`): Full-screen refresh. Passing `0` (`fx_update_partial`) triggers a non-flashing GU/PU update (fast, but accumulates ghosting). Passing `1` (`fx_update_full`) forces a GC16 waveform flash to wipe ghosting.
  - `FBIO_EINK_UPDATE_DISPLAY_AREA` (`0x46dd`): Bounding-box partial update using `struct update_area_t { int x1, y1, x2, y2; fx_type which_fx; __u8 *buffer; }`.
  - `FBIO_EINK_CLEAR_SCREEN` (`0x46e1`): Wipes the panel to clean white.
- **Hardware Refresh Latencies & Edge Cases:**
  - Fast partial update (DU mode): ~120 ms to 250 ms. Fast enough for terminal typing and cursor movement, but causes ghosting and contrast degradation after 10–20 updates.
  - Full refresh (GC16 mode): ~600 ms to 900 ms. Involves black-to-white flashing; necessary periodically or on full screen redraws.
  - Pixel alignment: Sub-byte boundary clipping on 4 bpp packed pixels (2 pixels per byte) requires even coordinate alignment (`x0 & ~1`) to avoid horizontal jitter or corrupted nibble pairing.

### 1.3 Networking Capabilities & Kindle 3G Limitations
- **Wi-Fi:** 802.11b/g/n (Atheros AR6102 chipset via `wlan0`). Standard TCP/IP routing without proxy restrictions.
- **3G Cellular Modem:**
  - Qualcomm Gobi or AnyDATA modem exposed via `ppp0`.
  - **Whispernet Proxies:** 3G traffic is intercepted and routed via Amazon FINTS HTTP proxy (`fints-g7g.amazon.com:80` / `fints.amazon.com:443`).
  - **Authentication Header:** Requests require the Amazon hardware serial token (`x-fsn` header) over HTTP `CONNECT` or proxy GET/POST. Direct raw TCP sockets (non-HTTP/HTTPS) are dropped by the cellular APN gateway unless tunneled over HTTP `CONNECT`.

---

## 2. Identified Opportunities in `kindle-myts`

1. **64-bit Pointer Corruption & Segmentation Faults**:
   - In `screen.c:151-155`: `b` is cast to `uint32_t*` (`uint32_t *b=(uint32_t*)font->pixmap; ppx->surface=(unsigned char *)b[code];`). On 64-bit systems, pointers are 8 bytes, so indexing with `uint32_t*` slices addresses in half, causing instant crashes when testing on x86_64 host machines. Must be `void**` or `uintptr_t*`.
2. **Zero Automated Tests**:
   - `kindle-myts` has no unit tests. Any refactoring or bug fix risks regressions in dynamic string handling, ANSI escape parsing, and INI configuration loading.
3. **Compiler Warnings & Unsafe String Operations**:
   - Multiple `-Wsign-compare`, `-Wformat`, and `-Wtype-limits` warnings in `dynstring.c`, `terminal.c`, `launchpad.c`, and `font.c`.
   - Out-of-bounds risks in `sscanf` font parsing and signed/unsigned length checks in `dynstring.c`.
4. **Build System Rigidity**:
   - Makefile hardcodes `CC=musl-gcc` and `-static -Werror`. Lacks native host test runners, debug flags (`-g3 -O0 -DDEBUG`), and sanitizer targets (`-fsanitize=address,undefined`).
5. **E-Ink Refresh Inefficiencies**:
   - Screen update routines perform full updates where partial bounding-box updates (`FBIO_EINK_UPDATE_DISPLAY_AREA`) would eliminate flicker and cut latency from ~800ms down to ~150ms.

---

## 3. Test-Driven Development (TDD) Seams & Strategy

Following `mattpocock-skills:tdd`, tests will be implemented against public domain seams:

### Seam 1: Dynamic String Buffer (`dynstring.h`)
- **Seam:** `ds_create`, `dsprintf`, `ds_append`, `ds_truncate`, `ds_shift`, `ds_reset`, `ds_free`.
- **Test Scenarios:**
  - Safe allocation and dynamic growth past initial chunk size.
  - Truncation and shifting without memory leaks or underflow.
  - Read-only buffer reference safety (`ds_ref`).

### Seam 2: Configuration Loader (`config.h`)
- **Seam:** `cfg_read`, `cfg_get_str`, `cfg_get_int`, `cfg_set`.
- **Test Scenarios:**
  - Parsing key-value pairs from INI text.
  - Handling missing keys with default fallbacks.
  - Comment and whitespace sanitization.

### Seam 3: Pixel Operations & Coordinate Truncation (`pixop.h`)
- **Seam:** `c_truncate`, coordinate clipping, and 4 bpp packed nibble bounds.
- **Test Scenarios:**
  - Truncating coordinates outside 600×800 display dimensions.
  - Even alignment verification for 4bpp nibble pairs.

### Seam 4: ANSI Terminal Escape Sequence Engine (`terminal.h`)
- **Seam:** `do_csi`, cursor movement, clear screen (`\033[2J`), line wrapping.
- **Test Scenarios:**
  - Cursor repositioning sequences.
  - UTF-8 multi-byte decoding vs 8-bit mode.

---

## 4. Execution Plan

### Step 1: Create Lightweight Test Harness & Makefile Targets
- Create `tests/test_framework.h` with assertion macros (`ASSERT_EQ`, `ASSERT_STR_EQ`, `ASSERT_TRUE`, `RUN_TEST`).
- Update `Makefile`:
  - Allow `CC ?= gcc` fallback if `musl-gcc` is absent.
  - Add `make test` (builds and executes test suite on host).
  - Add `make test-asan` (runs with AddressSanitizer and UndefinedBehaviorSanitizer).

### Step 2: Implement Seam 1 (Dynstring) with TDD
- Write `tests/test_dynstring.c` with failing tests.
- Fix signedness bugs and unsigned comparisons in `dynstring.c`.
- Verify green tests under standard `gcc` and `asan`.

### Step 3: Implement Seam 2 (Config) & Seam 3 (Pixop) with TDD
- Write `tests/test_config.c` and `tests/test_pixop.c`.
- Fix signedness and boundary handling in `config.c` and `pixop.c`.
- Verify green tests.

### Step 4: Fix 64-bit Pointer Bugs & Compiler Warnings
- Refactor `screen.c` and `font.c` to use `void**` instead of `uint32_t*` for `font->pixmap`.
- Fix format specifiers in `terminal.c` (`%td` for pointer differences, fix `.*` precision parameter types).
- Eliminate all compiler warnings under `-Wall -Wextra`.

### Step 5: Verification
- Execute `make test` and `make test-asan`.
- Confirm 100% test pass rate with zero leaks or sanitizer errors.
