---
id: 01-03
phase: 1
wave: 1
autonomous: true
objective: Define Message struct with standard SSE fields
req_ids: [MSG-01]
gap_closure: false
---

# Plan 01-03: Message Structure

**Objective:** Define the Message struct with standard SSE fields and helper methods.

**Estimated Time:** 20 minutes

## Tasks

### Task 1: Define Message Struct
Implement in `include/sse_parser/message.h`:
```cpp
struct Message {
    std::string event = "message";           // defaults to "message"
    std::string data;                         // accumulated data lines
    std::optional<std::string> id;           // last-event-id
    std::optional<int> retry;                // reconnection timing
};
```

### Task 2: Add clear() Method
```cpp
void clear() noexcept;
```
Resets all fields:
- event = "message"
- data.clear()
- id.reset()
- retry.reset()

### Task 3: Add empty() Method
```cpp
bool empty() const noexcept;
```
Returns true if data field is empty

### Task 4: Verify Compilation
- Ensure no warnings
- Check that std::optional is properly used

## Files Modified

- `include/sse_parser/message.h`

## Success Criteria

- [ ] Message struct defined with all 4 fields
- [ ] Default event value = "message"
- [ ] clear() method resets all fields
- [ ] empty() returns correct value
- [ ] Compiles without warnings

## Dependencies

- 01-01: Project Structure Setup

## Testing Notes

This will be tested in 01-07-test-message.md
