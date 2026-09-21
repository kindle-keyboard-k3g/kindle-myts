# Modern C++ Stack Architecture & Implementation Plan for `kindle-myts`

## 1. Context & Objectives

The legacy `kindle-myts` terminal emulator was written in 2010 in procedural C, targeting the Kindle 3 (Freescale i.MX353 ARMv6 @ 532 MHz, 128 MB RAM, Epson Broadsheet EPD controller). While fast, the legacy C codebase suffers from:
- Raw pointer lifecycle management without RAII, risking leaks and dangling pointers.
- Mixed concerns: terminal emulation, PTY communication, E-Ink refresh ioctls, and input event parsing are tightly coupled in global structs (`struct my_args __me`).
- Inability to unit-test critical paths without Kindle hardware kernel headers (`linux/einkfb.h`, `/dev/fb0`, `/dev/input/event*`).

Recent commits established modern C++ foundational modules (tested at 100% pass rate under AddressSanitizer):
1. `core/raii.hpp`: `UniqueFd`, `MemoryMapping`
2. `core/byte_buffer.hpp`: `ByteView`, `ByteBuffer`
3. `core/string_builder.hpp`: `StringBuilder`
4. `core/event_loop.hpp`: allocation-free `select()` dispatcher with timers
5. `graphics/geometry.hpp`: `Point`, `Rect`
6. `graphics/pixmap.hpp`: 4bpp packed raster surface, clipping, blit
7. `graphics/eink_display.hpp`: `IEinkDriver`, dirty region accumulation, even horizontal alignment for 4bpp nibble pairs
8. `config/config.hpp`: case-insensitive INI configuration parser
9. `terminal/ansi_parser.hpp`: VT100 / CSI stream state machine with `IAnsiHandler`

This plan details the modernization roadmap to replace legacy C entry points (`terminal.c`, `screen.c`, `launchpad.c`, `myts.c`) with a modern C++ stack (C++20 for host testing, C++17 embedded profile with `-fno-exceptions -fno-rtti` for ARMv6), adhering strictly to Test-Driven Development (`/tdd`), structured Doxygen docstrings, and isolated git worktrees.

---

## 2. Target Architecture & Seams

```
+-------------------------------------------------------------------------+
|                              myts (C++ Entry)                           |
+-------------------------------------------------------------------------+
       |                                      |                     |
       v                                      v                     v
+------------------+                 +------------------+   +------------------+
|  InputManager    |                 |   EventLoop      |   |   EinkDisplay    |
| (input/          |                 |  (core/          |   |  (graphics/      |
|  input_manager)  |                 |   event_loop)    |   |   eink_display)  |
+------------------+                 +------------------+   +------------------+
       |                                      |                     |
       | Key events                           | Poll descriptors    | Partial refresh
       v                                      v                     v
+-------------------------------------------------------+   +------------------+
|                 TerminalSession                       |   | HardwareEink     |
| (terminal/terminal_session.hpp)                       |   | (display/        |
| - PTY master/slave (UniqueFd)                         |   |  eink_driver)    |
| - Screen grid buffer (chars + attributes)             |   | - /dev/fb0       |
| - AnsiParser stream decoder (IAnsiHandler)            |   | - einkfb ioctl   |
| - Render to PixmapView using FontRenderer             |   +------------------+
+-------------------------------------------------------+
```

### Seam 1: Terminal Session & Screen Buffer (`terminal/terminal_session.hpp`)
- **Responsibility**: Manages the 2D grid of characters and attributes (rows × cols), cursor position, scrollback history, and PTY I/O.
- **Seam**:
  - Implements `IAnsiHandler` from `terminal/ansi_parser.hpp`.
  - Exposes `feed_pty_input(std::string_view data)`.
  - Exposes `char_at(int row, int col)`, `attr_at(int row, int col)`.
  - Exposes `render(graphics::PixmapView& dst, const graphics::FontRenderer& font)`.

### Seam 2: BDF / Hex Font Renderer (`graphics/font_renderer.hpp`)
- **Responsibility**: Parses Kindle hex bitmap fonts (`ter-u12n.hex`) into compact glyph bitmaps and renders them into 4bpp packed surfaces.
- **Seam**:
  - `load_hex_data(std::string_view hex_content)`.
  - `glyph(uint32_t codepoint) -> std::optional<GlyphBitmap>`.
  - `draw_char(graphics::PixmapView& dst, int x, int y, uint32_t codepoint, uint8_t fg, uint8_t bg)`.

### Seam 3: Input Manager & Key Translation (`input/input_manager.hpp`)
- **Responsibility**: Decodes Linux input events (`struct input_event`) from Kindle keypad, volume buttons, and 5-way joystick without hardware locks.
- **Seam**:
  - `process_event(const struct input_event& ev)`.
  - Key translation matrix: Shift, Alt/Fn, Sym modes.
  - Callback sink: `on_key_press(std::string_view key_sequence)`.

### Seam 4: Hardware E-Ink Driver (`display/hardware_eink_driver.hpp`)
- **Responsibility**: Concrete implementation of `IEinkDriver` interfacing with `/dev/fb0` and `FBIO_EINK_UPDATE_DISPLAY_AREA`.
- **Seam**:
  - Wraps framebuffer mapping with `core::MemoryMapping`.
  - Invokes `FBIO_EINK_UPDATE_DISPLAY_AREA` via `ioctl`.
  - Automatically falls back to mock buffer during host testing without Kindle hardware.

### Seam 5: Modern Application Entry (`app/application.hpp` & `main.cpp`)
- **Responsibility**: Top-level application coordinator linking `EventLoop`, `InputManager`, `TerminalSession`, and `EinkDisplay`. Replaces legacy `myts.c`.

---

## 3. Phased Implementation Plan (TDD Vertical Slices)

### Phase 1: Font Renderer (`graphics/font_renderer.hpp`)
- **Red Phase**: Write `tests/test_font_renderer.cpp`:
  - Test hex glyph string parsing (e.g., `"0041:00003c66667e66660000"` for `'A'`).
  - Test rendering glyph into a 4bpp `OwnedPixmap`.
- **Green Phase**: Implement `graphics/font_renderer.hpp` with Doxygen docstrings.
- **Verification**: `make test` & `make test-asan`.

### Phase 2: Terminal Screen Buffer & Session (`terminal/terminal_session.hpp`)
- **Red Phase**: Write `tests/test_terminal_session.cpp`:
  - Test cursor movement, character insertion, newline scrolling, and line wrap.
  - Test ANSI sequence handling via `AnsiParser` integration.
  - Test rendering grid to `PixmapView` via `FontRenderer`.
- **Green Phase**: Implement `terminal/terminal_session.hpp` with Doxygen docstrings.
- **Verification**: `make test` & `make test-asan`.

### Phase 3: Input Keycode & Modifier Translation (`input/input_manager.hpp`)
- **Red Phase**: Write `tests/test_input_manager.cpp`:
  - Feed synthetic `struct input_event` (press 'A', Shift+'A', 5-way navigation, volume keys).
  - Test escape sequence generation (`\033[A` for Up, etc.).
- **Green Phase**: Implement `input/input_manager.hpp` with Doxygen docstrings.
- **Verification**: `make test` & `make test-asan`.

### Phase 4: Modern Application Assembly (`main.cpp` & `app/application.hpp`)
- Assemble all subsystems into a single binary (`myts-ng` or `myts`).
- Update `Makefile` with clean dual host/target rules.
- Run complete test suite and sanitizers.

---

## 4. Verification & Testing

Every vertical slice must pass:
1. **Unit Tests**: `make test` on host platform (x86_64).
2. **Sanitizers**: `make test-asan` (AddressSanitizer + UndefinedBehaviorSanitizer).
3. **Embedded Constraint Check**: Compile with `-fno-exceptions -fno-rtti -std=c++17` to guarantee zero heavy C++ runtime dependencies on ARMv6.
4. **Git Workflow**: Execute in dedicated `.worktrees/` branches, rebase against `origin/master`, and merge via `--no-ff` directly without opening PRs.
