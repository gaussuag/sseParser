# SSE Parser

## Current State

**v1.0 SHIPPED** — 2026-03-19

A production-ready C++17 header-only library for parsing Server-Sent Events (SSE) protocol. Successfully handles real-world LLM streaming scenarios with 295 tests passing.

### What Shipped

- ✅ **Core parsing** — Full SSE protocol support (event, data, id, retry fields)
- ✅ **Network resilience** — Handles arbitrary fragmentation across chunks
- ✅ **Clean API** — parse(), callbacks, flush(), reset()
- ✅ **Zero-copy** — string_view input, minimal allocations
- ✅ **Comprehensive tests** — 295 tests, fuzzing, performance benchmarks
- ✅ **Documentation** — 111 Doxygen comments, 8 usage examples
- ✅ **Multi-compiler** — GCC, Clang, MSVC CI validation

**Stats:** 35K+ LOC, 122 commits, 5 phases, 24 plans, 3 days

## Core Value

Parse any valid SSE stream correctly, regardless of how network chunks arrive, and deliver complete, well-formed messages to the caller via clean callbacks.

## Requirements

### Validated (v1.0)

- ✅ Parse SSE protocol messages from arbitrary byte chunks — v1.0
- ✅ Handle message fragmentation across multiple input calls — v1.0
- ✅ Support all standard SSE fields: event, data, id, retry — v1.0
- ✅ Preserve multi-line data formatting as received — v1.0
- ✅ Provide clean callback interface (on_message) — v1.0
- ✅ Support both const char*+size and string_view inputs — v1.0
- ✅ Use error codes for all error handling (no exceptions) — v1.0
- ✅ Header-only design for easy integration — v1.0

### Active (Next Milestone)

- [ ] Thread-safe wrapper for concurrent access
- [ ] Streaming data output (no full accumulation)
- [ ] Configurable max message size limits
- [ ] Parse statistics (message count, byte count)
- [ ] Custom field handlers

### Out of Scope

- WebSocket support — strictly SSE only
- Network I/O implementation — caller provides data
- JSON parsing of data field — raw string only
- Compression/decompression — caller handles encoding
- HTTP protocol handling — assume caller stripped headers
- Server-side SSE generation — client-side only
- Automatic reconnection logic — network layer responsibility

## Context

- **Shipped:** v1.0 with 35K+ LOC, 295 tests (100% pass rate)
- **Target platforms:** Desktop (Windows/Linux/macOS), embedded systems
- **Tech stack:** C++17, header-only, STL only
- **Performance:** 8-11 MB/s debug, >100 MB/s expected release
- **Integration:** 8 usage examples, comprehensive documentation
- **Known limitation:** Uses std::string internally (accepted deviation from zero-allocation for usability)

## Constraints

- **Language:** C++17 minimum — no C++20 features
- **Threading:** Single-threaded parse context — caller serializes access
- **Dependencies:** Header-only, no external libraries (STL only)
- **Portability:** GCC 9+, Clang 10+, MSVC 2019+

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Header-only | Easy integration, no build complexity | ✅ Validated — works well |
| Callback-based (on_message) | Clean separation, user handles dispatch | ✅ Validated — flexible |
| Internal buffering | Handles network fragmentation | ✅ Validated — seamless |
| Error codes vs exceptions | Embedded-friendly, deterministic | ✅ Validated — clean |
| string_view input | Zero-copy, modern C++ | ✅ Validated — efficient |
| std::string for Buffer | Usability over strict zero-allocation | ⚠️ Accepted deviation |

---
*Last updated: 2026-03-19 after v1.0 milestone completion*
