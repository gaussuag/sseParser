# Research Summary: C++ SSE Parser

**Date:** 2025-03-17  
**Domain:** Server-Sent Events (SSE) protocol parsing  
**Target:** C++17 header-only library

---

## Key Findings

### Stack Recommendation

**Core approach:**
- **C++17** with `std::string_view` for zero-copy inputs
- **Fixed-size ring buffer** (configurable, default 4KB) for internal storage
- **State machine parser** (no regex/stream overhead)
- **Error codes** instead of exceptions (embedded-friendly)
- **Zero dynamic allocation** in hot path

**Compiler support:** GCC 7+, Clang 6+, MSVC 2017+

### Table Stakes Features

Must implement for a usable parser:
1. Field parsing (`field: value` format)
2. Standard fields: event, data, id, retry
3. Message delimiting (handle `\n\n`, `\r\n\r\n`, `\r\r`)
4. Data line accumulation (join with `\n`)
5. Comment handling (lines starting with `:`)
6. Input buffering (handle network fragmentation)
7. Callback-based message delivery

### Critical Pitfalls

**Top 3 risks to address:**

1. **Line ending handling** — Spec allows three styles, can be mixed
   - *Prevention:* Normalize early, test all combinations
   - *Phase:* Phase 2 (Core Parsing)

2. **Buffer overrun** — Network faster than consumption
   - *Prevention:* RingBuffer rejects writes when full, return error
   - *Phase:* Phase 1 (Foundation)

3. **Data accumulation** — Multi-line data must join with `\n`
   - *Prevention:* Explicit test cases, follow spec exactly
   - *Phase:* Phase 3 (Message Assembly)

### Architecture Overview

```
SseParser (Facade)
├── RingBuffer<N> (internal storage)
├── StateMachine (parsing logic)
├── MessageBuilder (accumulation)
└── Callback (user handler)
```

**Build order:**
1. Foundation (Error codes, Message, RingBuffer)
2. Core Parsing (State machine, field extraction)
3. Message Assembly (Multi-line data, dispatch)
4. Public API (Facade, callback integration)
5. Polish (Edge cases, docs)

### Design Decisions

| Decision | Rationale |
|----------|-----------|
| Header-only | Easy integration, no build complexity |
| Template buffer size | Flexibility for embedded vs desktop |
| string_view input | Zero-copy, C++17 best practice |
| Error codes | Embedded-friendly, deterministic |
| User-provided callback | Flexibility for threading model |
| Single-threaded | User handles synchronization |

### Anti-Features (Out of Scope)

- HTTP client implementation
- JSON parsing
- Compression handling
- Thread synchronization
- WebSocket support
- Automatic reconnection

### Performance Targets

- Throughput: >100MB/s on modern hardware
- Memory: <8KB per parser instance (with 4KB buffer)
- Allocations: Zero in hot path
- Cache-friendly sequential access

---

## Recommended Requirements

Based on research, v1 should include:

**P0 (Critical):**
- Parse standard SSE format
- Handle all line ending styles
- Multi-line data accumulation
- Callback message delivery
- Input buffering for fragmentation
- Error code handling
- Zero dynamic allocation

**P1 (High):**
- Zero-copy input (string_view)
- Configurable buffer size
- Field validation (retry numeric)

**P2 (Medium):**
- BOM handling
- Error recovery
- Last-Event-ID tracking

**Out of Scope:**
- Network layer
- JSON parsing
- Compression
- Thread safety

---

## Next Steps

1. Define formal requirements (REQUIREMENTS.md)
2. Create implementation roadmap
3. Begin Phase 1: Foundation types and buffer
4. Prioritize line-ending handling in Phase 2

---

*Synthesized from: STACK.md, FEATURES.md, ARCHITECTURE.md, PITFALLS.md*
