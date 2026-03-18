# Phase 2: Core Parsing - Context

**Gathered:** 2025-03-18
**Status:** Ready for planning
**Depends on:** Phase 1 Foundation (complete)

<domain>
## Phase Boundary

**Goal:** Parse SSE fields and handle line endings

This phase implements the core SSE protocol parsing logic on top of Phase 1's foundation (Buffer, Message, Error codes).

**Deliverables:**
- Field parser (`field: value` format)
- Line ending handler (LF/CRLF/CR)
- Leading space trimmer
- Comment skipper
- Retry field validator
- UTF-8 BOM detector (P2)
- Comprehensive test suite

**Scope Limit:**
- NO message assembly (Phase 3)
- NO public API facade (Phase 4)
- NO callbacks (Phase 4)
- Focus on parsing raw bytes to structured fields

**Dependencies from Phase 1:**
- `Buffer` class - for accumulating partial data
- `Message` struct - for storing parsed fields
- `SseError` enum - for error reporting
- `error_message()` - for error descriptions

</domain>

<decisions>
## Implementation Decisions

### Field Parser Design

**Format to Parse:**
```
field: value
field:value
field: value with spaces
:comment line (ignored)
```

**Parsing Strategy:**
- Read line by line from Buffer (using `read_line()`)
- Split on first `:` to separate field name from value
- Field names: `event`, `data`, `id`, `retry` (case-sensitive per spec)
- Unknown fields: silently ignore (not an error per SSE spec)
- Lines without `:`: treat as field with empty value

**Leading Space Handling (PAR-03):**
```cpp
// Per SSE spec: remove exactly one space after colon if present
"data: hello"   → field="data", value="hello"
"data:  hello"  → field="data", value=" hello" (keep second space)
"data:hello"    → field="data", value="hello"
```

**State Machine Approach:**
- Simple line-by-line parser (no complex state needed in this phase)
- Each line is independent until message boundary (blank line)
- Output: populated Message struct fields

### Line Ending Handler (PAR-02)

**Supported Styles:**
- `\n` (LF) — Unix, modern systems
- `\r\n` (CRLF) — Windows, HTTP standard
- `\r` (CR) — Classic Mac, legacy

**Test Matrix (9 combinations):**
```
Line1 + Line2 = Message boundary
--------------------------------
LF + LF       (Unix style)
LF + CRLF     (mixed)
LF + CR       (mixed)
CRLF + LF     (mixed)
CRLF + CRLF   (HTTP style)
CRLF + CR     (mixed)
CR + LF       (mixed)
CR + CRLF     (mixed)
CR + CR       (classic Mac)
```

**Implementation Note:** Buffer already handles all three line endings in `read_line()`. Phase 2 just needs to detect double line endings (message boundaries).

### Comment Handling (PAR-04)

**Comment Lines:**
- Start with `:` (colon as first character)
- Entire line is ignored
- Do NOT produce empty messages (per spec, empty messages require `\n\n`)

**Examples:**
```
: This is a comment
data: hello\n\n  → Message with data="hello"
```

### Retry Validation (VAL-01)

**Retry Field Format:**
- Must be valid integer
- Range: positive integer (typically milliseconds)
- Overflow detection: return `invalid_retry` error
- Non-numeric: return `invalid_retry` error

**Implementation:**
```cpp
SseError parse_retry(const std::string& value, int& result);
// Returns success or invalid_retry
```

### UTF-8 BOM Detection (EXT-01, P2)

**BOM Sequence:** `\xEF\xBB\xBF`
- Only at stream start
- Skip if present, don't treat as data
- Optional: warn/log detection

**Implementation:**
- Check first 3 bytes of stream
- Remove BOM before processing
- Low priority (P2), can be deferred if needed

### Claude's Discretion

The following areas are left to implementation discretion:

- Exact parsing function signatures (static functions or class methods)
- Whether to create a Parser class or free functions
- How to handle partial field values (wait for complete line)
- Test organization (group by feature or test file per component)
- Error message details for retry validation failures

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase 1 Foundation (Dependencies)
- `.planning/phases/01-foundation/` — All Phase 1 deliverables
- `include/sse_parser/buffer.h` — Buffer class interface
- `include/sse_parser/message.h` — Message struct
- `include/sse_parser/error_codes.h` — SseError enum

### Project Context
- `.planning/REQUIREMENTS.md` — Phase 2 requirements (PAR-01/02/03/04, VAL-01, EXT-01)
- `.planning/ROADMAP.md` — Phase 2 success criteria and test requirements
- `.planning/TESTING.md` — Testing strategy

### Technical References
- [SSE Spec](https://html.spec.whatwg.org/multipage/server-sent-events.html) — Official specification
- Phase 1 decisions on Buffer/Message implementation

</canonical_refs>

<code_context>
## Existing Code to Build Upon

### Phase 1 Deliverables

**Buffer (include/sse_parser/buffer.h):**
```cpp
class Buffer {
public:
    SseError append(const char* data, size_t len);
    std::optional<std::string_view> read_line();
    bool has_complete_message() const;
    // ... other methods
};
```

**Message (include/sse_parser/message.h):**
```cpp
struct Message {
    std::string event = "message";
    std::string data;
    std::optional<std::string> id;
    std::optional<int> retry;
    void clear() noexcept;
    bool empty() const noexcept;
};
```

**Error Codes (include/sse_parser/error_codes.h):**
```cpp
enum class SseError : uint8_t {
    success = 0,
    incomplete_message,
    invalid_retry,
    out_of_memory,
    buffer_overflow
};
```

### Integration Points
- Parser reads from Buffer using `read_line()`
- Parser populates Message struct fields
- Parser returns SseError for validation failures
- Parser works line-by-line (streaming)

</code_context>

<specifics>
## Specific Requirements

### Field Parsing (PAR-01)

**Valid Fields:**
- `event: <value>` → Message.event
- `data: <value>` → Message.data (Phase 3: multi-line accumulation)
- `id: <value>` → Message.id
- `retry: <value>` → Message.retry (validated)
- `<unknown>: <value>` → Silently ignore

**Edge Cases:**
```
"event"        → field="event", value=""
"event:"       → field="event", value=""
"event: "      → field="event", value=""
"event: test"  → field="event", value="test"
":comment"     → skip (comment)
""             → empty line = message boundary
```

### Line Ending Matrix (PAR-02)

Must test all 9 combinations of line1-ending + line2-ending.

### Leading Space Rule (PAR-03)

Only remove the first space after colon, if present.

### Success Criteria

From ROADMAP.md:
1. ✓ Correctly parse single-field messages
2. ✓ Pass line ending test matrix (9 combinations)
3. ✓ Handle leading spaces correctly (spec examples)
4. ✓ Skip comment lines
5. ✓ Return error for invalid retry values

### Test Deliverables

- `tests/test_field_parser.cpp` — 12+ test cases
- `tests/test_line_endings.cpp` — 9 combination matrix
- Target: 85% line coverage

</specifics>

<deferred>
## Deferred to Later Phases

**Phase 3 (Message Assembly):**
- Multi-line data accumulation (`data:` lines merge with `\n`)
- Empty message detection and handling
- Message completion triggering

**Phase 4 (Public API):**
- Callback integration
- `parse()` method facade
- `flush()` and `reset()` methods

**Not in scope:**
- Streaming data output (ADV-01, v2)
- Custom field handlers (PRO-01, v2)

</deferred>

---

*Phase: 02-core-parsing*
*Context gathered: 2025-03-18*
*Depends on: Phase 1 Foundation (complete)*
*Key focus: SSE protocol parsing, line endings, field validation*
