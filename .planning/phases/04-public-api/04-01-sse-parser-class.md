---
id: "04-01"
phase: "04-public-api"
wave: 1
autonomous: true
objective: "Create SseParser facade class with parse(const char*, size_t) and std::function callback interface (API-01, API-03)"
req_ids: ["API-01", "API-03"]
---

# Plan: SseParser Facade Class

## Objective
Create the main SseParser facade class that integrates Buffer and MessageBuilder to provide a clean public API for parsing SSE streams.

## Requirements Covered
- **API-01**: `parse(const char*, size_t)` interface for raw byte input
- **API-03**: Callback interface using `std::function<void(const Message&)>`

## Read First
<read_first>
- `.planning/phases/04-public-api/04-CONTEXT.md` — Phase context and design decisions
- `include/sse_parser/message_builder.h` — MessageBuilder integration (lines 1-131)
- `include/sse_parser/buffer.h` — Buffer integration (lines 1-146)
- `include/sse_parser/message.h` — Message structure
- `include/sse_parser/error_codes.h` — SseError enum
- `include/sse_parser/sse_parser.h` — Existing main header to update
</read_first>

## Actions

### 1. Create SseParser Header
<action id="1" name="create-sse-parser-header">
**File:** `include/sse_parser/sse_parser_facade.h`

**Must Implement:**
```cpp
namespace sse {

class SseParser {
public:
    using MessageCallback = std::function<void(const Message&)>;
    
    SseParser();
    explicit SseParser(MessageCallback callback);
    
    // API-01: parse raw bytes
    SseError parse(const char* data, size_t len);
    
    // API-03: set callback
    void set_callback(MessageCallback callback);
    
    // Utility accessors
    const std::string& last_event_id() const;
    bool has_incomplete_message() const;
    
private:
    Buffer buffer_;
    MessageBuilder builder_;
    MessageCallback callback_;
    std::string last_event_id_;
    
    void deliver_message(const Message& msg);
};

} // namespace sse
```

**Implementation Details:**
- SseParser::parse() should: buffer_.append() → while (auto line = buffer_.read_line()) → builder_.feed_line()
- SseParser::set_callback() should set both local callback_ and builder_.set_callback()
- deliver_message() should update last_event_id_ from msg.id and call callback_
- Keep header-only design (inline implementations)
</action>

### 2. Update Main Header
<action id="2" name="update-main-header">
**File:** `include/sse_parser/sse_parser.h`

**Must Add:**
```cpp
// Add to existing includes:
#include "sse_parser/sse_parser_facade.h"
```

**Verify:** Main header now includes all components including the new SseParser class
</action>

## Acceptance Criteria
<acceptance_criteria>
**Grep Verification:**

1. SseParser class exists:
```bash
grep -n "class SseParser" include/sse_parser/sse_parser_facade.h
grep -n "using MessageCallback = std::function<void(const Message&)>" include/sse_parser/sse_parser_facade.h
```

2. API-01 parse method exists:
```bash
grep -n "SseError parse(const char\* data, size_t len)" include/sse_parser/sse_parser_facade.h
```

3. API-03 callback interface exists:
```bash
grep -n "void set_callback(MessageCallback callback)" include/sse_parser/sse_parser_facade.h
```

4. Main header includes facade:
```bash
grep -n "sse_parser_facade.h" include/sse_parser/sse_parser.h
```

5. Header-only design (inline keyword):
```bash
grep -n "inline SseError SseParser::parse" include/sse_parser/sse_parser_facade.h
```
</acceptance_criteria>

## Dependencies
- Phase 1: Buffer, Message, Error codes (complete)
- Phase 2: Field parser (complete)
- Phase 3: MessageBuilder (complete)

## Deferred
- parse(string_view) - covered in 04-02
- flush() and reset() - covered in 04-02 and 04-03
- Function pointer callbacks - covered in 04-03 (IFC-01)

## Test Requirements
- Test file created in 04-04
- Basic parse test with callback
- Verify message delivery through callback
</content>
<filePath>.planning/phases/04-public-api/04-01-sse-parser-class.md</filePath>