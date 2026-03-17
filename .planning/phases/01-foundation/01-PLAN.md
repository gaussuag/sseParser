# Phase 1: Foundation - Execution Plan

**Phase:** 01-foundation  
**Goal:** Core data structures and error handling  
**Created:** 2025-03-17  
**Status:** Ready for execution

---

## Overview

This phase establishes the foundational types and error handling mechanisms for the SSE parser. All subsequent phases build upon these core components.

**Key Decisions from Context:**
- Pure dynamic allocation using `std::string` (usability over zero-allocation)
- Custom `SseError` enum (not std::error_code)
- `std::string` fields in Message struct (not string_view)
- Target: 90% test coverage

---

## Task Breakdown

### Task 1: Project Structure Setup

**ID:** T1  
**Priority:** P0 (Blocking)  
**Estimated Time:** 15 minutes

**Description:**
Create directory structure and initial files for the header-only library.

**Files to Create:**
```
include/
└── sse_parser/
    ├── sse_parser.h          # Main public header
    ├── error_codes.h         # SseError enum and error_message()
    ├── message.h             # Message struct definition
    └── buffer.h              # Internal buffer implementation
```

**Acceptance Criteria:**
- [ ] Directory structure created
- [ ] Empty header files created with include guards
- [ ] CMakeLists.txt updated to include new headers

**Dependencies:** None

---

### Task 2: Error Code System

**ID:** T2  
**Priority:** P0 (Blocking)  
**Estimated Time:** 30 minutes

**Description:**
Implement SseError enum and error message functions.

**Requirements:**
```cpp
// include/sse_parser/error_codes.h
#pragma once
#include <cstdint>

namespace sse {

enum class SseError : uint8_t {
    success = 0,
    incomplete_message,
    invalid_retry,
    out_of_memory
};

const char* error_message(SseError error) noexcept;

// Optional: operator bool for easy checking
inline bool operator!(SseError e) noexcept {
    return e == SseError::success;
}

} // namespace sse
```

**Implementation Details:**
- Use `uint8_t` underlying type for compactness
- `error_message()` returns English-only strings
- Inline function for zero overhead
- Include guards: `#pragma once`

**Acceptance Criteria:**
- [ ] Enum defined with all 4 values
- [ ] error_message() implemented with switch statement
- [ ] Helper operators for bool conversion
- [ ] Compiles without warnings (C++17)

**Test Requirements:**
- All enum values tested
- error_message() returns correct strings
- Bool conversion works correctly

**Dependencies:** T1

---

### Task 3: Message Structure

**ID:** T3  
**Priority:** P0 (Blocking)  
**Estimated Time:** 20 minutes

**Description:**
Define Message struct with standard SSE fields.

**Requirements:**
```cpp
// include/sse_parser/message.h
#pragma once
#include <string>
#include <optional>

namespace sse {

struct Message {
    std::string event = "message";           // Event type (default: "message")
    std::string data;                         // Message data (accumulated)
    std::optional<std::string> id;           // Last-Event-ID
    std::optional<int> retry;                // Reconnection timing (milliseconds)
    
    // Clear all fields (for reuse)
    void clear() noexcept;
    
    // Check if message has any content
    bool empty() const noexcept;
};

} // namespace sse
```

**Implementation Details:**
- Use `std::string` for string fields (not string_view)
- `std::optional` for nullable fields (C++17)
- `clear()` method for buffer reuse optimization
- `empty()` checks if data field is empty

**Acceptance Criteria:**
- [ ] Struct defined with all fields
- [ ] Default event value = "message"
- [ ] clear() method implemented
- [ ] empty() method implemented
- [ ] Compiles without warnings

**Test Requirements:**
- Default initialization correct
- clear() resets all fields
- empty() returns correct value

**Dependencies:** T1

---

### Task 4: Buffer Implementation

**ID:** T4  
**Priority:** P0 (Blocking)  
**Estimated Time:** 45 minutes

**Description:**
Implement internal buffer using std::string for data accumulation.

**Requirements:**
```cpp
// include/sse_parser/buffer.h
#pragma once
#include <string>
#include <string_view>
#include <optional>
#include "error_codes.h"

namespace sse {

class Buffer {
public:
    // Default constructor
    Buffer() = default;
    
    // Append data to buffer
    SseError append(const char* data, size_t len);
    SseError append(std::string_view data);
    
    // Read a complete line (up to and including \n)
    // Returns empty optional if no complete line available
    std::optional<std::string_view> read_line();
    
    // Check if buffer has complete message (contains \n\n or \r\n\r\n or \r\r)
    bool has_complete_message() const;
    
    // Get current buffer size
    size_t size() const noexcept { return buffer_.size(); }
    
    // Check if buffer is empty
    bool empty() const noexcept { return buffer_.empty(); }
    
    // Clear buffer
    void clear() noexcept { buffer_.clear(); }
    
    // Get raw buffer view (for testing/debugging)
    std::string_view view() const { return buffer_; }

private:
    std::string buffer_;  // Internal storage
    size_t read_pos_ = 0; // Current read position
    
    // Find end of line (\n, \r\n, or \r)
    size_t find_line_end(size_t start) const;
};

} // namespace sse
```

**Implementation Details:**
- Use `std::string` for automatic memory management
- Track read position separately from string size
- Handle all three line endings: `\n`, `\r\n`, `\r`
- `read_line()` returns string_view pointing into buffer (valid until next modification)
- `has_complete_message()` looks for `\n\n` pattern

**Key Methods:**

1. **append()**: Add data to buffer
   - Returns success or out_of_memory
   - String automatically grows

2. **read_line()**: Extract one line
   - Returns view into buffer (not copy)
   - Updates read_pos_
   - Handles partial lines (returns nullopt)

3. **has_complete_message()**: Check for SSE message boundary
   - SSE messages end with double newline
   - Check for: `\n\n`, `\r\n\r\n`, `\r\r`

**Acceptance Criteria:**
- [ ] append() adds data to buffer
- [ ] read_line() returns complete lines
- [ ] Handles all three line ending types
- [ ] has_complete_message() detects SSE boundaries
- [ ] Clear/empty/size work correctly
- [ ] No memory leaks (RAII via std::string)

**Test Requirements:**
- Basic append and read
- Line ending handling (LF, CRLF, CR)
- Empty line handling
- Large data handling
- Multiple append operations

**Dependencies:** T1, T2

---

### Task 5: Main Header Integration

**ID:** T5  
**Priority:** P1  
**Estimated Time:** 15 minutes

**Description:**
Create main public header that includes all components.

**Requirements:**
```cpp
// include/sse_parser/sse_parser.h
#pragma once

// Version info
#define SSE_PARSER_VERSION_MAJOR 1
#define SSE_PARSER_VERSION_MINOR 0
#define SSE_PARSER_VERSION_PATCH 0

// Core components
#include "error_codes.h"
#include "message.h"
#include "buffer.h"

// Convenience namespace alias
namespace sse_parser = sse;
```

**Acceptance Criteria:**
- [ ] All sub-headers included
- [ ] Version macros defined
- [ ] Single include provides all functionality
- [ ] Compiles without warnings

**Dependencies:** T2, T3, T4

---

### Task 6: Error Code Tests

**ID:** T6  
**Priority:** P0  
**Estimated Time:** 30 minutes

**Description:**
Implement comprehensive tests for error code system.

**Test File:** `tests/test_error_codes.cpp`

**Test Cases:**
```cpp
#include <gtest/gtest.h>
#include "sse_parser/error_codes.h"

using namespace sse;

TEST(ErrorCodes, EnumValues) {
    EXPECT_EQ(static_cast<uint8_t>(SseError::success), 0);
    EXPECT_NE(static_cast<uint8_t>(SseError::incomplete_message), 0);
    EXPECT_NE(static_cast<uint8_t>(SseError::invalid_retry), 0);
    EXPECT_NE(static_cast<uint8_t>(SseError::out_of_memory), 0);
}

TEST(ErrorCodes, ErrorMessages) {
    EXPECT_STREQ(error_message(SseError::success), "Success");
    EXPECT_STREQ(error_message(SseError::incomplete_message), "Message incomplete");
    EXPECT_STREQ(error_message(SseError::invalid_retry), "Invalid retry value");
    EXPECT_STREQ(error_message(SseError::out_of_memory), "Out of memory");
}

TEST(ErrorCodes, BoolConversion) {
    EXPECT_TRUE(!SseError::success);
    EXPECT_FALSE(!SseError::incomplete_message);
    EXPECT_FALSE(!SseError::invalid_retry);
    EXPECT_FALSE(!SseError::out_of_memory);
}

TEST(ErrorCodes, AllValuesHaveMessages) {
    // Ensure every enum value has a non-empty message
    EXPECT_NE(error_message(SseError::success)[0], '\0');
    EXPECT_NE(error_message(SseError::incomplete_message)[0], '\0');
    EXPECT_NE(error_message(SseError::invalid_retry)[0], '\0');
    EXPECT_NE(error_message(SseError::out_of_memory)[0], '\0');
}
```

**Acceptance Criteria:**
- [ ] All test cases pass
- [ ] 100% coverage of error_codes.h
- [ ] Tests compile without warnings

**Dependencies:** T2

---

### Task 7: Message Structure Tests

**ID:** T7  
**Priority:** P0  
**Estimated Time:** 25 minutes

**Description:**
Implement tests for Message struct.

**Test File:** `tests/test_message.cpp`

**Test Cases:**
```cpp
#include <gtest/gtest.h>
#include "sse_parser/message.h"

using namespace sse;

TEST(Message, DefaultConstruction) {
    Message msg;
    EXPECT_EQ(msg.event, "message");
    EXPECT_TRUE(msg.data.empty());
    EXPECT_FALSE(msg.id.has_value());
    EXPECT_FALSE(msg.retry.has_value());
}

TEST(Message, Clear) {
    Message msg;
    msg.event = "custom";
    msg.data = "test data";
    msg.id = "123";
    msg.retry = 5000;
    
    msg.clear();
    
    EXPECT_EQ(msg.event, "message");  // Reset to default
    EXPECT_TRUE(msg.data.empty());
    EXPECT_FALSE(msg.id.has_value());
    EXPECT_FALSE(msg.retry.has_value());
}

TEST(Message, Empty) {
    Message msg;
    EXPECT_TRUE(msg.empty());
    
    msg.data = "test";
    EXPECT_FALSE(msg.empty());
    
    msg.clear();
    EXPECT_TRUE(msg.empty());
}

TEST(Message, OptionalFields) {
    Message msg;
    
    // Set optional fields
    msg.id = "event-id";
    EXPECT_TRUE(msg.id.has_value());
    EXPECT_EQ(*msg.id, "event-id");
    
    msg.retry = 3000;
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(*msg.retry, 3000);
    
    // Clear optional
    msg.id.reset();
    EXPECT_FALSE(msg.id.has_value());
}

TEST(Message, LargeData) {
    Message msg;
    msg.data = std::string(10000, 'x');
    EXPECT_EQ(msg.data.size(), 10000);
    EXPECT_FALSE(msg.empty());
}
```

**Acceptance Criteria:**
- [ ] All test cases pass
- [ ] 100% coverage of message.h
- [ ] Edge cases covered (empty, large data)

**Dependencies:** T3

---

### Task 8: Buffer Tests

**ID:** T8  
**Priority:** P0  
**Estimated Time:** 45 minutes

**Description:**
Implement comprehensive tests for Buffer class.

**Test File:** `tests/test_buffer.cpp`

**Test Cases:**
```cpp
#include <gtest/gtest.h>
#include "sse_parser/buffer.h"

using namespace sse;

// Basic operations
TEST(Buffer, EmptyConstruction) {
    Buffer buf;
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0);
}

TEST(Buffer, AppendAndSize) {
    Buffer buf;
    EXPECT_EQ(buf.append("hello", 5), SseError::success);
    EXPECT_EQ(buf.size(), 5);
    EXPECT_FALSE(buf.empty());
}

TEST(Buffer, StringViewAppend) {
    Buffer buf;
    EXPECT_EQ(buf.append(std::string_view("test")), SseError::success);
    EXPECT_EQ(buf.size(), 4);
}

// Line reading - LF
TEST(Buffer, ReadLineLF) {
    Buffer buf;
    buf.append("line1\nline2\n", 12);
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(*line1, "line1\n");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(*line2, "line2\n");
    
    auto line3 = buf.read_line();
    EXPECT_FALSE(line3.has_value());
}

// Line reading - CRLF
TEST(Buffer, ReadLineCRLF) {
    Buffer buf;
    buf.append("line1\r\nline2\r\n", 14);
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(*line1, "line1\r\n");
}

// Line reading - CR only
TEST(Buffer, ReadLineCR) {
    Buffer buf;
    buf.append("line1\rline2\r", 12);
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(*line1, "line1\r");
}

// Incomplete line
TEST(Buffer, IncompleteLine) {
    Buffer buf;
    buf.append("incomplete", 10);
    
    auto line = buf.read_line();
    EXPECT_FALSE(line.has_value());
}

// Has complete message
TEST(Buffer, HasCompleteMessage) {
    Buffer buf;
    buf.append("data: hello\n\n", 13);
    EXPECT_TRUE(buf.has_complete_message());
}

TEST(Buffer, NoCompleteMessage) {
    Buffer buf;
    buf.append("data: hello\n", 12);
    EXPECT_FALSE(buf.has_complete_message());
}

TEST(Buffer, CompleteMessageCRLF) {
    Buffer buf;
    buf.append("data: hello\r\n\r\n", 16);
    EXPECT_TRUE(buf.has_complete_message());
}

// Clear
TEST(Buffer, Clear) {
    Buffer buf;
    buf.append("data", 4);
    EXPECT_FALSE(buf.empty());
    
    buf.clear();
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0);
}

// Multiple appends
TEST(Buffer, MultipleAppends) {
    Buffer buf;
    buf.append("data: ", 6);
    buf.append("hello", 5);
    buf.append("\n\n", 2);
    
    EXPECT_TRUE(buf.has_complete_message());
}

// Large data
TEST(Buffer, LargeData) {
    Buffer buf;
    std::string large(100000, 'x');
    large += "\n\n";
    
    EXPECT_EQ(buf.append(large), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
}

// View
TEST(Buffer, View) {
    Buffer buf;
    buf.append("test data", 9);
    EXPECT_EQ(buf.view(), "test data");
}
```

**Acceptance Criteria:**
- [ ] All test cases pass
- [ ] Line endings (LF, CRLF, CR) tested
- [ ] Edge cases covered (empty, incomplete, large)
- [ ] 90%+ coverage of buffer.h

**Dependencies:** T4

---

### Task 9: Integration Test

**ID:** T9  
**Priority:** P1  
**Estimated Time:** 20 minutes

**Description:**
Create integration test combining all Phase 1 components.

**Test File:** `tests/test_integration_p1.cpp`

**Test Cases:**
```cpp
#include <gtest/gtest.h>
#include "sse_parser/sse_parser.h"

using namespace sse;

// Test that all headers compile together
TEST(Phase1Integration, HeadersCompile) {
    // If this compiles and links, headers are correct
    EXPECT_TRUE(true);
}

// End-to-end buffer + error handling
TEST(Phase1Integration, BufferWithErrors) {
    Buffer buf;
    
    // Append SSE data
    EXPECT_EQ(buf.append("event: test\n"), SseError::success);
    EXPECT_EQ(buf.append("data: hello\n"), SseError::success);
    EXPECT_EQ(buf.append("\n"), SseError::success);
    
    // Should have complete message
    EXPECT_TRUE(buf.has_complete_message());
    
    // Read all lines
    int line_count = 0;
    while (buf.read_line().has_value()) {
        ++line_count;
    }
    EXPECT_EQ(line_count, 3);  // event line, data line, empty line
}

// Error message integration
TEST(Phase1Integration, ErrorHandling) {
    Buffer buf;
    
    // Normal operation
    EXPECT_EQ(buf.append("test\n"), SseError::success);
    EXPECT_STREQ(error_message(SseError::success), "Success");
    
    // Error case (simulate by checking error messages)
    EXPECT_STRNE(error_message(SseError::out_of_memory), "");
}
```

**Acceptance Criteria:**
- [ ] All headers compile together
- [ ] Components work together correctly
- [ ] Tests pass

**Dependencies:** T5, T6, T7, T8

---

### Task 10: Build System Update

**ID:** T10  
**Priority:** P1  
**Estimated Time:** 20 minutes

**Description:**
Update CMakeLists.txt to include new test files.

**Changes Required:**

1. **tests/CMakeLists.txt:**
```cmake
add_executable(${PROJECT_NAME}_tests 
    main_test.cpp
    test_error_codes.cpp
    test_message.cpp
    test_buffer.cpp
    test_integration_p1.cpp
)

target_link_libraries(${PROJECT_NAME}_tests 
    PRIVATE
    gtest_main
    gmock
)

target_include_directories(${PROJECT_NAME}_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)
```

2. **Root CMakeLists.txt (optional):**
   - Add header-only library target for IDE support

**Acceptance Criteria:**
- [ ] Tests compile with `cmake --build`
- [ ] All test files linked
- [ ] Include directories correct

**Dependencies:** T6, T7, T8, T9

---

## Dependency Graph

```
T1 (Project Structure)
    ├── T2 (Error Codes) ───┬── T6 (Error Tests)
    │                       │
    ├── T3 (Message) ───────┼── T7 (Message Tests)
    │                       │
    └── T4 (Buffer) ────────┴── T8 (Buffer Tests)
                                │
                                ├── T5 (Main Header)
                                │
                                └── T9 (Integration Test)
                                        │
                                        └── T10 (Build System)
```

**Critical Path:** T1 → T2/T3/T4 → T6/T7/T8 → T9 → T10

---

## Execution Order

**Wave 1 - Foundation (Parallel):**
1. T1: Create directory structure
2. T2: Error codes
3. T3: Message struct
4. T4: Buffer implementation

**Wave 2 - Tests (Parallel):**
5. T6: Error code tests
6. T7: Message tests
7. T8: Buffer tests

**Wave 3 - Integration:**
8. T5: Main header
9. T9: Integration test
10. T10: Build system

---

## Success Criteria

- [ ] All 6 core header files created
- [ ] All 4 test files created with 90%+ coverage
- [ ] All tests pass (`ctest`)
- [ ] No compiler warnings (C++17)
- [ ] Code follows project conventions

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Buffer complexity | Start with simple implementation, iterate |
| Test coverage | Use gcov to verify, add tests as needed |
| std::string performance | Document trade-off, consider future optimization |
| CMake issues | Test on available platform, document limitations |

---

## Next Phase Dependencies

Phase 2 (Core Parsing) will need:
- ✅ `SseError` for error reporting
- ✅ `Message` struct for assembled messages
- ✅ `Buffer` for data accumulation
- ⏳ State machine (to be built in Phase 2)
- ⏳ Field parsing logic (to be built in Phase 2)

---

*Plan created: 2025-03-17*  
*Ready for execution*
