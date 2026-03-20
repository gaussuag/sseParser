# Requirements: SSE Parser v2.0

**Defined:** 2026-03-20
**Core Value:** Parse any valid SSE stream correctly regardless of network fragmentation

## v2.0 Requirements

Requirements for single header file integration. This is an API-breaking release (v2.0).

### Header Integration

- [ ] **HDRI-01**: Single header file `sse_parser.h` contains complete SSE parser implementation
- [ ] **HDRI-02**: All component headers (buffer.h, message.h, field_parser.h, etc.) are merged into single header
- [ ] **HDRI-03**: Main header includes version macros (SSE_PARSER_VERSION_MAJOR, MINOR, PATCH)
- [ ] **HDRI-04**: Public API symbols properly exported with `SSE_PARSER_API` macro (if needed for DLL export)

### Backward Compatibility

- [ ] **BCMP-01**: Legacy include path `sse_parser/buffer.h` still works via forwarding headers
- [ ] **BCMP-02**: Legacy include path `sse_parser/message.h` still works via forwarding headers
- [ ] **BCMP-03**: Legacy include path `sse_parser/field_parser.h` still works via forwarding headers
- [ ] **BCMP-04**: Legacy include path `sse_parser/sse_parser_facade.h` still works via forwarding headers
- [ ] **BCMP-05**: Deprecation warnings added to legacy include paths

### Compilation Optimization

- [ ] **COMP-01**: Include guards prevent double inclusion penalty
- [ ] **COMP-02**: Header dependencies minimized through forward declarations
- [ ] **COMP-03**: No circular dependencies between components
- [ ] **COMP-04**: Compilation time measurable improvement vs v1.0

### Documentation

- [ ] **DOCS-01**: Updated README with v2.0 include instructions
- [ ] **DOCS-02**: Doxygen configuration updated for single header
- [ ] **DOCS-03**: Migration guide from v1.x to v2.0 provided
- [ ] **DOCS-04**: All usage examples updated to single include

## v1.0 Validated (Preserved)

The following v1.0 requirements remain validated and must be preserved in v2.0:

- Parse SSE protocol messages from arbitrary byte chunks
- Handle message fragmentation across multiple input calls
- Support all standard SSE fields: event, data, id, retry
- Preserve multi-line data formatting as received
- Provide clean callback interface (on_message)
- Support both const char*+size and string_view inputs
- Use error codes for all error handling (no exceptions)
- Header-only design for easy integration

## Out of Scope

| Feature | Reason |
|---------|--------|
| C++20 features | C++17 minimum maintained |
| Runtime performance changes | No algorithmic changes, only structural |
| New functionality | Focus is consolidation, not feature addition |
| Multi-header option | Single header is the goal |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| HDRI-01 | Phase 1 | Pending |
| HDRI-02 | Phase 1 | Pending |
| HDRI-03 | Phase 1 | Pending |
| HDRI-04 | Phase 2 | Pending |
| BCMP-01 | Phase 4 | Pending |
| BCMP-02 | Phase 4 | Pending |
| BCMP-03 | Phase 4 | Pending |
| BCMP-04 | Phase 4 | Pending |
| BCMP-05 | Phase 4 | Pending |
| COMP-01 | Phase 3 | Pending |
| COMP-02 | Phase 3 | Pending |
| COMP-03 | Phase 3 | Pending |
| COMP-04 | Phase 5 | Pending |
| DOCS-01 | Phase 5 | Pending |
| DOCS-02 | Phase 5 | Pending |
| DOCS-03 | Phase 5 | Pending |
| DOCS-04 | Phase 5 | Pending |

**Coverage:**
- v2.0 requirements: 17 total
- Mapped to phases: 17
- Unmapped: 0 ✓

---
*Requirements defined: 2026-03-20*
*Last updated: 2026-03-20 after v2.0 milestone started*
