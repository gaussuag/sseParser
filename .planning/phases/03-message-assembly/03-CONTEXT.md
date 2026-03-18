# Phase 3: Message Assembly - Context

**Gathered:** 2025-03-18
**Status:** Ready for planning
**Depends on:** Phase 1 Foundation, Phase 2 Core Parsing (both complete)

<domain>
## Phase Boundary

**Goal:** Build complete messages and handle multi-line data

This phase assembles parsed fields into complete SSE messages, handling multi-line data accumulation and empty messages.

**Deliverables:**
- Message builder (assembles fields into complete messages)
- Multi-line data accumulator (DAT-01)
- Empty message handler (DAT-02)
- Last-Event-ID tracker (EXT-02, P2)
- Comprehensive test suite

**Scope Limit:**
- NO public API facade (Phase 4)
- NO callbacks (Phase 4)
- NO streaming output (ADV-01, v2)
- Focus on message assembly from parsed fields

**Dependencies from Phase 2:**
- `field_parser` - for parsing individual fields
- Line ending detection - for message boundaries

**Dependencies from Phase 1:**
- `Buffer` class - for data accumulation
- `Message` struct - for storing assembled messages
- `SseError` enum - for error reporting

</domain>

<decisions>
## Implementation Decisions

### Message Builder Design

**Core Responsibility:**
Accumulate parsed fields until a complete message is detected (blank line), then deliver the assembled message.

**Message Detection:**
- Message boundary = blank line (\n\n, \r\n\r\n, or \r\r)
- Parse fields line by line until boundary
- When boundary reached, message is complete

**Multi-line Data Handling (DAT-01):**
```
data: line1\n
data: line2\n
data: line3\n
→ Message.data = "line1\nline2\nline3"
```

Rules:
- Multiple `data:` lines accumulate with `\n` separator
- No trailing `\n` after last line
- Empty `data:` line adds empty string (just `\n`)

**Empty Message Handling (DAT-02):**
```
\n\n

→ Message with empty data (triggers callback)
```

- Blank line with no preceding fields = empty message
- Must still trigger message callback
- Message.empty() returns true, but it's a valid message

### State Machine Design

**Parser State:**
```cpp
enum class ParseState {
    waiting_for_field,    // Ready to parse next field
    accumulating_data,    // Building multi-line data
    message_complete      // Ready to deliver message
};
```

**State Transitions:**
- waiting_for_field → accumulating_data: Got data field
- accumulating_data → accumulating_data: Got another data field
- accumulating_data → message_complete: Got blank line
- waiting_for_field → message_complete: Got blank line (empty message)

### Last-Event-ID Tracking (EXT-02, P2)

**Requirement:** Track the last received event ID automatically

**Implementation:**
- Store last seen `id` field value
- Provide accessor: `const std::string& last_event_id() const`
- Update on every message with an ID field
- Used for reconnection scenarios (client sends Last-Event-ID header)

**Example:**
```
id: 1\n
data: first\n\n
id: 2\n
data: second\n\n
→ last_event_id() returns "2"
```

### Message Delivery Strategy

**Approach:** Callback-based (preparing for Phase 4)

```cpp
using MessageCallback = std::function<void(const Message&)>;

class MessageBuilder {
public:
    void set_callback(MessageCallback callback);
    SseError feed_line(const std::string& line);
    // ...
};
```

**Alternative (for testing without callbacks):**
- Store completed messages in a queue
- Provide `std::optional<Message> get_completed_message()`
- Simpler for unit testing

### Claude's Discretion

The following areas are left to implementation discretion:

- Exact MessageBuilder class interface
- Whether to use callbacks or message queue
- State machine implementation details
- How to handle partial messages at stream end
- Test organization and naming conventions

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase 1 & 2 Dependencies
- `.planning/phases/01-foundation/` — Buffer, Message, Error codes
- `.planning/phases/02-core-parsing/` — field_parser, line ending handling
- `include/sse_parser/buffer.h` — Buffer class interface
- `include/sse_parser/message.h` — Message struct
- `include/sse_parser/error_codes.h` — SseError enum
- `include/sse_parser/field_parser.h` — Field parsing functions

### Project Context
- `.planning/REQUIREMENTS.md` — Phase 3 requirements (DAT-01, DAT-02, EXT-02)
- `.planning/ROADMAP.md` — Phase 3 success criteria and test requirements
- `.planning/TESTING.md` — Testing strategy

### Technical References
- [SSE Spec](https://html.spec.whatwg.org/multipage/server-sent-events.html) — Message format specification

</canonical_refs>

<code_context>
## Existing Code to Build Upon

### Phase 2 Deliverables

**Field Parser (include/sse_parser/field_parser.h):**
```cpp
// Parses individual fields from lines
struct Field {
    std::string name;
    std::string value;
};

std::optional<Field> parse_field(const std::string& line);
SseError validate_retry(const std::string& value, int& result);
bool has_bom(const char* data, size_t len);
void skip_bom(const char*& data, size_t& len);
```

**Line Ending Detection:**
- Buffer::read_line() already handles all line ending styles
- Returns std::optional<std::string_view>

### Phase 1 Deliverables

**Buffer (include/sse_parser/buffer.h):**
```cpp
class Buffer {
public:
    SseError append(const char* data, size_t len);
    std::optional<std::string_view> read_line();
    bool has_complete_message() const;
    // ...
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

### Integration Points
- MessageBuilder uses field_parser to parse individual fields
- MessageBuilder accumulates data into Message struct
- MessageBuilder detects message boundaries (blank lines)
- MessageBuilder tracks last_event_id across messages

</code_context>

<specifics>
## Specific Requirements

### Multi-line Data Accumulation (DAT-01)

**Test Cases:**
```
Input:                    Expected Message.data:
─────────────────────────────────────────────────
data: hello\n             "hello"
data: world\n\n

data: line1\n              "line1\nline2\nline3"
data: line2\n
data: line3\n\n

data: \n                   "\n" (single newline)
data: \n\n

data: hello\n              "hello" (no trailing newline)
\n\n                      (empty line ends message)
```

### Empty Message Handling (DAT-02)

**Test Cases:**
```
Input:                    Result:
─────────────────────────────────────────────────
\n\n                      Empty message (data="")

event: test\n\n           Message with event="test", data=""

data: \n\n                Message with data=""
```

### Last-Event-ID Tracking (EXT-02)

**Test Cases:**
```
Input:                    Last-Event-ID:
─────────────────────────────────────────────────
id: 1\n                   "1"
data: first\n\n
id: 2\n                   "2"
data: second\n\n

data: no id\n\n            "2" (unchanged)
```

### Success Criteria

From ROADMAP.md:
1. ✓ Multi-line data connected with `\n` correctly
2. ✓ Empty messages trigger callbacks (data is empty string)
3. ✓ Message boundaries detected correctly
4. ✓ ID field tracked correctly

### Test Deliverables

- `tests/test_message_builder.cpp` — 8+ test cases
- Target: 90% line coverage

</specifics>

<deferred>
## Deferred to Later Phases

**Phase 4 (Public API):**
- Callback integration with std::function
- `parse()` method facade
- `flush()` and `reset()` methods

**Not in scope:**
- Streaming data output (ADV-01, v2)
- Custom field handlers (PRO-01, v2)

</deferred>

---

*Phase: 03-message-assembly*
*Context gathered: 2025-03-18*
*Depends on: Phase 1 & 2 complete*
*Key focus: Message assembly, multi-line data, empty messages, ID tracking*
