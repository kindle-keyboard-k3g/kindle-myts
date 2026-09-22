# ADR-0003: Hardware E-Ink Driver Mock Fallback for Host Testing

## Status
Accepted

## Context
Physical Kindle devices use proprietary kernel ioctl interfaces (`FBIO_EINK_UPDATE_DISPLAY_AREA` in `linux/einkfb.h`) and mapped `/dev/fb0` memory. Running unit tests and sanitizer checks on standard Linux/macOS host developer machines failed because `/dev/fb0` was missing or lacked E-Ink ioctl support.

## Decision
Design `display::HardwareEinkDriver` to implement `graphics::IEinkDriver` with an automatic host fallback:
1. Attempt to open `/dev/fb0` and query screen info.
2. If the device does not exist or cannot be opened, allocate an in-memory `mock_buffer_` (600×800 4bpp) and mark `is_hardware() == false`.
3. Emulate display refresh calls successfully in memory.

## Consequences
- 100% test coverage and AddressSanitizer verification on standard CI/workstations.
- Zero conditional compilation `#ifdef KINDLE` clutter in rendering algorithms.
