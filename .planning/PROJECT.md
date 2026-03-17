# SSE Parser

## What This Is

A C++17 header-only library for parsing Server-Sent Events (SSE) protocol data streams. Focused solely on protocol parsing with network layer decoupled via interfaces. Designed for desktop clients and embedded devices handling LLM streaming responses.

## Core Value

Parse any valid SSE stream correctly, regardless of how network chunks arrive, and deliver complete, well-formed messages to the caller via clean callbacks.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] Parse SSE protocol messages from arbitrary byte chunks
- [ ] Handle message fragmentation across multiple input calls
- [ ] Support all standard SSE fields: event, data, id, retry
- [ ] Preserve multi-line data formatting as received
- [ ] Provide clean callback interface (on_message)
- [ ] Support both const char*+size and string_view inputs
- [ ] Use error codes for all error handling (no exceptions)
- [ ] Ensure internal thread safety (no concurrent parse calls)
- [ ] Maintain zero dynamic allocations in hot path (configurable)
- [ ] Header-only design for easy integration

### Out of Scope

- WebSocket support — strictly SSE only
- Network I/O implementation — caller provides data
- JSON parsing of data field — raw string only
- Compression/decompression — caller handles encoding
- HTTP protocol handling — assume caller stripped headers
- Server-side SSE generation — client-side only

## Context

- **Target platforms**: Desktop (Windows/Linux/macOS), embedded systems
- **Data volume**: Optimized for LLM streaming (high frequency, moderate size)
- **Memory constraints**: Must work on resource-limited embedded devices
- **Integration pattern**: User implements network layer, feeds raw bytes to parser
- **Error philosophy**: Fail fast with error codes, never throw

## Constraints

- **Language**: C++17 minimum — no C++20 features
- **Threading**: Single-threaded parse context — caller serializes access
- **Dependencies**: Header-only, no external libraries (STL only)
- **Portability**: Must compile on GCC 7+, Clang 6+, MSVC 2017+
- **Performance**: Zero-copy where possible, minimal allocations

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Header-only | Easy integration, no build system complexity | — Pending |
| Callback-based (on_message) | Clean separation, user handles dispatch | — Pending |
| Internal buffering | Handles network fragmentation transparently | — Pending |
| Error codes vs exceptions | Embedded-friendly, deterministic | — Pending |
| string_view input | Zero-copy, modern C++ | — Pending |

---
*Last updated: 2025-03-17 after initialization*
