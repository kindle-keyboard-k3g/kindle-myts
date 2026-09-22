# ADR-0001: Adoption of Modern C++17 Embedded Profile with Zero Allocations

## Status
Accepted

## Context
The original `kindle-myts` code was written in 2010 in procedural C. While fast, it lacked RAII encapsulation, making file descriptor leaks, memory ownership bugs, and pointer mismanagement easy to introduce. The target device is a Kindle Keyboard (Freescale i.MX353 ARMv6 @ 532 MHz, 128 MB RAM).

## Decision
Adopt a modern C++17 embedded profile:
1. Enforce `-fno-exceptions -fno-rtti` to eliminate C++ runtime overhead and exception tables.
2. Encapsulate file descriptors and memory mappings with RAII (`core::UniqueFd`, `core::MemoryMapping`).
3. Ban dynamic heap allocations inside hot inner loops (key translation, rendering, event dispatching).
4. Use C++20 for host unit tests (`tests/`) while retaining C++17 compatibility for target compilation.

## Consequences
- Standalone stripped binary size is only ~23 KB (`myts-ng`).
- Eliminates resource leaks by construction.
- Enables strong type safety without performance penalties.
