# kindle myts

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Kindle%20Keyboard%20(K3%2FK3G)%20%7C%20DX-lightgrey.svg)](#hardware--platform-support)
[![Build & Test](https://img.shields.io/badge/Tests-Passing%20(Unit%20%26%20ASan)-brightgreen.svg)](#building-and-testing)

An event-driven, high-performance native terminal emulator designed for Amazon Kindle devices (Kindle Keyboard 3/3G and Kindle DX Graphite). Forked and modernized from Luigi Rizzo's [`myts` / `kiterm`](http://info.iet.unipi.it/~luigi/kindle/) with optimizations for e-ink refresh cycles, customizable keyboard mappings, multi-session multiplexing, and robust 64-bit safe architecture.

---

## Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Hardware & Platform Support](#hardware--platform-support)
- [Architecture](#architecture)
- [Installation & Setup](#installation--setup)
- [Usage & Keybindings](#usage--keybindings)
  - [Opening & Managing Terminals](#opening--managing-terminals)
  - [Special Keys & Navigation](#special-keys--navigation)
  - [Keyboard Layouts](#keyboard-layouts)
- [Configuration](#configuration)
  - [Core Settings (`myts.ini`)](#core-settings-mytsini)
  - [Custom Fonts](#custom-fonts)
- [Building and Testing](#building-and-testing)
  - [Prerequisites](#prerequisites)
  - [Host Testing (x86_64 / Linux)](#host-testing-x86_64--linux)
  - [Sanitizer Checks](#sanitizer-checks)
  - [Cross-Compilation for Kindle (ARMv6)](#cross-compilation-for-kindle-armv6)
  - [Release Packaging](#release-packaging)
- [Troubleshooting & FAQs](#troubleshooting--faqs)
- [Credits & Acknowledgments](#credits--acknowledgments)
- [License](#license)

---

## Overview

`kindle-myts` transforms e-ink Kindle devices into usable, lightweight, low-distraction terminals for SSH sessions, text editing, monitoring, and remote development. It interacts directly with the Linux framebuffer (`/dev/fb0`), the Kindle e-ink controller (`einkfb`), and Linux input event subsystems (`/dev/input/event*`), completely bypassing the heavy stock Java GUI framework.

### Why `kindle-myts`?

- **Direct Framebuffer Rendering:** Operates at 4 bits per pixel (16 grayscale levels) with custom font glyph rasterization and coordinate truncation.
- **Ultra-Lean Resource Usage:** Runs comfortably within the 128 MB RAM envelope of the Kindle 3, using negligible memory and CPU overhead.
- **64-bit Safe & Modernized:** Fully compatible with both 32-bit ARM targets and 64-bit host test runners without pointer slicing or alignment faults.
- **Integrated Unit Tests:** Backed by unit and AddressSanitizer test suites covering dynamic string management, INI configuration parsing, and pixel clipping operations.

---

## Key Features

- **Multi-Session Terminal Multiplexing:** Run up to 3 independent shell sessions concurrently, switching instantly with hotkeys.
- **Comprehensive Encoding Support:** Full UTF-8 support alongside configurable 8-bit codepages (including CP437 and CP1255).
- **Extensible Configuration:** Full INI configuration (`myts.ini`) supporting file inclusion (`include = keydefs.ini`), custom symbol overlays, keycode remapping, and scrollback depth.
- **E-Ink Refresh Optimization:** Hardware-level display refresh controls via `ioctl` interfaces to balance redraw speed and eliminate e-ink ghosting.
- **Customizable Typography:** Monospaced bitmap font rendering from hex/bdf font definitions with configurable font metrics and character offsets.
- **Full Launchpad Integration:** Integrates seamlessly into the Kindle Launchpad extension for fast launching and daemon supervision.

---

## Hardware & Platform Support

| Device | Model / Code | SoC / Architecture | Display Panel | Status |
|---|---|---|---|---|
| **Kindle Keyboard 3 (Wi-Fi)** | D00901 / K3W | Freescale i.MX353 (ARMv6 @ 532 MHz) | 6" E-Ink Pearl, 600×800 (16-gray) | Fully Supported |
| **Kindle Keyboard 3G (Cellular)** | D01001 / K3G | Freescale i.MX353 (ARMv6 @ 532 MHz) | 6" E-Ink Pearl, 600×800 (16-gray) | Fully Supported |
| **Kindle DX / DX Graphite** | B004, B005, B009 | Freescale i.MX31 / i.MX35 (ARMv6) | 9.7" E-Ink Pearl, 824×1200 | Supported via `[INKEYS-DX]` |

---

## Architecture

`kindle-myts` is structured into clean modular subsystems:

```
kindle-myts/
├── myts.c, myts.h         # select()-based event loop and multi-session coordinator
├── launchpad.c            # Linux input event handling (/dev/input), hotkeys, keycode mapping
├── terminal.c, terminal.h # ANSI/VT100 escape parser (CSI state machine), cursor, screen buffer
├── screen.c, screen.h     # Framebuffer (/dev/fb0) abstraction & Kindle einkfb ioctl interface
├── pixop.c, pixop.h       # 4bpp packed pixel rasterization, coordinate clipping, blitting
├── font.c, font.h         # Hex/bdf font loading, glyph cache, 64-bit safe pointer lookup
├── dynstring.c, dynstring.h # Dynamic extensible string buffers with formatted printing
├── config.c, config.h     # INI parser with section inheritance and file inclusion
└── tests/                 # Unit test suite & AddressSanitizer harness
```

---

## Installation & Setup

### Prerequisites on Kindle

1. **Jailbreak:** Your Kindle must be jailbroken.
2. **Launchpad:** Install the [Kindle Launchpad](https://wiki.mobileread.com/wiki/Launchpad) hack.
3. **USB Networking / SSH (Recommended):** Helpful for transferring files and running shell commands.

### Automated Installation & Update (Linux & WSL)

Connect your Kindle via USB (either with USBNetwork active or mounted as a USB drive) and run:

```bash
# 1. One-command automated deployment (auto-detects SSH or mounted USB storage):
make deploy

# Or run the script directly:
./scripts/update-kindle.sh
```

#### Specific Deployment Modes

```bash
# Deploy over SSH using your ~/.ssh/config 'kindle' alias:
./scripts/update-kindle.sh --ssh kindle

# Deploy over SSH directly using Kindle USBNetwork IP:
./scripts/update-kindle.sh --ssh 192.168.2.2

# Deploy to a Kindle mounted as a USB drive (e.g. drive E: in WSL):
./scripts/update-kindle.sh --usb /mnt/e

# Reset myts.ini to defaults during deployment:
./scripts/update-kindle.sh --reset-config
```

The script is **completely idempotent**:
- Stops active terminal sessions on the Kindle to avoid file-locking (`ETXTBSY`) errors.
- Automatically preserves existing custom `myts.ini` configurations.
- Backs up the legacy binary to `myts-legacy` without overwriting earlier backups.
- Updates `/mnt/us/launchpad/myts.ini` and triggers a Launchpad reload (`killall -HUP launchpad`).
- Executes a post-deployment sanity test (`myts --dry-run`).

---

### Manual Installation Steps (Alternative)

---

## Architecture & Binary Layout

Starting with v2+, `myts-ng` is the default modern C++ terminal engine on the Kindle:

- **`myts` (`tools/myts`)**: The default executable entry point invoked by Launchpad shortcuts (`Shift + T, T`) or shell. It invokes `launch_kindle.sh` to manage Kindle framework and watchdog daemons.
- **`myts-ng`**: The modern C++17 native terminal emulator binary compiled statically with musl libc for ARMv6. Provides auto-fit full-screen geometry ($66 \times 75$ characters on $600 \times 800$), zero-flash partial e-ink updates, dirty row caching, and bold font synthesis.
- **`launch_kindle.sh`**: Safely suspends Amazon's `framework` and `pmond` watchdog daemons during terminal sessions and restores them on exit, avoiding screen fighting and watchdog reboot timeouts.
- **`myts-legacy`**: The original 2010 C implementation preserved for backward compatibility and testing.

---

## Usage & Keybindings

### Opening & Managing Terminals

All terminal sessions are invoked via Launchpad hotkeys:

| Key Sequence | Action |
|---|---|
| <kbd>Shift</kbd> + <kbd>T</kbd>, then <kbd>T</kbd> | Open or switch to **Terminal 1** |
| <kbd>Shift</kbd> + <kbd>T</kbd>, then <kbd>Y</kbd> | Open or switch to **Terminal 2** |
| <kbd>Shift</kbd> + <kbd>T</kbd>, then <kbd>U</kbd> | Open or switch to **Terminal 3** |
| <kbd>Shift</kbd> + <kbd>T</kbd>, then <kbd>A</kbd> | Terminate all active `myts` processes |

### Special Keys & Navigation

Default bindings inside an active terminal session:

| Action | Default Kindle Key | Config Key (`myts.ini`) |
|---|---|---|
| **Exit / Detach Session** | <kbd>Right &lt;</kbd> (Top-right page turn) | `TermEnd` |
| **Send Escape (`\033`)** | <kbd>Right &gt;</kbd> (Bottom-right page turn) | `TermEsc` |
| **Send Control Modifier** | <kbd>aA</kbd> (Font size key) | `TermCtrl` |
| **Scroll Up** | <kbd>Left &lt;</kbd> (Top-left page turn) | `TermScrollup` |
| **Scroll Down** | <kbd>Left &gt;</kbd> (Bottom-left page turn) | `TermScrolldown` |
| **Home Key** | <kbd>Home</kbd> | `TermHome` |
| **Function Keys Overlay** | <kbd>Menu</kbd> | `TermFn` |
| **Symbol Layout** | <kbd>Back</kbd> / <kbd>Sym</kbd> | `TermSym` |

### Keyboard Layouts

#### Menu Layer (<kbd>Menu</kbd> Key)
Accesses function keys and common shell delimiters:
```
Row 1:  F1   F2   F3   F4   F5   F6   F7   F8   F9   F10
Row 2:  `    %    ^    <    >    [    ]    =    F11  F12
Row 3:  \t   ;    ,    (    )    {    }
```

#### Symbols Layer (<kbd>Back</kbd> / <kbd>Sym</kbd> Key)
Accesses programming punctuation and special characters:
```
Row 1:  !    @    #    $    %    ^    &    *    (    )
Row 2:  '    +    #    -    _    (    )    &    !    ?
Row 3:  ~    $    |    /    \    "    '    :
```
*Note: The symbol layout can be customized via the `Symbols` property in `myts.ini`.*

---

## Configuration

All runtime behaviors and visual options are managed in `/mnt/us/myts/myts.ini`.

### Core Settings (`myts.ini`)

```ini
[Settings]
# Navigation and Control Keys
TermEnd        = Right<      ; Key to detach/exit from terminal
TermEsc        = Right>      ; Key to emit ANSI escape
TermCtrl       = aA          ; Control modifier key
TermSym        = Back        ; Symbols menu trigger
TermScrollup   = Left<       ; Scrollback history up
TermScrolldown = Left>       ; Scrollback history down

# Typography and Dimensions
Font           = ter-u12n.hex ; Hex font file
FontWidth      = 6           ; Font character width in pixels
FontHeight     = 12          ; Font character height in pixels
XOffset        = 4           ; Screen margin offset X
YOffset        = 4           ; Screen margin offset Y
Encoding       = UTF8        ; Character encoding (UTF8 or iconv codepage)
ScrollbackLines= 1000        ; Number of lines preserved in scrollback

# Refresh and Input
RefreshDelay   = 50          ; Screen refresh throttle (milliseconds)
KpadIn         = /dev/input/event0
FwIn           = /dev/input/event1
VolIn          = /dev/input/event2

# Include hardware-specific keycode definitions
include        = keydefs.ini
```

### Custom Fonts

`kindle-myts` reads fonts formatted in plain ASCII hexadecimal representation (`.hex`).
To convert BDF bitmap fonts to the supported hex format, use the included Perl utility:
```bash
./bdf2hex my_font.bdf > my_font.hex
```

---

## Building and Testing

The project includes build and test automation for both host machines and target ARM environments.

### Prerequisites

- GCC / Clang with C99 support
- GNU Make
- AddressSanitizer and UndefinedBehaviorSanitizer libraries (`libasan`, `libubsan`)
- Optional for packaging: `zip`
- Optional for cross-compilation: ARMv6 toolchain (e.g., `arm-linux-gnueabi-gcc` or `musl-cross`)

### Host Testing (x86_64 / Linux)

Run the automated test suite natively on your development workstation:
```bash
make test
```
This builds and verifies all test suites:
- `tests/test_dynstring.c`: String allocation, growth, formatting, shifting, and references.
- `tests/test_config.c`: INI configuration parsing, whitespace trimming, and comment handling.
- `tests/test_pixop.c`: Coordinate clipping, truncation, and bounds checking.

### Sanitizer Checks

Run the entire test suite with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) enabled:
```bash
make test-asan
```

### Cross-Compilation for Kindle (ARMv6)

To build the standalone binary for the Kindle i.MX353 target using an ARM toolchain:
```bash
CC=arm-linux-gnueabi-gcc make myts
```
For statically-linked musl binaries:
```bash
CC=arm-linux-musleabi-gcc CFLAGS="-Os -static -Wall -Wextra" make myts
```

### Release Packaging

Generate the ready-to-deploy `myts.zip` distribution package:
```bash
make myts.zip
```

---

## Troubleshooting & FAQs

#### 1. Why does nothing happen when pressing <kbd>Shift</kbd> <kbd>T</kbd> <kbd>T</kbd>?
- Ensure Launchpad is running (`Shift Shift Space` to restart).
- Verify that `launchpad/myts.l.ini` exists and points to `/mnt/us/myts/myts`.
- Ensure `/mnt/us/myts/myts` has executable permissions (`chmod +x /mnt/us/myts/myts` via SSH).

#### 2. How can I reduce e-ink ghosting?
- Adjust the `RefreshDelay` setting in `myts.ini`.
- You can force a full waveform screen refresh by invoking the full-refresh trigger configured in your terminal session.

#### 3. How do I stop the Amazon Kindle framework to save memory and battery?
Connect via SSH or serial console and execute:
```bash
/etc/init.d/framework stop
```
To restore the default Amazon home screen interface later:
```bash
/etc/init.d/framework start
```

---

## Credits & Acknowledgments

- **Luigi Rizzo:** Original author of [`kiterm` / `myts`](http://info.iet.unipi.it/~luigi/kindle/).
- **MobileRead Community:** Valuable contributions, patches, and feedback from community members `PoP`, `Xqtftqx`, and `dsmid`.
- **GNU Unifont & Markus Kuhn:** Bitmap font definitions and BDF font tooling.

---

## License

This project is licensed under the **GNU General Public License v3.0** (GPLv3). See the [`LICENSE`](LICENSE) file for details.
