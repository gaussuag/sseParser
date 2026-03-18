# Phase 4: Public API - Context

**Gathered:** 2025-03-18
**Status:** Ready for planning
**Depends on:** Phase 1, 2, 3 (all complete)

<domain>
## Phase Boundary

**Goal:** Facade class and callback integration

This phase creates the public-facing API that combines all previous phases into a cohesive, easy-to-use interface.

**Deliverables:**
- SseParser facade class
- parse(const char*, size_t) interface (API-01)
- parse(string_view) interface (API-02)
- Callback interface (API-03)
- flush() method (API-04)
- reset() method (API-05)
- Function pointer callback support (IFC-01, P1)
- Comprehensive test suite

**Scope Limit:**
- NO advanced features (ADV-01/02/03, v2)
- NO thread safety (out of scope per REQUIREMENTS.md)
- Focus on clean, simple public API

**Dependencies from Previous Phases:**
- Phase 1: Buffer, Message, Error codes
- Phase 2: Field parser, line ending handling
- Phase 3: MessageBuilder with multi-line and ID tracking

</domain>

<decisions>
## Implementation Decisions

### SseParser Class Design

**Primary Interface:**
```cpp
class SseParser {
public:
    using MessageCallback = std::function<void(const Message&)>;
    
    SseParser();
    explicit SseParser(MessageCallback callback);
    
    // API-01: parse with raw bytes
    SseError parse(const char* data, size_t len);
    
    // API-02: parse with string_view (zero-copy input)
    SseError parse(std::string_view data);
    
    // API-03: set callback
    void set_callback(MessageCallback callback);
    
    // API-04: flush incomplete message
    SseError flush();
    
    // API-05: reset state
    void reset();
    
    // Utility
    const std::string& last_event_id() const;
    bool has_incomplete_message() const;
};
```

**Design Principles:**
- Header-only (consistent with rest of library)
- RAII-compliant
- Callback-based (message delivery)
- Non-blocking (parse returns immediately)

### Callback Strategy (API-03)

**Primary:** std::function<void(const Message&)>
- Flexible, accepts lambdas, functors, function pointers
- Type-safe
- Slight overhead (acceptable for most use cases)

**Alternative (IFC-01, P1):** Function pointer
```cpp
using MessageCallbackFn = void(*)(const Message*, void* user_data);
```
- Zero overhead abstraction
- Embedded-friendly
- Can be set alongside std::function (user chooses)

### Zero-Copy Input (API-02)

**string_view Interface:**
```cpp
SseError parse(std::string_view data) {
    return parse(data.data(), data.size());
}
```
- True zero-copy (data not modified)
- Internal buffer copies only when necessary
- C++17 feature

### Chunked Input Handling

**Scenario:** Network delivers data in arbitrary chunks
```
Chunk 1: "data: hel"
Chunk 2: "lo\n\n"
```

**Implementation:**
- Buffer accumulates partial data (Phase 1)
- parse() appends to buffer
- Messages delivered when complete
- No special handling needed (Buffer handles it)

### flush() Behavior (API-04)

**Purpose:** Handle incomplete message at stream end

**Cases:**
1. No incomplete message → success, no callback
2. Incomplete message without trailing newline → deliver as-is
3. Empty buffer → success

**Example:**
```
Input: "data: hello" (no newline)
flush() → Message with data="hello"
```

### reset() Behavior (API-05)

**Purpose:** Clear all state (reconnection scenario)

**Actions:**
1. Clear internal buffer
2. Clear MessageBuilder state
3. Reset last_event_id
4. Clear any pending message
5. Keep callback (don't unregister)

**Use Case:** HTTP reconnection, want fresh parser state but keep same callback

### Claude's Discretion

The following areas are left to implementation discretion:

- Move semantics (should Message be movable?)
- Copy semantics for SseParser
- Thread-safety documentation (explicitly not thread-safe)
- Error handling for invalid callbacks
- Buffer size limits (use Phase 1 Buffer limits)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase 1-3 Dependencies
- `.planning/phases/01-foundation/` — Buffer, Message, Error codes
- `.planning/phases/02-core-parsing/` — Field parser
- `.planning/phases/03-message-assembly/` — MessageBuilder
- `include/sse_parser/buffer.h`
- `include/sse_parser/message.h`
- `include/sse_parser/error_codes.h`
- `include/sse_parser/field_parser.h`
- `include/sse_parser/message_builder.h`

### Project Context
- `.planning/REQUIREMENTS.md` — Phase 4 requirements (API-01/02/03/04/05, IFC-01)
- `.planning/ROADMAP.md` — Phase 4 success criteria
- `.planning/TESTING.md` — Testing strategy

</canonical_refs>

<code_context>
## Existing Code to Build Upon

### Phase 3 Deliverables

**MessageBuilder (include/sse_parser/message_builder.h):**
```cpp
class MessageBuilder {
public:
    using MessageCallback = std::function<void(const Message&)>;
    
    void set_callback(MessageCallback callback);
    SseError feed_line(const std::string& line);
    std::optional<Message> get_message();
    const std::string& last_event_id() const;
    void reset();
};
```

### Phase 1 Deliverables

**Buffer (include/sse_parser/buffer.h):**
```cpp
class Buffer {
public:
    SseError append(const char* data, size_t len);
    std::optional<std::string_view> read_line();
    bool has_complete_message() const;
    void clear();
    // ...
};
```

**Integration Points:**
- SseParser owns Buffer and MessageBuilder
- SseParser::parse() → Buffer::append() → Buffer::read_line() → MessageBuilder::feed_line()
- SseParser::flush() → MessageBuilder::flush_current_message()
- SseParser::reset() → Buffer::clear() + MessageBuilder::reset()

</code_context>

<specifics>
## Specific Requirements

### API-01: parse(const char*, size_t)

```cpp
SseError parse(const char* data, size_t len);
```

- Accepts raw byte buffer
- Appends to internal buffer
- Processes complete lines
- Delivers complete messages via callback
- Returns error code

### API-02: parse(string_view)

```cpp
SseError parse(std::string_view data);
```

- C++17 zero-copy interface
- Delegates to parse(const char*, size_t)
- No data modification

### API-03: Callback Interface

```cpp
using MessageCallback = std::function<void(const Message&)>;

void set_callback(MessageCallback callback);
```

- std::function for flexibility
- Called synchronously when message complete
- Can be changed at runtime

### API-04: flush()

```cpp
SseError flush();
```

- Forces delivery of incomplete message
- Called at stream end
- Returns success or error

### API-05: reset()

```cpp
void reset();
```

- Clears all state
- Keeps callback registered
- For reconnection scenarios

### IFC-01: Function Pointer Support (P1)

```cpp
using MessageCallbackFn = void(*)(const Message*, void* user_data);

void set_callback(MessageCallbackFn callback, void* user_data);
```

- Zero overhead
- Embedded-friendly
- Optional P1 feature

### Success Criteria

From ROADMAP.md:
1. ✓ Handle chunked input correctly
2. ✓ Callback triggered on complete messages
3. ✓ flush() handles incomplete final message
4. ✓ reset() clears state correctly
5. ✓ string_view interface is zero-copy

### Test Deliverables

- `tests/test_sse_parser.cpp` — 12+ test cases
- Chunked input simulation
- Callback type tests (std::function + function pointer)
- Target: 85% line coverage

</specifics>

<deferred>
## Deferred to Later Versions

**v2 / Advanced Features (ADV):**
- ADV-01: Streaming data output (no accumulation)
- ADV-02: Configurable max message size
- ADV-03: Statistics (message count, bytes)
- ADV-04: Thread-safe wrapper

**Not in scope:**
- HTTP client (out of scope per REQUIREMENTS.md)
- Auto-reconnect logic (network layer)
- WebSocket support (different protocol)

</deferred>

---

*Phase: 04-public-api*
*Context gathered: 2025-03-18*
*Depends on: Phases 1-3 complete*
*Key focus: Public facade, callbacks, parse methods, flush/reset*
