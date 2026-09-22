# TDD Implementation Plan: Dedicated Debug Version & Diagnostics for `kindle-myts`

## 1. Context & Objectives

`kindle-myts` runs both as legacy C (`myts`) and modern C++17/20 (`myts-ng`) on Freescale i.MX353 ARMv6 Kindle hardware (Kindle 3 / Kindle DX) as well as developer host workstations.
Currently:
- The release binary compiles with `-Os -DNODEBUG -fno-exceptions -fno-rtti` to achieve minimal size (~23 KB stripped) and avoid performance degradation on the 532 MHz CPU.
- Debugging on Kindle hardware or during headless integration is difficult without structured runtime logging, frame refresh metrics, or dirty area diagnostics.
- The user requested a dedicated **Debug Version** planned using **Test-Driven Development (`/tdd`)** and **superpowers**, ensuring that debug instrumentation has zero cost in production release builds while providing rich diagnostics in debug builds.

This plan specifies the architecture and phased TDD slices to introduce:
1. A zero-allocation modern C++ logging & diagnostics framework (`core/logger.hpp`).
2. Performance & display telemetry metrics collector (`core/metrics.hpp`).
3. CLI argument parsing for debug options in `main.cpp` (`--debug`, `--log-level`, `--log-file`, `--metrics`, `--dry-run`).
4. Optional on-screen debug status bar / overlay rendering (`graphics/debug_overlay.hpp`).
5. Dedicated build targets (`myts-ng-dbg`, `myts-dbg`) and automated sanitizer test suites.

---

## 2. Architecture & Seams

```
+--------------------------------------------------------------------------+
|                       Application / main.cpp                             |
|  - Parses debug flags (--debug, --log-level, --log-file, --metrics)      |
|  - Configures global/injected Logger & MetricsCollector                  |
+--------------------------------------------------------------------------+
         |                                                 |
         v                                                 v
+------------------+                             +--------------------+
|  core::Logger    |                             |  core::Metrics     |
| - Timestamping   |                             | - Refresh counts   |
| - Log levels     |                             | - Bytes read/write |
| - Tagged domains |                             | - Dirty rect stats |
| - Stderr or File |                             | - Frame durations  |
+------------------+                             +--------------------+
         |                                                 |
         +------------------------+------------------------+
                                  |
                                  v
+--------------------------------------------------------------------------+
|                     graphics::DebugOverlay                               |
|  - Renders compact monospace telemetry banner onto 4bpp canvas:          |
|    "FPS: 12 | REFRESH: 45 | DIRTY: 120x80@(0,24) | PTY: 1.4KB"           |
|  - Stripped out completely when DEBUG is not defined                     |
+--------------------------------------------------------------------------+
```

### Seam 1: Diagnostic Logging Subsystem (`core/logger.hpp`)
- **Responsibility**: Zero-allocation formatted logging with configurable severity thresholds (`TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`), microsecond/millisecond timestamps, subsystem tags (`[PTY]`, `[EINK]`, `[INPUT]`, `[APP]`, `[LOOP]`), and dual sink support (stderr or log file).
- **Public Seam**:
  - `Logger::instance()` / dependency-injected `Logger`.
  - `set_level(LogLevel level)`.
  - `set_output_fd(int fd)` or `set_output_file(const char* filepath)`.
  - `log(LogLevel level, std::string_view tag, std::string_view message)`.
  - Macro wrappers `MYTS_LOG_DEBUG(tag, msg)`, `MYTS_LOG_INFO(tag, msg)` compiling to no-ops when `-DNODEBUG` is set.

### Seam 2: Performance & Frame Telemetry Metrics (`core/metrics.hpp`)
- **Responsibility**: Tracks cumulative and per-frame metrics for E-Ink screen flushes, dirty bounding boxes, key input counts, and PTY I/O byte counts.
- **Public Seam**:
  - `record_refresh(const graphics::Rect& dirty_area, bool full_flash)`.
  - `record_pty_read(size_t bytes)`, `record_pty_write(size_t bytes)`.
  - `record_input_event()`.
  - `snapshot() -> MetricsSnapshot`.
  - `format_summary(core::StringBuilder& out) const`.

### Seam 3: On-Screen Framebuffer Debug Overlay (`graphics/debug_overlay.hpp`)
- **Responsibility**: Renders diagnostic metrics directly onto the 4bpp display canvas (e.g. top or bottom 12px banner) using `FontRenderer` without corrupting terminal grid state.
- **Public Seam**:
  - `draw_overlay(OwnedPixmap& canvas, const FontRenderer& font, const MetricsSnapshot& metrics)`.

### Seam 4: CLI Debug Flag Integration & Application Wiring (`main.cpp`, `app/application.hpp`)
- **Responsibility**: Parses command line arguments and switches `Application` into debug mode, configuring file logging and metrics reporting.
- **Public Seam**:
  - `DebugConfig parse_args(int argc, char** argv)`.
  - `Application::enable_debug(const DebugConfig& cfg)`.

---

## 3. Phased TDD Implementation Plan (Red → Green → Refactor)

Following the `/tdd` skill rules:
- Red before green: author failing test suite before implementation.
- One vertical slice per cycle.
- Verify each slice with `make test` and `make test-asan`.

### Phase 1: Core Diagnostic Logger (`core/logger.hpp`)
- **Red Phase**:
  - Create `tests/test_logger.cpp`:
    - Test log level filtering (e.g., `DEBUG` suppressed when level is `INFO`).
    - Test formatted log entry formatting (timestamp + tag + level + message).
    - Test file output redirection via `UniqueFd`.
    - Test zero-allocation compile-time elimination when `NODEBUG` is defined.
- **Green Phase**:
  - Implement `core/logger.hpp` with structured Doxygen docstrings.
  - Add `tests/test_logger` to `Makefile` and run under AddressSanitizer.
- **Verification**: `tests/test_logger` passing 100%.

### Phase 2: Metrics Collector (`core/metrics.hpp`)
- **Red Phase**:
  - Create `tests/test_metrics.cpp`:
    - Test recording partial and full refreshes.
    - Test dirty area accumulator and bounding box expansion.
    - Test PTY byte count accounting.
    - Test `format_summary` output.
- **Green Phase**:
  - Implement `core/metrics.hpp` with Doxygen docstrings.
  - Add `tests/test_metrics` to `Makefile`.
- **Verification**: `tests/test_metrics` passing 100%.

### Phase 3: Visual Debug Overlay Renderer (`graphics/debug_overlay.hpp`)
- **Red Phase**:
  - Create `tests/test_debug_overlay.cpp`:
    - Test drawing status banner into 4bpp `OwnedPixmap`.
    - Test coordinate clipping at screen margins.
    - Test string formatting for metrics snapshot without heap allocations.
- **Green Phase**:
  - Implement `graphics/debug_overlay.hpp` with Doxygen docstrings.
  - Add `tests/test_debug_overlay` to `Makefile`.
- **Verification**: `tests/test_debug_overlay` passing 100%.

### Phase 4: CLI Configuration & Application Diagnostics Integration
- **Red Phase**:
  - Author test cases in `tests/test_application.cpp` for debug mode startup and metrics telemetry.
- **Green Phase**:
  - Update `main.cpp` to parse `--debug`, `--verbose`, `--log-file`, `--metrics`, `--dry-run`.
  - Wire `Logger`, `MetricsCollector`, and `DebugOverlay` into `app/application.hpp`.
  - Update `Makefile` to produce dedicated debug target:
    - `myts-ng-dbg`: Compiled with `-g3 -O0 -DDEBUG -UNDEBUG` and symbol tables.
    - `myts-dbg`: Legacy C compiled with `-g3 -O0 -DDEBUG -UNDEBUG`.
- **Verification**:
  - Run full test suite: `make test` & `make test-asan`.
  - Verify build targets: `make myts-ng-dbg` and `make myts-ng`.

---

## 4. Verification & Validation Plan

1. **Unit Testing (`make test`)**:
   - 17 total test suites (all 14 existing + `test_logger`, `test_metrics`, `test_debug_overlay`).
2. **Sanitizers (`make test-asan`)**:
   - AddressSanitizer + UndefinedBehaviorSanitizer running against all debug and release modules.
3. **Zero Overhead In Release (`make myts-ng`)**:
   - Verify that release binary `myts-ng` remains small (< 30 KB stripped) and has all debug macros compiled out.
4. **Git Workflow**:
   - Implement in clean vertical slices, commit directly to default branch (`master`), run full validation suite before push.
