# Research: SSE Parser Pitfalls

## Critical Mistakes

### 1. Line Ending Handling

**The Problem:**
SSE spec allows three line ending styles:
- `\n\n` (Unix)
- `\r\n\r\n` (Windows)
- `\r\r` (Old Mac)

And they can be mixed in the same stream!

**Warning Signs:**
- Tests pass on Linux but fail on Windows
- Messages not recognized correctly
- Buffer corruption or infinite loops

**Prevention:**
- Normalize line endings early in state machine
- Test with all three styles
- Test with mixed endings: `data: hello\r\ndata: world\n\n`

**Phase to Address:** Phase 2 (Core Parsing)

---

### 2. Data Line Accumulation

**The Problem:**
Multiple `data:` lines must be joined with `\n`:
```
data: hello
data: world

// Should produce: "hello\nworld"
```

Common errors:
- Using `\r\n` instead of `\n` as separator
- Forgetting separator (concatenation)
- Adding extra separator at end

**Warning Signs:**
- Multi-line messages have wrong format
- JSON parsing fails on accumulated data

**Prevention:**
- Explicit test cases for multi-line data
- Verify spec example exactly

**Phase to Address:** Phase 3 (Message Assembly)

---

### 3. Buffer Overrun

**The Problem:**
Network chunks can arrive faster than messages are consumed.
Without proper handling, buffer overflows and data is lost.

**Warning Signs:**
- Intermittent parse errors under load
- Lost messages during high throughput
- Memory corruption crashes

**Prevention:**
- RingBuffer must reject writes when full
- Return error code to caller
- Document maximum message size limit

```cpp
if (buffer_.full()) {
    return Error::buffer_overflow;
}
```

**Phase to Address:** Phase 1 (Foundation)

---

### 4. Incomplete Final Message

**The Problem:**
Stream ends (connection closed) with partial message in buffer.
User needs to decide: deliver partial or discard?

```
data: hello
// Connection closes here - no \n\n
```

**Warning Signs:**
- Last message lost on disconnect
- Memory leaks (buffer never cleared)

**Prevention:**
- Provide explicit `flush()` method
- Document behavior clearly
- Test connection close scenarios

**Phase to Address:** Phase 4 (Public API)

---

### 5. Field Name Validation

**The Problem:**
SSE allows any field name, but only these have standard meaning:
- `event`
- `data`
- `id`
- `retry`

Unknown fields should be ignored (per spec), not error.

**Warning Signs:**
- Parser rejects valid SSE streams
- Interoperability issues with non-standard servers

**Prevention:**
- Silently ignore unknown fields
- Only validate retry is numeric
- Don't be strict about field names

**Phase to Address:** Phase 2 (Core Parsing)

---

### 6. Leading Space in Values

**The Problem:**
Spec says: "If value starts with space, remove exactly one space."

```
data: hello   // value is "hello"
data:  hello  // value is " hello" (one space kept)
```

**Warning Signs:**
- Leading spaces missing in parsed data
- Content verification fails

**Prevention:**
- Implement exactly per spec
- Add test case for this specific rule

**Phase to Address:** Phase 2 (Core Parsing)

---

### 7. Empty Messages

**The Problem:**
`\n\n` alone is a valid (empty) message per spec.
Should it trigger callback with empty data?

**Warning Signs:**
- Empty callbacks causing issues
- Infinite loops on empty messages

**Prevention:**
- Decide behavior explicitly
- Document clearly
- Test edge case

**Phase to Address:** Phase 3 (Message Assembly)

---

### 8. UTF-8 and BOM

**The Problem:**
Some servers send UTF-8 BOM (`\xEF\xBB\xBF`) at stream start.
This should be stripped, not treated as data.

**Warning Signs:**
- First character of first message is garbage
- JSON parsing fails on BOM-prefixed data

**Prevention:**
- Detect and strip BOM at stream start
- State machine tracks "first bytes seen"

**Phase to Address:** Phase 2 (Core Parsing) - optional P2

---

### 9. Integer Overflow in Retry

**The Problem:**
`retry:` field is milliseconds as integer.
Very large values can overflow int.

```
retry: 999999999999999999999999999999
```

**Warning Signs:**
- Negative retry values
- Undefined behavior on parse

**Prevention:**
- Use checked conversion (strtol with validation)
- Clamp to reasonable range
- Return error on overflow

**Phase to Address:** Phase 2 (Core Parsing)

---

### 10. Callback Exception Safety

**The Problem:**
User callback might throw. Parser must remain in valid state.

**Warning Signs:**
- State corruption after callback exception
- Double callbacks
- Memory leaks

**Prevention:**
- Document: callback must not throw
- Or catch and convert to error code
- Clear builder state before callback

**Phase to Address:** Phase 4 (Public API)

---

## Domain-Specific Gotchas

### LLM Streaming Specifics

1. **Very long data lines**: LLM tokens can accumulate into large messages
   - Ensure buffer is sized adequately
   - Consider streaming data in chunks (but SSE doesn't support this)

2. **High frequency**: LLMs can emit many small messages
   - Minimize allocations in hot path
   - Profile for cache efficiency

3. **JSON in data**: Most LLM APIs send JSON
   - Don't parse JSON in library (out of scope)
   - Ensure data field preserves JSON exactly

### Embedded Systems

1. **Stack size**: Large buffer on stack may overflow
   - Allow static allocation or placement new
   - Document stack requirements

2. **No exceptions**: Ensure truly exception-free
   - Compiler flags: `-fno-exceptions`
   - No STL functions that throw

3. **No RTTI**: Don't use dynamic_cast, typeid
   - Pure header-only shouldn't need this anyway

## Testing Strategy

**Essential Test Cases:**
1. All three line ending styles
2. Mixed line endings
3. Multi-line data accumulation
4. Buffer boundary conditions
5. Maximum message size
6. Empty messages
7. Unknown fields
8. Leading space handling
9. Connection close mid-message
10. UTF-8 content
11. BOM handling
12. Very long lines
13. Invalid retry values

**Fuzzing Targets:**
- Random byte sequences
- Truncated messages
- Malformed fields
