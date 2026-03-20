# Phase 6: Header Consolidation - Context

**Gathered:** 2026-03-20
**Status:** Ready for planning

<domain>
## Phase Boundary

Merge all 7 header files into a single `sse_parser.hpp` file for simplified user integration. This is an API-breaking release (v2.0).

**Scope:**
- Combine error_codes.h, message.h, buffer.h, field_parser.h, message_builder.h, sse_parser_facade.h into one file
- Maintain all existing public API
- Version bump to 2.0.0
- Update all tests and examples to use new single include

</domain>

<decisions>
## Implementation Decisions

### File Format
- **Single file**: `sse_parser.hpp` (C++ header-only convention)
- **All implementations inline**: No separate .cpp files
- **Use `#pragma region`**: For IDE code folding to simulate multi-file structure

### Version
- **Version number**: 2.0.0 (API-breaking release)

### Code Organization
- **Merge order**: error_codes → message → buffer → field_parser → message_builder → sse_parser_facade
- **Namespace**: Keep `sse` namespace (already decided in v1.0)
- **Namespace alias**: Keep `sse_parser = sse` for backward compatibility

### Legacy Headers
- **Delete old headers**: Remove the 6 separate header files (buffer.h, message.h, etc.)
- **No forwarding headers**: No need for deprecation forwarding in v2.0

### No Development/Release Separation
- **Direct merge**: No automation for merge-on-release workflow
- **Rationale**: 1000 lines is manageable, simpler is better
- **Token cost**: ~35-40K tokens for full file, acceptable for modern AI context windows

</decisions>

<specifics>
## Specific References

- **Reference library**: BS_thread_pool.hpp - single header-only pattern
- **Reference library**: nlohmann/json - large single header (for scale reference)

</specifics>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project Requirements
- `.planning/REQUIREMENTS.md` — HDRI-01 through HDRI-04 requirements
- `.planning/ROADMAP.md` — Phase 6 description and success criteria

### Existing Code Structure
- `include/sse_parser/sse_parser.h` — Current main header (to be replaced)
- `include/sse_parser/error_codes.h` — Component to merge
- `include/sse_parser/message.h` — Component to merge
- `include/sse_parser/buffer.h` — Component to merge (241 lines, has inline implementations)
- `include/sse_parser/field_parser.h` — Component to merge (196 lines, has inline implementations)
- `include/sse_parser/message_builder.h` — Component to merge (224 lines, has inline implementations)
- `include/sse_parser/sse_parser_facade.h` — Component to merge (279 lines, has inline implementations)

### Testing
- `tests/test_sse_parser.cpp` — Main API tests (612 lines)
- `examples/usage_examples.cpp` — Usage examples to update (269 lines)

</canonical_refs>

<code_context>
## Existing Code Insights

### Header Structure
- **Current entry point**: `sse_parser.h` includes all other 6 headers
- **Inline implementations**: buffer.h, field_parser.h, message_builder.h, sse_parser_facade.h all have inline function implementations
- **No separate .cpp**: Header-only library, all implementation in headers

### Dependencies
```
sse_parser.h
├── error_codes.h (no dependencies)
├── message.h (no dependencies)
├── buffer.h (depends on error_codes.h)
├── field_parser.h (depends on error_codes.h, message.h)
├── message_builder.h (depends on error_codes.h, message.h, field_parser.h)
└── sse_parser_facade.h (depends on buffer.h, error_codes.h, message.h, message_builder.h)
```

### Key Implementation Details
- **Namespace**: `sse` namespace throughout
- **Error handling**: `enum class SseError` with `inline const char* error_message()`
- **Inline functions**: Many implementations are inline in headers (required for header-only)

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 06-header-consolidation*
*Context gathered: 2026-03-20*
