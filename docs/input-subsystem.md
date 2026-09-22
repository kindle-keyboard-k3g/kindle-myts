# Input Subsystem & Key Translation

This document details the Linux input event subsystem integration and keycode translation logic in `kindle-myts`.

---

## 1. Hardware Input Devices

Kindle devices expose physical buttons and keypads via standard Linux input device nodes (`/dev/input/event*`):

- **Kindle Keyboard (K3)**: Full physical QWERTY keyboard, two Shift keys, Sym button, Del, Return, 5-way directional joystick with center click, and side Page Up / Page Down buttons.
- **Kindle DX**: Large physical keyboard with numerical row and 5-way controller.

---

## 2. Event Processing Pipeline (`input/input_manager.hpp`)

The `InputManager` processes raw `struct input_event` structures read from `/dev/input/event*`:

```
Linux /dev/input/event*
         |
         v
struct input_event { type, code, value }
         |
         v
+-------------------------------------------------------------+
|                     InputManager                            |
| 1. Filter: type == EV_KEY                                   |
| 2. State: Update modifiers if code is Shift, Ctrl, Alt, Sym |
| 3. Translate: Look up keycode in translation table          |
| 4. Output: Generate ASCII char or ANSI escape sequence      |
+-------------------------------------------------------------+
         |
         v
std::string_view (Written to PTY master descriptor)
```

---

## 3. Modifier Keys and Shift States

`InputManager` tracks modifier states using `ModifierState`:

```cpp
struct ModifierState {
    bool shift{false};
    bool ctrl{false};
    bool alt{false};
    bool sym{false};
};
```

When an `EV_KEY` event with value `1` (press) or `0` (release) is received for modifier keycodes:
- `KEY_LEFTSHIFT` / `KEY_RIGHTSHIFT`: Toggles `shift`.
- `KEY_LEFTCTRL` / `KEY_RIGHTCTRL`: Toggles `ctrl`.
- `KEY_LEFTALT` / `KEY_RIGHTALT`: Toggles `alt`.
- `KEY_COMPOSE` (Kindle Sym key): Toggles `sym`.

---

## 4. Keycode Mapping Table

### 4.1 Letters and Numerical Row
- Without Shift: emits lowercase `a`..`z`.
- With Shift: emits uppercase `A`..`Z`.
- With Ctrl: emits ASCII control characters (`0x01` for `Ctrl+A` through `0x1A` for `Ctrl+Z`).

### 4.2 Special and Navigation Keys

| Keycode | Physical Button | Output Sequence |
|---|---|---|
| `KEY_UP` | 5-way Up | `\033[A` |
| `KEY_DOWN` | 5-way Down | `\033[B` |
| `KEY_RIGHT` | 5-way Right | `\033[C` |
| `KEY_LEFT` | 5-way Left | `\033[D` |
| `KEY_ENTER` | Return / 5-way Click | `\r` (`0x0D`) |
| `KEY_BACKSPACE` | Delete key | `\x7f` or `\b` |
| `KEY_SPACE` | Space bar | ` ` (`0x20`) |
| `KEY_PAGEUP` | Left Next Page button | `\033[5~` |
| `KEY_PAGEDOWN` | Right Next Page button | `\033[6~` |

---

## 5. Zero-Allocation Design

The translation pipeline uses fixed internal stack buffers (`char seq_buf_[8]`) and returns a `std::string_view`. No heap allocations take place during keyboard event handling, guaranteeing deterministic, low-latency response times on 532 MHz ARMv6 processors.
