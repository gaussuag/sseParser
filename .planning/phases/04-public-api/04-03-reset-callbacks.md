---
id: "04-03"
phase: "04-public-api"
wave: 3
autonomous: true
objective: "Implement reset() method and function pointer callback support for embedded scenarios (API-05, IFC-01)"
req_ids: ["API-05", "IFC-01"]
---

# Plan: Reset and Callback Variants

## Objective
Add reset() method for reconnection scenarios and function pointer callback support for embedded/low-overhead use cases.

## Requirements Covered
- **API-05**: `reset()` - Clear all state while keeping callback
- **IFC-01**: Function pointer callback support - Zero overhead for embedded systems

## Read First
<read_first>
- `.planning/phases/04-public-api/04-02-string-view-flush.md` — Previous plan with parse and flush
- `include/sse_parser/sse_parser_facade.h` — SseParser to extend
- `.planning/phases/04-public-api/04-CONTEXT.md` — reset() behavior spec (lines 133-145)
- `include/sse_parser/message_builder.h` — reset() implementation to mirror (lines 51-56)
</read_first>

## Actions

### 1. Add Reset Method
<action id="1" name="add-reset-method">
**File:** `include/sse_parser/sse_parser_facade.h`

**Must Add to SseParser class:**
```cpp
// API-05: Reset all state for reconnection scenarios
void reset() noexcept;
```

**Implementation:**
```cpp
inline void SseParser::reset() noexcept {
    buffer_.clear();           // Clear internal buffer
    builder_.reset();          // Clear MessageBuilder state
    last_event_id_.clear();    // Reset last_event_id tracking
    // Note: callback_ is NOT cleared - keep registered callback
}
```

**Design Notes:**
- noexcept - no allocations, cannot fail
- Callback remains registered for reconnection use case
- Clears: buffer, MessageBuilder, last_event_id
- Keeps: callback registration
</action>

### 2. Add Function Pointer Support
<action id="2" name="add-function-pointer-callback">
**File:** `include/sse_parser/sse_parser_facade.h`

**Must Add to SseParser class:**
```cpp
// IFC-01: Function pointer callback for zero overhead
using MessageCallbackFn = void(*)(const Message* msg, void* user_data);

void set_callback(MessageCallbackFn callback, void* user_data);
```

**Add to private members:**
```cpp
private:
    // Existing:
    Buffer buffer_;
    MessageBuilder builder_;
    MessageCallback callback_;
    std::string last_event_id_;
    
    // New for IFC-01:
    MessageCallbackFn fn_callback_ = nullptr;
    void* user_data_ = nullptr;
```

**Implementation:**
```cpp
inline void SseParser::set_callback(MessageCallbackFn callback, void* user_data) {
    fn_callback_ = callback;
    user_data_ = user_data;
    
    // Wrap function pointer in std::function for MessageBuilder compatibility
    if (callback) {
        builder_.set_callback([this, callback](const Message& msg) {
            callback(&msg, user_data_);
        });
    }
}
```

**Design Notes:**
- Function pointer + user_data pattern (C-compatible)
- Zero overhead - no std::function overhead in hot path when using fn_callback_
- Can coexist with std::function callback (last set wins)
- For embedded systems without RTTI/exceptions
</action>

### 3. Update Deliver Message Logic
<action id="3" name="update-deliver-message">
**File:** `include/sse_parser/sse_parser_facade.h`

**Update deliver_message() to support both callback types:**
```cpp
inline void SseParser::deliver_message(const Message& msg) {
    // Update last_event_id_ (EXT-02)
    if (msg.id.has_value()) {
        last_event_id_ = *msg.id;
    }
    
    // Try function pointer first (zero overhead path)
    if (fn_callback_) {
        fn_callback_(&msg, user_data_);
        return;
    }
    
    // Fall back to std::function
    if (callback_) {
        callback_(msg);
    }
}
```

**Alternative:** Use only MessageBuilder callback and track delivery there
</action>

## Acceptance Criteria
<acceptance_criteria>
**Grep Verification:**

1. API-05 reset() exists:
```bash
grep -n "void reset() noexcept" include/sse_parser/sse_parser_facade.h
```

2. Reset clears required state:
```bash
grep -n "buffer_.clear()" include/sse_parser/sse_parser_facade.h
grep -n "builder_.reset()" include/sse_parser/sse_parser_facade.h
grep -n "last_event_id_.clear()" include/sse_parser/sse_parser_facade.h
```

3. Reset does NOT clear callback:
```bash
# Verify NO "callback_ = " in reset implementation
grep -A 10 "void SseParser::reset" include/sse_parser/sse_parser_facade.h | grep -v "callback_ ="
```

4. IFC-01 function pointer type exists:
```bash
grep -n "using MessageCallbackFn = void\(\*\)" include/sse_parser/sse_parser_facade.h
grep -n "void set_callback(MessageCallbackFn callback, void\* user_data)" include/sse_parser/sse_parser_facade.h
```

5. Function pointer members exist:
```bash
grep -n "MessageCallbackFn fn_callback_" include/sse_parser/sse_parser_facade.h
grep -n "void\* user_data_" include/sse_parser/sse_parser_facade.h
```

6. Both interfaces can be set:
```bash
grep -c "set_callback" include/sse_parser/sse_parser_facade.h
# Should return 2 (std::function and function pointer variants)
```
</acceptance_criteria>

## Dependencies
- 04-01: SseParser base class
- 04-02: Optional - flush() may interact with reset behavior

## Test Requirements
- reset() clears buffer and state but keeps callback
- Function pointer callback receives messages correctly
- User data passed correctly to function pointer
- Mixed usage: std::function and function pointer can both work
