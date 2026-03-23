# SSE Parser

## Current State

**v2.0 SHIPPED** — 2026-03-23

A production-ready C++17 header-only library for parsing Server-Sent Events (SSE) protocol. Single-header distribution (`sse_parser.hpp`) with 298 tests passing.

### What Shipped (v2.0)

- ✅ **Single header** — `#include "sse_parser/sse_parser.hpp"` only
- ✅ **Core parsing** — Full SSE protocol support (event, data, id, retry fields)
- ✅ **Network resilience** — Handles arbitrary fragmentation across chunks
- ✅ **Clean API** — parse(), callbacks, flush(), reset()
- ✅ **Zero-copy** — string_view input, minimal allocations
- ✅ **Comprehensive tests** — 298 tests, fuzzing, performance benchmarks
- ✅ **Documentation** — README, CHANGELOG, Doxygen, migration guide
- ✅ **Multi-compiler** — GCC, Clang, MSVC CI validation

**Stats:** 35K+ LOC, 150+ commits, 2 milestones

## Core Value

Parse any valid SSE stream correctly, regardless of how network chunks arrive, and deliver complete, well-formed messages to the caller via clean callbacks.

## Requirements

### Validated (v1.0 + v2.0)

- ✅ Parse SSE protocol messages from arbitrary byte chunks
- ✅ Handle message fragmentation across multiple input calls
- ✅ Support all standard SSE fields: event, data, id, retry
- ✅ Preserve multi-line data formatting as received
- ✅ Provide clean callback interface (on_message)
- ✅ Support both const char*+size and string_view inputs
- ✅ Use error codes for all error handling (no exceptions)
- ✅ Header-only design for easy integration
- ✅ Single header distribution (v2.0)
- ✅ Compilation optimized with consolidated includes (v2.0)

### Active

_(none yet — ready for next milestone)_

### Out of Scope

| Feature | Reason |
|---------|--------|
| C++20 features | C++17 minimum maintained |
| Multi-header option | Single header is the goal |
| Runtime performance changes | No algorithmic changes, only structural |

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Header-only | Easy integration, no build complexity | ✅ Validated |
| Callback-based (on_message) | Clean separation, user handles dispatch | ✅ Validated |
| Internal buffering | Handles network fragmentation | ✅ Validated |
| Error codes vs exceptions | Embedded-friendly, deterministic | ✅ Validated |
| string_view input | Zero-copy, modern C++ | ✅ Validated |
| std::string for Buffer | Usability over strict zero-allocation | ⚠️ Accepted deviation |
| v2.0 Single Header | Simplified user include experience | ✅ Validated |

## Context

- **Tech stack:** C++17, header-only, STL only
- **Target platforms:** Desktop (Windows/Linux/macOS), embedded systems
- **Performance:** 8-11 MB/s debug, >100 MB/s expected release
- **Integration:** 8 usage examples, comprehensive documentation
- **Breaking Change:** v2.0 is API-breaking release (old .h headers removed)

## Constraints

- **Language:** C++17 minimum — no C++20 features
- **Threading:** Single-threaded parse context — caller serializes access
- **Dependencies:** Header-only, no external libraries (STL only)
- **Portability:** GCC 9+, Clang 10+, MSVC 2019+

---

_Milestones: [v1.0](./.planning/milestones/v1.0-ROADMAP.md) | [v2.0](./.planning/milestones/v2.0-ROADMAP.md)_

---
*Last updated: 2026-03-23 after v2.0 milestone*
