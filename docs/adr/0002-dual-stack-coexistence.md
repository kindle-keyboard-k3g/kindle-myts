# ADR-0002: Coexistence of Legacy C Engine and Modern C++ Stack

## Status
Accepted

## Context
The legacy C implementation (`myts.c`, `terminal.c`, `screen.c`) has been used on physical Kindles since 2010. While transitioning to modern C++, maintaining backward compatibility and preserving the legacy build artifact prevents breakage on older Kindle firmware installations.

## Decision
Retain both binary targets in the `Makefile`:
1. `make myts`: Builds the legacy C binary.
2. `make myts-ng`: Builds the modern C++ binary.
3. `make myts.zip`: Bundles both `myts` and `myts-ng` alongside fonts and configuration files.

## Consequences
- Zero regression risk for existing users.
- Allows side-by-side benchmarking on real hardware.
