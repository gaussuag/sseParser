---
id: 01-04
phase: 1
wave: 1
autonomous: true
objective: Implement Buffer class using std::string for internal storage
req_ids: [BUF-01]
gap_closure: false
---

# Plan 01-04: Buffer Implementation

**Objective:** Implement the Buffer class using std::string for automatic memory management.

**Estimated Time:** 45 minutes

## Tasks

### Task 1: Define Buffer Class
Implement in `include/sse_parser/buffer.h`:
```cpp
class Buffer {
public:
    Buffer() = default;
    
    SseError append(const char* data, size_t len);
    SseError append(std::string_view data);
    
    std::optional<std::string_view> read_line();
    bool has_complete_message() const;
    
    size_t size() const noexcept { return buffer_.size(); }
    bool empty() const noexcept { return buffer_.empty(); }
    void clear() noexcept { buffer_.clear(); }
    std::string_view view() const { return buffer_; }

private:
    std::string buffer_;
    size_t read_pos_ = 0;
    
    size_t find_line_end(size_t start) const;
};
```

### Task 2: Implement append()
- Append data to internal std::string
- Return success (std::string handles memory)
- Handle both const char* and string_view overloads

### Task 3: Implement read_line()
- Read from current read_pos_ to line ending
- Handle \n, \r\n, and \r
- Return string_view into buffer (valid until next modification)
- Update read_pos_
- Return nullopt if no complete line

### Task 4: Implement has_complete_message()
- Check for SSE message boundary: \n\n, \r\n\r\n, or \r\r
- Return true if found in buffer

### Task 5: Implement find_line_end()
- Helper to find line ending position
- Handle all three newline styles

## Files Modified

- `include/sse_parser/buffer.h`

## Success Criteria

- [ ] Buffer class with all methods
- [ ] append() adds data to buffer
- [ ] read_line() returns complete lines
- [ ] Handles LF, CRLF, and CR
- [ ] has_complete_message() detects boundaries
- [ ] No memory leaks (RAII via std::string)
- [ ] Compiles without warnings

## Dependencies

- 01-01: Project Structure Setup
- 01-02: Error Code System

## Testing Notes

This will be tested in 01-08-test-buffer.md
