# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and Test Commands

- **Build binary (`myts`):** `make myts` (uses `CC ?= gcc`, with `-Os -Wall -Wextra`)
- **Run unit tests:** `make test` (builds and executes test binaries in `tests/`)
- **Run tests with sanitizers:** `make test-asan` (builds and runs test suite with AddressSanitizer and UndefinedBehaviorSanitizer: `-fsanitize=address,undefined -fno-omit-frame-pointer`)
- **Run a single test suite:**
  - Dynamic strings: `make tests/test_dynstring && tests/test_dynstring`
  - INI config: `make tests/test_config && tests/test_config`
  - Pixel/clipping operations: `make tests/test_pixop && tests/test_pixop`
- **Clean build and test artifacts:** `make clean`
- **Build distribution zip:** `make myts.zip`

## Architecture Overview

`kindle-myts` is an event-driven native terminal emulator designed for Kindle devices (Freescale i.MX353 ARMv6 @ 532 MHz running an Epson Broadsheet EPD controller):

- **Event Loop & Session Framework (`myts.c`, `myts.h`):**
  - Implements a central `select()`-based event dispatcher managing asynchronous descriptors and timers via `struct app`, `struct sess`, and `struct cb_args`.
  - Global application state is maintained in `__me` (`struct my_args`).
  - Supports multi-session multiplexing across up to 3 terminal shells.

- **Launchpad & Input Handling (`launchpad.c`):**
  - Manages Kindle keypad, 5-way controller, and volume key events via Linux input event subsystem (`/dev/input/event*`).
  - Handles key translation, modifier tracking (Shift, Ctrl, Sym, Fn), and Launchpad shortcuts.

- **Terminal Emulation Engine (`terminal.c`, `terminal.h`):**
  - Implements ANSI/VT100 escape sequence decoding (`do_csi`), character insertion/deletion, cursor positioning, and screen buffer management (`sh->page`, `sh->attributes`).
  - Supports UTF-8 and 8-bit codepages.

- **Display & Framebuffer Subsystem (`screen.c`, `screen.h`, `pixop.c`, `pixop.h`):**
  - Interfaces with the Kindle e-ink controller via `/dev/fb0` and Kindle-specific `ioctl` calls defined in `linux/einkfb.h` (`FBIO_EINK_UPDATE_DISPLAY`, `FBIO_EINK_UPDATE_DISPLAY_AREA`).
  - Handles 4 bpp packed pixel drawing, coordinate truncation (`c_truncate`), and font rendering (`get_char_pixmap`).
  - Note: Font tables use `void**` for 64-bit safe pointer indexing.

- **Dynamic Strings & Memory (`dynstring.c`, `dynstring.h`):**
  - Extensible buffer structure (`dynstr`) providing safe growth, formatted printing (`dsprintf`), truncation (`ds_truncate`), shifting (`ds_shift`), and read-only references (`ds_ref`).

- **Configuration Parser (`config.c`, `config.h`):**
  - INI-style configuration loader supporting sections, comments (`#` and `;`), quotation escaping, and file inclusion (`include = ...`).

- **Automated Test Suite (`tests/`):**
  - Uses a lightweight assertion framework in `tests/test_framework.h` covering core domain logic without external dependencies.
