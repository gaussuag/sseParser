---
id: "04-02"
phase: "04-public-api"
wave: 2
autonomous: true
objective: "Implement string_view parse interface and flush() method for incomplete messages (API-02, API-04)"
req_ids: ["API-02", "API-04"]
---

# Plan: String View and Flush

## Objective
Add zero-copy string_view parse interface and flush() method to handle incomplete messages at stream end.

## Requirements Covered
- **API-02**: `parse(std::string_view)` - C++17 zero-copy interface
- **API-04**: `flush()` - Force delivery of incomplete final message

## Read First
<read_first>
- `.planning/phases/04-public-api/04-01-sse-parser-class.md` — Previous plan with SseParser base
- `include/sse_parser/sse_parser_facade.h` — SseParser class to extend
- `include/sse_parser/message_builder.h` — Check for flush_current_message() or similar (lines 50-66)
- `.planning/phases/04-public-api/04-CONTEXT.md` — flush() behavior specification (lines 118-131)
</read_first>

## Actions

### 1. Add String View Parse
<action id="1" name="add-string-view-parse">
**File:** `include/sse_parser/sse_parser_facade.h`

**Must Add to SseParser class:**
```cpp
// API-02: Zero-copy string_view interface
SseError parse(std::string_view data) {
    return parse(data.data(), data.size());
}
```

**Design Notes:**
- Delegates to parse(const char*, size_t) for single implementation
- True zero-copy - no data modification
- C++17 feature, consistent with rest of library
</action>

### 2. Add Flush Method
<action id="2" name="add-flush-method">
**File:** `include/sse_parser/sse_parser_facade.h`

**Must Add to SseParser class:**
```cpp
// API-04: Flush incomplete message at stream end
SseError flush();
```

**Implementation Requirements:**
```cpp
inline SseError SseParser::flush() {
    // Case 1: No incomplete message → success, no callback
    if (buffer_.empty()) {
        return SseError::success;
    }
    
    // Case 2: Get remaining data from buffer
    auto remaining = buffer_.view();
    if (remaining.empty()) {
        return SseError::success;
    }
    
    // Case 3: Process any remaining complete lines first
    while (auto line = buffer_.read_line()) {
        SseError err = builder_.feed_line(*line);
        if (err != SseError::success) {
            return err;
        }
    }
    
    // Case 4: Handle incomplete final line (no trailing newline)
    auto leftover = buffer_.view();
    if (!leftover.empty()) {
        // Feed as final line without newline
        SseError err = builder_.feed_line(leftover);
        if (err != SseError::success) {
            return err;
        }
        // Force message completion
        builder_.feed_line("");  // Empty line triggers completion
    }
    
    buffer_.clear();
    return SseError::success;
}
```

**Edge Cases:**
- Empty buffer → returns success
- Data ending with newline → already processed, no action
- Data without newline → deliver as message
- Multiple calls → second call should be no-op
</action>

### 3. Update MessageBuilder if Needed
<action id="3" name="verify-message-builder">
**File:** `include/sse_parser/message_builder.h`

**Check:** Does MessageBuilder have a way to force completion without blank line?

**If Not:** Add `force_complete()` method or verify `feed_line("")` triggers completion
</action>

## Acceptance Criteria
<acceptance_criteria>
**Grep Verification:**

1. API-02 string_view interface exists:
```bash
grep -n "SseError parse(std::string_view data)" include/sse_parser/sse_parser_facade.h
grep -n "data.data(), data.size()" include/sse_parser/sse_parser_facade.h
```

2. API-04 flush method exists:
```bash
grep -n "SseError flush()" include/sse_parser/sse_parser_facade.h
```

3. Flush implementation handles incomplete messages:
```bash
grep -n "buffer_.empty()" include/sse_parser/sse_parser_facade.h
grep -n "builder_.feed_line" include/sse_parser/sse_parser_facade.h | head -5
```

4. Both methods are inline (header-only):
```bash
grep -n "inline SseError.*parse.*string_view" include/sse_parser/sse_parser_facade.h
grep -n "inline SseError.*flush" include/sse_parser/sse_parser_facade.h
```
</acceptance_criteria>

## Dependencies
- 04-01: SseParser base class must exist
- Phase 1: Buffer.clear() method available

## Test Requirements
- parse(string_view) delegates correctly to parse(const char*, size_t)
- flush() delivers incomplete message ("data: hello" without \n)
- flush() with no data returns success and no callback
- Double flush() is safe (second is no-op)
