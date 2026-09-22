# kindle-myts Technical Documentation

Welcome to the internal engineering documentation for `kindle-myts` and its modern modern C++ rewrite `myts-ng`.

## Overview & Architecture

`kindle-myts` is a native, ultra-low-overhead terminal emulator engineered for E-Ink Kindle devices (specifically Freescale i.MX353 ARMv6 @ 532 MHz platforms such as Kindle Keyboard / K3 and Kindle DX).

The repository maintains both:
1. **The Modern C++ Stack (`myts-ng`)**: An idiomatic, modular, zero-allocation C++17/C++20 stack organized into clean testable seams with RAII memory management, native 4bpp packed rasterization, and hardware fallback emulation.
2. **The Legacy Procedural C Engine (`myts`)**: The battle-tested 2010 implementation by Luigi Rizzo and Andy based on asynchronous `select()` loops and direct `linux/einkfb.h` driver ioctls.

---

## Documentation Index

The documentation in `docs/` is divided into functional technical guides:

- **[Architecture & Subsystems](architecture.md)**
  High-level architectural overview, component hierarchy, data-flow pipelines, and clean seam boundaries.
- **[E-Ink Hardware & Display Subsystem](hardware-eink.md)**
  Deep dive into `/dev/fb0`, 4bpp packed pixel nibbles, Broadsheet EPD controller ioctls (`FBIO_EINK_UPDATE_DISPLAY_AREA`), partial/full waveform updates, and host mock fallback.
- **[Terminal Emulation & PTY Engine](terminal-pty.md)**
  Grid character matrices, ANSI/VT100 CSI state machine, cursor tracking, scrollback history, and POSIX pseudo-terminal lifecycle.
- **[Input Subsystem & Key Translation](input-subsystem.md)**
  Linux input event subsystem (`/dev/input/event*`), Kindle keypad mapping, modifier states (`Shift`, `Ctrl`, `Alt`, `Sym`), and 5-way joystick navigation.
- **[BDF / Hex Font Renderer](font-rendering.md)**
  Glyph parsing from GNU Unifont / BDF hex format, glyph raster caching, and 4bpp direct drawing algorithms.
- **[Configuration System](configuration.md)**
  INI file syntax, section hierarchy, file inclusion (`include = ...`), escape rules, and legacy vs modern C++ parser equivalence.
- **[Building & Testing Guide](build-and-test.md)**
  Compiler flags, embedded profile optimization (`-fno-exceptions -fno-rtti`), AddressSanitizer testing, cross-compilation for ARMv6, and packaging.
- **[Architectural Decision Records (ADR)](adr/README.md)**
  Design rationale and trade-offs behind modernizing to C++17/C++20, RAII encapsulation, zero dynamic allocation inner loops, and dual-stack coexistence.
- **[Source of Truth (SoT)](sot/README.md)**
  Authoritative references on memory layouts, color palettes, and Kindle hardware registers.

---

## Directory Structure

```
kindle-myts/
├── app/                  # Application orchestrator and top-level coordinator
├── core/                 # RAII primitives, buffers, and select() event loop
├── display/              # Hardware eink driver & ioctl integration
├── docs/                 # Detailed technical documentation
│   ├── adr/              # Architecture Decision Records
│   ├── plans/            # Engineering roadmaps & implementation plans
│   └── sot/              # Source of truth specifications
├── graphics/             # 4bpp pixmap, clipping, font renderer, display flush
├── input/                # Linux input_event translation & modifier state
├── linux/                # E-Ink Linux framebuffer kernel headers
├── terminal/             # ANSI state machine, grid buffer, PTY subshell
├── tests/                # Automated unit tests and AddressSanitizer harness
├── Makefile              # Build automation for host tests and target binaries
└── main.cpp              # Modern C++ CLI entry point
```
