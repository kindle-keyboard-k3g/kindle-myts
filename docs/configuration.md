# Configuration System

This document outlines the configuration file architecture, parsing algorithms, and supported syntax in `kindle-myts`.

---

## 1. Syntax Overview

The configuration parser accepts standard INI-style files (e.g. `myts.ini`, `keydefs.ini`):

```ini
# Lines beginning with '#' or ';' are comments
[default]
font = ter-u12n.hex
rows = 24
cols = 80
shell = /bin/sh

; Include another file relative to the current directory
include = keydefs.ini

[keydefs]
shift_up = pageup
shift_down = pagedown
```

### 1.1 Parsing Rules
- **Sections**: Enclosed in square brackets `[section_name]`. Case-insensitive.
- **Key-Value Pairs**: Separated by `=`. Leading and trailing whitespace around keys and values is stripped automatically.
- **Comments**: Initiated with `#` (at beginning of line) or `;` (anywhere unless quoted).
- **Includes**: The special directive `include = <filename>` recursively reads and merges secondary configuration files.

---

## 2. Legacy vs. Modern C++ Parsers

### 2.1 Legacy C Parser (`config.c`, `config.h`)
- Implemented as a singly-linked list of `struct entry` nodes attached to `struct section`.
- Allocates memory with `malloc` and `strdup`.
- Accessor: `cfg_find_val(cfg, "section", "key")`.

### 2.2 Modern C++ Parser (`config/config.hpp`)
- Replaces raw pointers with value-typed STL mappings:
  `std::unordered_map<std::string, std::unordered_map<std::string, std::string>>`.
- Case-insensitive key lookup.
- Type-safe accessors:
  - `get_string(section, key, default_val)`
  - `get_int(section, key, default_val)`
  - `get_bool(section, key, default_val)`
- Fully verified in `tests/test_modern_config.cpp` under AddressSanitizer.

---

## 3. Kindle Launchpad Configuration (`myts.l.ini`)

Kindle Launchpad scans INI files in `/mnt/us/launchpad/` to bind hotkeys to terminal launcher commands:

```ini
[Actions]
T T = !/mnt/us/myts/myts &
T Y = !/mnt/us/myts/myts &
T U = !/mnt/us/myts/myts &
T A = !killall -9 myts-ng myts-ng-kindle myts-ng-kindle-dbg myts 2>/dev/null &
```

- When `Shift + T, T` is pressed on the Kindle Keyboard physical keyboard, Launchpad executes `/mnt/us/myts/myts`.
- `/mnt/us/myts/myts` wraps `launch_kindle.sh` to safely stop `framework` and `pmond`, launches `myts-ng`, and restores framework daemons upon terminal termination.

