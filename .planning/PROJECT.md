# SSE Parser

## Current State

**v1.0 SHIPPED** — 2026-03-19
**Phase 6 (Header Consolidation) Complete** — 2026-03-20
**Phase 7 (API Export Verification) Complete** — 2026-03-20
**Phase 8 (Compilation Optimization) Complete** — 2026-03-20
**Phase 9 (Backward Compatibility) Complete** — 2026-03-20
**Phase 10 (Documentation Update) Complete** — 2026-03-20

A production-ready C++17 header-only library for parsing Server-Sent Events (SSE) protocol. Successfully handles real-world LLM streaming scenarios with 298 tests passing.

### What Shipped (v2.0)

- ✅ **Single header** — `#include "sse_parser/sse_parser.hpp"` only
- ✅ **Core parsing** — Full SSE protocol support (event, data, id, retry fields)
- ✅ **Network resilience** — Handles arbitrary fragmentation across chunks
- ✅ **Clean API** — parse(), callbacks, flush(), reset()
- ✅ **Zero-copy** — string_view input, minimal allocations
- ✅ **Comprehensive tests** — 298 tests, fuzzing, performance benchmarks
- ✅ **Documentation** — Doxygen comments, README, CHANGELOG, migration guide
- ✅ **Multi-compiler** — GCC, Clang, MSVC CI validation

**Stats:** 35K+ LOC, 140+ commits, 10 phases, 5-day v2.0 milestone

## Core Value

Parse any valid SSE stream correctly, regardless of how network chunks arrive, and deliver complete, well-formed messages to the caller via clean callbacks.

## Current Milestone: v2.0 Single Header Integration ✅ COMPLETE

**Goal:** Merge all header files into a single include for simplified user integration — ACHIEVED

**Target features:**
- Single `sse_parser.h` header with complete functionality
- All existing public APIs preserved and re-exported
- Optimized include structure for faster compilation
- Backward compatibility maintained via include guards

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
- **Breaking Change:** v2.0 is API-breaking release

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Header-only | Easy integration, no build complexity | ✅ Validated — works well |
| Callback-based (on_message) | Clean separation, user handles dispatch | ✅ Validated — flexible |
| Internal buffering | Handles network fragmentation | ✅ Validated — seamless |
| Error codes vs exceptions | Embedded-friendly, deterministic | ✅ Validated — clean |
| string_view input | Zero-copy, modern C++ | ✅ Validated — efficient |
| std::string for Buffer | Usability over strict zero-allocation | ⚠️ Accepted deviation |
| v2.0 Single Header | Simplified user include experience | ✅ Validated — Phase 10 complete, README/CHANGELOG updated, v2.0 ready to ship |

---
*Last updated: 2026-03-20 after Phase 10 completion — v2.0 milestone COMPLETE*
