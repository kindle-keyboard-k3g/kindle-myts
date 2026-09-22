# Architecture & Subsystems

This document describes the high-level architecture of `kindle-myts` (both modern C++ `myts-ng` and legacy C `myts`), detailing subsystem responsibilities, communication paths, and test seams.

---

## 1. System Block Diagram

```
+-------------------------------------------------------------------------+
|                       Application / main (Entry Point)                  |
+-------------------------------------------------------------------------+
         |                                |                        |
         v                                v                        v
+------------------+             +------------------+     +------------------+
|   InputManager   |             |    EventLoop     |     |   EinkDisplay    |
| (input/          |             |  (core/          |     |  (graphics/      |
|  input_manager)  |             |   event_loop)    |     |   eink_display)  |
+------------------+             +------------------+     +------------------+
         |                                |                        |
         | ASCII/ANSI sequences           | Descriptors & Timers   | Partial/Full
         v                                v                        v
+---------------------------------------------------+     +------------------+
|                 TerminalSession                   |     | HardwareEink     |
| (terminal/terminal_session.hpp)                   |     | (display/        |
| - PTY master/slave (UniqueFd)                     |     |  eink_driver)    |
| - 2D Screen Grid Buffer (chars + attributes)      |     | - /dev/fb0 mmap  |
| - AnsiParser stream state machine (IAnsiHandler)  |     | - einkfb ioctls  |
| - Rasterization to Pixmap via FontRenderer        |     +------------------+
+---------------------------------------------------+
```

---

## 2. Core Subsystems

### 2.1 Event Loop Dispatcher (`core/event_loop.hpp`)
- **Role**: Replaces OS thread bloat with a single-threaded, asynchronous POSIX `select()` multiplexer.
- **Embedded Constraints**: Zero heap allocations during operation; fixed arrays `std::array<FdWatcher, 16>` and `std::array<TimerEntry, 8>`.
- **Interface**:
  - `register_read(int fd, IoCallback cb)`
  - `unregister(int fd)`
  - `set_timer(uint32_t delay_ms, TimerCallback cb)`
  - `step(int timeout_ms)` & `run(int poll_timeout_ms)`

### 2.2 Terminal Session & ANSI Decoder (`terminal/`)
- **Role**: Emulates VT100 / ANSI escape sequences, maintains screen matrices, and drives child processes inside a POSIX pseudo-terminal (PTY).
- **AnsiParser (`terminal/ansi_parser.hpp`)**: State machine implementing `IAnsiHandler` interface. Decodes CSI codes (`CUP`, `ED`, `EL`, `SGR`) without allocating strings.
- **TerminalSession (`terminal/terminal_session.hpp`)**:
  - Encapsulates master PTY descriptor in `core::UniqueFd`.
  - Maintains `std::vector<char>` character grid and attribute buffers.
  - Implements vertical scrolling when text exceeds rows.
  - Renders the grid into a target 4bpp surface using `FontRenderer`.

### 2.3 Graphics & Font Rendering Engine (`graphics/`)
- **Geometry (`graphics/geometry.hpp`)**: Minimal `Point` and `Rect` primitives with intersection, union, and clipping calculations.
- **Pixmap (`graphics/pixmap.hpp`)**: Packed 4bpp (2 pixels per byte, 16 grayscale levels). Manages nibble extraction (`get_pixel`) and insertion (`set_pixel`) with coordinate boundary checks.
- **FontRenderer (`graphics/font_renderer.hpp`)**: Decodes GNU Unifont / BDF hexadecimal character glyph lines (e.g. `0041:00003c66667e66660000`) and renders glyphs into 4bpp packed raster buffers.
- **EinkDisplay (`graphics/eink_display.hpp`)**: Tracks dirty rectangular bounding boxes, enforces even horizontal pixel alignment (required for byte-paired 4bpp packing), and flushes changes to `IEinkDriver`.

### 2.4 Hardware E-Ink Driver (`display/hardware_eink_driver.hpp`)
- **Role**: Concrete implementation of `graphics::IEinkDriver`.
- **Kindle Integration**: Opens `/dev/fb0`, retrieves screen geometry (`FBIOGET_VSCREENINFO`), maps memory with `core::MemoryMapping`, and issues `FBIO_EINK_UPDATE_DISPLAY_AREA` or `FBIO_EINK_UPDATE_DISPLAY` ioctl calls.
- **Host Testing Fallback**: Detects non-Kindle environments (failed `/dev/fb0` open or invalid ioctls) and automatically routes pixel operations to an in-memory buffer, allowing 100% test coverage on developer workstations.

### 2.5 Input Subsystem (`input/input_manager.hpp`)
- **Role**: Decodes `/dev/input/event*` Linux key events from Kindle hardware.
- **State Tracking**: Tracks active state of modifier keys (`Shift`, `Ctrl`, `Alt`, `Sym`).
- **Translation**: Translates raw hardware keycodes into ASCII characters, ANSI cursor navigation escapes (`\033[A`), or custom shortcut commands.

---

## 3. Data Flow

1. **Input Flow**:
   User presses a key → `/dev/input/event*` generates `struct input_event` → `InputManager::process_event()` translates keycode → ASCII/ANSI sequence written to PTY master descriptor.
2. **Subshell Processing**:
   Shell process running in slave PTY reads character and writes terminal output to PTY master.
3. **Event Loop Notification**:
   `EventLoop` detects PTY master is readable via `select()` → invokes read callback.
4. **Terminal Processing**:
   PTY output fed into `TerminalSession::feed_input()` → `AnsiParser` updates screen grid and cursor.
5. **Frame Rendering & E-Ink Flush**:
   `Application::render_frame()` renders grid into 4bpp canvas via `FontRenderer` → copies raster to `HardwareEinkDriver` → issues `FBIO_EINK_UPDATE_DISPLAY_AREA` ioctl for dirty regions.
