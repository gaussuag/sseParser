# Phase 1: Foundation - Context

**Gathered:** 2025-03-17
**Status:** Ready for planning

<domain>
## Phase Boundary

Core data structures and error handling for the header-only C++ SSE parser. This phase establishes the foundational types and mechanisms that all subsequent phases will build upon.

**Deliverables:**
- Error code system (`SseError` enum)
- Error message strings
- Message structure definition
- Buffer implementation (internal data storage)
- Unit tests for all components

**Scope Limit:**
- NO parsing logic (Phase 2)
- NO message assembly (Phase 3)
- NO public API facade (Phase 4)
- Focus on types, errors, and buffer only

</domain>

<decisions>
## Implementation Decisions

### Error Handling Strategy

**Error Code Style:**
- Use lightweight custom `enum class SseError` (not std::error_code)
- Rationale: Zero overhead, embedded-friendly, no STL error code complexity
- Values: `success`, `incomplete_message`, `invalid_retry`, `out_of_memory`
- **Note:** `buffer_overflow` removed (see Buffer Strategy below)

**Error Messages:**
- Compile-time constant strings via inline function
- Format: `const char* error_message(SseError)` 
- Header-only, zero runtime overhead
- English only (no localization)

**Parse Result API:**
```cpp
SseError parse(const char* data, size_t len);
// Messages delivered via callback (separate design decision)
```

**Error Recovery:**
- Hard fail mode for non-recoverable errors
- Parser enters error state on failure
- Requires `reset()` to clear state and continue
- Note: With dynamic buffer, most buffer-related failures eliminated

### Buffer Memory Strategy

**Storage Mechanism:**
- Pure dynamic allocation using `std::string` as internal buffer
- Rationale changed from zero-allocation to usability-first
- Never fails under normal conditions (auto-expansion)

**Buffer Behavior:**
- Automatic expansion when full
- No buffer overflow errors under normal operation
- Only `out_of_memory` possible in extreme cases
- Simpler implementation, maximum reliability

**Trade-offs Accepted:**
- ❌ Zero dynamic allocation (abandoned)
- ❌ Zero copy (abandoned)  
- ✅ Maximum usability
- ✅ Never loses data
- ✅ Simple, reliable implementation

### Message Structure Design

**Message Fields:**
```cpp
struct Message {
    std::string event;                    // defaults to "message"
    std::string data;                     // accumulated data lines
    std::optional<std::string> id;        // last-event-id
    std::optional<int> retry;             // reconnection timing
};
```

**Storage Strategy:**
- Use `std::string` for all string fields (not string_view)
- Rationale: Dynamic buffer allows copies, simpler lifetime management
- `std::optional` for nullable fields (C++17)

### Testing Requirements

**Test Deliverables:**
- `tests/test_ring_buffer.cpp` - 15+ test cases, 90% coverage target
- `tests/test_error_codes.cpp` - Error code and message tests
- `tests/test_message.cpp` - Message structure validation

**Test Categories:**
- Unit tests for each component
- Boundary condition tests
- Edge case handling
- Memory behavior verification

### Claude's Discretion

The following areas are left to implementation discretion:

- Exact string buffer implementation details (std::string methods used)
- Error state machine implementation (how to track/parser state)
- Test framework setup details (fixture organization, helper functions)
- Exact error message text (as long as clear and accurate)
- Internal buffer growth strategy (std::string handles this)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project Context
- `.planning/PROJECT.md` — Core value, constraints, target platforms (note: zero-allocation constraint modified in this phase)
- `.planning/REQUIREMENTS.md` — Requirement IDs mapped to phases (ERR-01/02, MSG-01, BUF-01, ROB-01, PER-01/02)
- `.planning/ROADMAP.md` — Phase 1 success criteria and test requirements
- `.planning/research/STACK.md` — Technical stack recommendations (adjust for dynamic allocation decision)
- `.planning/TESTING.md` — Testing strategy and coverage targets

### Key Requirement Changes from Original Spec
- **BUF-01** implementation changed from fixed ring buffer to dynamic std::string
- **ROB-01** buffer overflow handling no longer applicable (dynamic expansion)
- **PER-01** zero allocation requirement abandoned in favor of usability

### Important Notes
- Original PROJECT.md specified "zero dynamic allocations" — this has been overridden by Phase 1 decision
- Original design used ring buffer with fixed size — now using std::string with auto-expansion
- These changes improve usability at cost of strict embedded constraints

</canonical_refs>

<code_context>
## Existing Code Insights

### Project Structure
```
sseParser/
├── CMakeLists.txt          # C++17, gtest enabled
├── src/
│   └── main.cpp           # Example code (not parser)
├── tests/
│   ├── CMakeLists.txt     # gtest configuration
│   └── main_test.cpp      # Existing sample tests
└── libs/
    └── googletest/        # gtest submodule
```

### Build System
- CMake 3.16+
- C++17 standard
- Google Test for testing
- Header-only library target (no src compilation needed)

### Reusable Patterns
- Existing CMake setup supports header-only libraries
- Google Test infrastructure ready for new test files
- Project root `include/` directory (to be created) for public headers

### Integration Points
- Parser will live in `include/sse_parser/` (suggested)
- Tests in `tests/` following existing pattern
- Public API header: `include/sse_parser/sse_parser.h` (suggested)

</code_context>

<specifics>
## Specific Requirements

### Error Code Enum (exact)
```cpp
enum class SseError {
    success = 0,
    incomplete_message,
    invalid_retry,
    out_of_memory
};
```

### Message Structure (exact)
```cpp
struct Message {
    std::string event = "message";
    std::string data;
    std::optional<std::string> id;
    std::optional<int> retry;
};
```

### Buffer Implementation Guidelines
- Use `std::string` as internal storage
- Append incoming data: `buffer_.append(data, len)`
- Process complete messages, keep incomplete tail
- Let std::string handle memory automatically

### Testing Must Cover
- All error code values
- Error message string conversion
- Message structure initialization
- Buffer append and processing
- Edge cases (empty input, large input)

</specifics>

<deferred>
## Deferred Ideas

**Ideas that came up but belong in later phases:**

- **Message parsing logic** — Phase 2 (field extraction, line ending handling)
- **Multi-line data accumulation** — Phase 3 (message assembly)
- **Public API facade** — Phase 4 (SseParser class, callbacks)
- **Callback design** — Phase 4 (std::function vs function pointer)
- **Zero-allocation mode** — Potential future enhancement (template flag for embedded)
- **Ring buffer alternative** — Potential future enhancement (for strict embedded constraints)

**Noted for future consideration:**
- Could add template parameter to choose static vs dynamic buffer
- Could provide two parser variants: `SseParser` (dynamic) and `StaticSseParser<N>` (fixed)
- Out of scope for Phase 1-5, consider for v2.0

</deferred>

---

*Phase: 01-foundation*
*Context gathered: 2025-03-17*
*Key decisions: Dynamic buffer (std::string), usability over zero-allocation, custom error enum*
