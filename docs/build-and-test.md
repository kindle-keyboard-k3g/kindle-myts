# Building and Testing Guide

This document describes how to build, test, and cross-compile `kindle-myts` and `myts-ng`.

---

## 1. Prerequisites

- **C Compiler**: GCC or Clang supporting C99/C11.
- **C++ Compiler**: GCC (>= 10) or Clang (>= 11) supporting C++17 and C++20.
- **Build Tools**: GNU Make, `strip`, `zip`.
- **Operating System**: Linux (Ubuntu 20.04+, Debian 11+, or similar).

---

## 2. Common Build Commands

| Target | Command | Description |
|---|---|---|
| Build Modern Binary | `make myts-ng` | Builds stripped C++17 embedded executable. |
| Build Legacy Binary | `make myts` | Builds stripped legacy C executable. |
| Run Test Suite | `make test` | Builds and executes 14 unit test suites (44 tests). |
| Run Sanitizers | `make test-asan` | Runs full test suite under AddressSanitizer and UBSan. |
| Package Release | `make myts.zip` | Creates distribution zip package. |
| Clean Build | `make clean` | Removes all object files and test binaries. |

---

## 3. Embedded Profile Compilation Flags

To guarantee zero overhead on ARMv6 (Freescale i.MX353, 128 MB RAM), `myts-ng` is compiled with strict embedded profile flags:

```makefile
CXXFLAGS = -std=c++17 -Os -Wall -Wextra \
           -fno-exceptions -fno-rtti \
           -ffunction-sections -fdata-sections \
           -fno-unwind-tables -fno-asynchronous-unwind-tables
```

- `-Os`: Optimize for minimum code size (maximizes instruction cache hits on 532 MHz CPU).
- `-fno-exceptions -fno-rtti`: Strips exception tables and runtime type information, reducing binary footprint to ~23 KB.
- `-ffunction-sections -fdata-sections`: Enables dead-code stripping during linking.
- `-fno-unwind-tables`: Eliminates `.eh_frame` DWARF unwinding tables.

---

## 4. Cross-Compilation for Kindle (ARMv6)

To build binaries for physical Kindle devices using an `arm-linux-gnueabi` toolchain:

```bash
export CC=arm-linux-gnueabi-gcc
export CXX=arm-linux-gnueabi-g++
export STRIP=arm-linux-gnueabi-strip

make clean
make myts-ng
make myts
make myts.zip
```

---

## 5. AddressSanitizer & UndefinedBehaviorSanitizer

All unit test suites are instrumented with `-fsanitize=address,undefined -fno-omit-frame-pointer`:

```bash
make test-asan
```

Expected output:
```
=== Starting Test Suite: tests/test_dynstring.c === ... PASSED
=== Starting Test Suite: tests/test_config.c === ... PASSED
=== Starting Test Suite: tests/test_pixop.c === ... PASSED
=== Starting Test Suite: tests/test_raii.cpp === ... PASSED
=== Starting Test Suite: tests/test_buffers.cpp === ... PASSED
=== Starting Test Suite: tests/test_pixmap.cpp === ... PASSED
=== Starting Test Suite: tests/test_modern_config.cpp === ... PASSED
=== Starting Test Suite: tests/test_ansi.cpp === ... PASSED
=== Starting Test Suite: tests/test_event_loop.cpp === ... PASSED
=== Starting Test Suite: tests/test_eink_display.cpp === ... PASSED
=== Starting Test Suite: tests/test_font_renderer.cpp === ... PASSED
=== Starting Test Suite: tests/test_terminal_session.cpp === ... PASSED
=== Starting Test Suite: tests/test_input_manager.cpp === ... PASSED
=== Starting Test Suite: tests/test_application.cpp === ... PASSED
All sanitizer tests passed!
```

---

## Deployment to Kindle Device

### Automated Deployment (`make deploy` / `scripts/update-kindle.sh`)

Deploying and updating the terminal on a physical Kindle device (Linux or WSL) is fully automated and idempotent:

```bash
# Auto-detects Kindle via SSH or mounted USB storage:
make deploy

# Or run the script directly with options:
./scripts/update-kindle.sh --ssh kindle      # SSH alias
./scripts/update-kindle.sh --ssh 192.168.2.2 # Direct USBNetwork IP
./scripts/update-kindle.sh --usb /mnt/e      # USB drive mount (WSL / Linux)
./scripts/update-kindle.sh --reset-config    # Overwrite myts.ini with defaults
```

Key features:
- Safely terminates running sessions prior to transfer to avoid `ETXTBSY` file-busy errors.
- Preserves existing custom user configurations in `/mnt/us/myts/myts.ini`.
- Installs Launchpad bindings (`Shift + T, T`) to `/mnt/us/launchpad/myts.ini` and reloads the daemon.
- Automatically executes a post-deploy `--dry-run` sanity check on the device.
