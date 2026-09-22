# Terminal Emulation & PTY Subsystem

This document covers the terminal emulation layer of `kindle-myts`: ANSI escape sequence decoding, 2D screen buffer management, and child process execution using POSIX pseudo-terminals (PTYs).

---

## 1. Terminal Session (`terminal/terminal_session.hpp`)

The `TerminalSession` class represents an active shell instance.

### 1.1 Screen Grid & Attributes
- **Dimensions**: Configurable grid (e.g. 24 rows × 80 columns).
- **Buffer Storage**:
  - `chars_`: Contiguous array of `rows * cols` characters (`char`).
  - `attrs_`: Contiguous array of `rows * cols` attribute bytes (`uint8_t` for foreground, background, inverse).
- **Cursor Tracking**: Maintains `(cursor_row, cursor_col)`.
- **Scrolling**: When the cursor advances past the final row, `scroll_up()` shifts all buffer rows upward by 1 and clears the bottom line.

---

## 2. ANSI Escape Sequence State Machine (`terminal/ansi_parser.hpp`)

The parser implements a zero-allocation streaming state machine based on the VT100 / ANSI CSI standard.

### 2.1 State Transitions

```
[STATE_TEXT]
    |
    | Receives '\033' (ESC)
    v
[STATE_ESC]
    |
    | Receives '['
    v
[STATE_CSI]
    |
    | Accumulates parameters (digits, ';')
    | Receives final command byte ('H', 'J', 'm', etc.)
    v
Dispatches to IAnsiHandler callback -> Returns to [STATE_TEXT]
```

### 2.2 Implemented Escape Commands

| Sequence | Name | Action in `TerminalSession` |
|---|---|---|
| `\033[H` or `\033[r;cH` | Cursor Position (CUP) | Moves cursor to `(r, c)` (1-indexed). |
| `\033[2J` | Erase in Display (ED) | Clears entire screen buffer and homes cursor. |
| `\033[0J` | Erase Below | Clears from cursor to end of screen. |
| `\033[K` | Erase in Line (EL) | Clears from cursor position to end of current line. |
| `\033[A`, `B`, `C`, `D` | Cursor Up/Down/Right/Left | Relative cursor stepping with boundary clamping. |
| `\033[m` or `\033[0m` | SGR Reset | Resets colors and reverse video attributes. |
| `\033[7m` | SGR Reverse Video | Swaps foreground and background colors. |

---

## 3. PTY Lifecycle & Process Management

`TerminalSession::spawn_pty(const char* shell)` handles the complete lifecycle of subshell spawning:

```
+-------------------------------------------------------------+
|                          Parent                             |
|  - openpty(&master_fd, &slave_fd, ...)                      |
|  - fork()                                                   |
|  - Closes slave_fd                                          |
|  - Stores master_fd in core::UniqueFd                       |
|  - Registers master_fd read watcher with EventLoop          |
+-------------------------------------------------------------+
                              |
                              | fork()
                              v
+-------------------------------------------------------------+
|                          Child                              |
|  - Closes master_fd                                         |
|  - login_tty(slave_fd) or dup2 to stdin/stdout/stderr       |
|  - setsid()                                                 |
|  - execvp(shell, ...)                                       |
+-------------------------------------------------------------+
```

### 3.1 RAII Descriptor Safety
PTY descriptors are managed using `core::UniqueFd`. When the `TerminalSession` object is destroyed or re-initialized, the master descriptor is closed automatically, triggering a `SIGHUP` in the subshell process and preventing orphaned processes.
