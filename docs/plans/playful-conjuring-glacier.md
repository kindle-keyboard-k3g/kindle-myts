# Implementation Plan: Interactive Help Screen & Keyboard Layout Viewer (`myts-ng`)

## 1. Context

`myts-ng` runs directly on Kindle devices with non-standard hardware keyboards (Kindle Keyboard 3/3G and Kindle DX Graphite). These devices have compact 4-row physical keyboards with multi-layer keychords:
- Physical letters and digits
- **Shift layer** for uppercase letters and standard punctuation
- **Sym layer** for programming symbols (`!@#$%^&*()*+#-_()&!?~$|/\"':`)
- **Menu/Fn layer** for function keys (F1–F12) and special shell delimiters
- Specialized navigation keys: top/bottom page-turn buttons (`Right<`, `Right>`, `Left<`, `Left>`), `Home`, `Back`, `aA` (Ctrl modifier), and the 5-way D-Pad.

Users need an on-demand, interactive help screen accessible at any time during a terminal session:
1. Typing `help` followed by Enter (or pressing the Kindle hardware `Menu` key / `Shift+H`) should open the help interface.
2. The help screen displays an interactive physical keyboard layout, layer maps (Standard/Shift, Sym, Menu/Fn), and general terminal shortcuts.
3. The interface reflects live key presses and allows seamless tab navigation using number keys `1`–`4` or the 5-way D-Pad.
4. Pressing <kbd>q</kbd>, <kbd>Enter</kbd>, <kbd>Back</kbd>, <kbd>Esc</kbd> (`Right>`), or the help trigger exits the help screen cleanly and restores the active terminal display without ghosting.

---

## 2. Requirements & Architectural Constraints

- **Language & Embedded Constraints**: Modern C++17 (`-std=c++17 -Os -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections`).
- **Memory & Allocation Rules**: Zero dynamic allocations on the input, event-routing, and rendering hot paths. Fixed-capacity buffers, stack structs, and string views only.
- **Object Calisthenics & Clean Code**:
  - One indent level per method; guard clauses and early returns (no `else`).
  - Small, focused classes (≤100 lines) with ≤2 member variables per class.
  - Functions ≤15 lines.
  - Stateless renderers; tell-don't-ask interfaces.
- **PTY Isolation**: When the help screen is active, keystrokes are intercepted and consumed by the help system; no input is forwarded to the PTY. Terminal session continues buffering incoming PTY data in memory without redrawing canvas until help exits.
- **Clean Display Restoration**: Exiting help screen triggers full terminal redraw (`TerminalSession::mark_all_dirty()`) and hardware full e-ink refresh (`EinkDisplay::refresh_full()`).

---

## 3. Component Architecture & Class Decomposition

```
help/
├── help_types.hpp            # Value objects & enums (HelpPage, HelpRoute, KeySnapshot)
├── help_key_catalog.hpp      # Single source of truth for keycodes, layers, labels, and geometry
├── help_command_tracker.hpp  # Zero-allocation observer for typed "help\r" sequence
├── help_screen.hpp           # State container for active mode & navigation (≤2 members)
├── help_renderer.hpp         # Stateless full and delta canvas rasterizer (FontRenderer/OwnedPixmap)
└── help_controller.hpp       # Coordinates Screen & Tracker; interfaces with Application (≤2 members)

input/
└── key_catalog.hpp           # Physical Kindle keycodes & modifier constants

terminal/
└── terminal_session.hpp      # Enhanced with mark_all_dirty() for full restore

app/
└── application.hpp           # Input routing integration & display coordination
```

### 3.1 Domain Types (`help/help_types.hpp`)
- `enum class HelpPage : uint8_t { Overview = 0, Keypad = 1, Sym = 2, Fn = 3 };`
- `enum class HelpRoute : uint8_t { Pass, Consume, OpenAfterWrite, Redraw, Exit };`
- `struct KeySnapshot { uint16_t code{0}; char character{'\0'}; uint8_t modifiers{0}; };`
- `struct HelpNavigationState { HelpPage page{HelpPage::Overview}; KeySnapshot last_key{}; };`

### 3.2 Key Catalog (`help/help_key_catalog.hpp` & `input/key_catalog.hpp`)
Static/constexpr definitions shared across InputManager and HelpScreen:
- Kindle 3 & DX hardware keycodes: Menu (`139`), Back (`158` / `91`), Right< (`109`), Right> (`191` / `124`), Left< (`193`), Left> (`104`), aA/Ctrl (`190` / `90`), Sym (`126` / `94`), 5-way arrows (`103`, `108`, `105`, `106`), Select (`194` / `92`).
- Physical row layout:
  - Row 1: `Q W E R T Y U I O P` (Shift: `! @ # $ % ^ & * ( )`)
  - Row 2: `A S D F G H J K L Del`
  - Row 3: `Z X C V B N M . / Enter`
  - Control row: `Shift`, `Ctrl (aA)`, `Sym (Back)`, `Menu`, `Space`
- Sym Layer symbols: `!@#$%^&*()*+#-_()&!?~$|/\"':`
- Menu/Fn Layer mappings: `F1`–`F10` on Row 1, `F11`–`F12` + punctuation on Row 2 & 3.

### 3.3 Command Tracker (`help/help_command_tracker.hpp`)
- Fixed 8-byte stack ring/buffer tracking line input.
- Detects the exact typed sequence `help` followed by Enter (`\r`).
- Ignores prefixes (`echo help`, `helper`) and resets on control/escape codes or backspace.
- Strictly zero runtime allocation.

### 3.4 Help Screen (`help/help_screen.hpp`)
- Exactly 2 member variables:
  ```cpp
  bool active_{false};
  HelpNavigationState nav_{};
  ```
- Methods: `open()`, `close()`, `set_page(HelpPage)`, `record_key(KeySnapshot)`.

### 3.5 Help Controller (`help/help_controller.hpp`)
- Exactly 2 member variables:
  ```cpp
  HelpScreen screen_{};
  HelpCommandTracker tracker_{};
  ```
- Manages routing in `Application::handle_input_event()`:
  - `before_terminal_write()`: intercepts Menu key (`139`), `Shift+H`, or active help navigation.
  - `after_terminal_write()`: detects typed `help\r` to open help after shell receives command.
  - Handles exit keys (<kbd>q</kbd>, <kbd>Enter</kbd>, <kbd>Back</kbd>, <kbd>Right></kbd>, <kbd>Menu</kbd>).

### 3.6 Stateless Renderer (`help/help_renderer.hpp`)
- Uses existing `graphics::FontRenderer::draw_char()` and `graphics::OwnedPixmap::set_pixel()`.
- Renders:
  - Top tab header: `[1] Overview   [2] Keypad   [3] Sym   [4] Menu/Fn`
  - Active page content (Overview shortcuts, visual ASCII keyboard with active key highlighted, Sym table, or Fn table)
  - Bottom status bar: `Key: [code] -> [action] | Press Back/Right>/q/Enter to Exit`
- Two render pathways:
  - `render_full()`: Clears canvas and renders entire help page (on open or tab switch).
  - `render_delta()`: Re-renders only changed key box and status line for partial e-ink refresh.

---

## 4. Integration with `app/application.hpp`

1. **Input Interception**:
   - `handle_input_event(ev)`:
     - Feeds `ev` to `input_.process_event(ev)`.
     - Queries `help_.before_terminal_write(ev, input_.modifiers(), seq)`.
     - If `HelpRoute::Consume` or `HelpRoute::Exit`: consumes event, updates help screen, does NOT write to PTY.
     - If `HelpRoute::Redraw`: draws help frame, flushes to display.
     - If `HelpRoute::Pass`: writes `seq` to PTY, then feeds `help_.after_terminal_write(seq)` (opening help if `help\r` was entered).
2. **PTY Suppression**:
   - In PTY read callback: if `help_.active()`, reads and feeds data to `session_.feed_input(data)` (buffering) but bypasses `render_frame()` to keep the help screen visible.
3. **Exit & Canvas Restoration**:
   - On help exit:
     - `session_.mark_all_dirty();`
     - `session_.render(canvas_, font_, true, nullptr);`
     - `driver_.copy_surface(canvas_);`
     - `display_.refresh_full();`

---

## 5. Verification & Testing Plan

1. **Unit Test Suites**:
   - `tests/test_help_command_tracker.cpp`:
     - Verifies exact `help\r` trigger.
     - Verifies non-triggers: `helper\r`, `echo help\r`, `ahelp\r`.
     - Verifies backspace, control sequences, and buffer overflow safety.
   - `tests/test_help_navigation.cpp`:
     - Verifies default page is `Overview`.
     - Verifies numeric key navigation (`1`..`4`) and 5-way D-Pad wrapping (Left/Right).
     - Verifies exit triggers (<kbd>q</kbd>, <kbd>Enter</kbd>, <kbd>Back</kbd>, <kbd>Right></kbd>, <kbd>Menu</kbd>).
   - `tests/test_help_renderer.cpp`:
     - Verifies all 4 pages render without bounds violations.
     - Verifies `render_full()` and `render_delta()` return valid clipped Rects.
     - Verifies zero memory allocations during rendering.
   - `tests/test_application.cpp`:
     - Verifies modal help lifecycle: enter via Menu key, suppression of PTY writes, and full screen restoration on exit.

2. **Sanitizer Verification**:
   - Run `make test-asan` covering AddressSanitizer and UndefinedBehaviorSanitizer across all suites.

3. **Kindle Hardware / Cross-Compilation Verification**:
   - Cross-compile with `armv6-linux-musleabi-g++` (`make myts-ng-kindle`).
   - Run `./scripts/update-kindle.sh --ssh kindle` to deploy to connected device.
   - Verify typing `help` in terminal opens help screen on device.
   - Verify pressing `Menu` toggles help screen.
   - Verify navigating pages with 5-way D-Pad and exiting with `q` or `Back`.
