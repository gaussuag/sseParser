---
id: 02-02-line-endings
phase: 02-core-parsing
wave: 1
autonomous: true
objective: Test all 9 combinations of line endings (LF/CRLF/CR)
req_ids:
  - PAR-02
---

# Plan 02-02: Line Ending Handler Tests

## Objective

Create comprehensive tests for all 9 combinations of line endings (LF, CRLF, CR) to verify that message boundary detection works correctly regardless of line ending style or mixed styles.

## Context

Based on 02-CONTEXT.md:

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

**Implementation Note:** Buffer already handles all three line endings in `read_line()`. This phase tests that double line endings (message boundaries) are correctly detected.

## Read First

<read_first>
- `.planning/phases/02-core-parsing/02-CONTEXT.md` (Line Ending Handler section)
- `.planning/REQUIREMENTS.md` (PAR-02 specification)
- `include/sse_parser/buffer.h` (understand has_complete_message() and read_line())
- `.planning/TESTING.md` (testing strategy and conventions)
</read_first>

## Tasks

### Task 1: Create line endings test file

<action type="create_file">
  <file_path>tests/test_line_endings.cpp</file_path>
  <content>
#include <gtest/gtest.h>
#include "sse_parser/buffer.h"

using namespace sse;

// Helper: Create string with specific line ending
static std::string make_line_ending(const char* ending) {
    if (std::string_view(ending) == "LF") return "\n";
    if (std::string_view(ending) == "CRLF") return "\r\n";
    if (std::string_view(ending) == "CR") return "\r";
    return "";
}

// PAR-02: Test Matrix - 9 combinations
// Format: first_line_ending + second_line_ending = message boundary

TEST(LineEndingsTest, LF_LF_UnixStyle) {
    Buffer buf;
    // data: hello\n\n -> complete message
    ASSERT_EQ(buf.append("data: hello\n\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");  // Empty line = message boundary
}

TEST(LineEndingsTest, LF_CRLF_Mixed) {
    Buffer buf;
    // data: hello\n\r\n -> complete message
    ASSERT_EQ(buf.append("data: hello\n\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, LF_CR_Mixed) {
    Buffer buf;
    // data: hello\n\r -> complete message
    ASSERT_EQ(buf.append("data: hello\n\r"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, CRLF_LF_Mixed) {
    Buffer buf;
    // data: hello\r\n\n -> complete message
    ASSERT_EQ(buf.append("data: hello\r\n\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, CRLF_CRLF_HTTPStyle) {
    Buffer buf;
    // data: hello\r\n\r\n -> complete message (HTTP standard)
    ASSERT_EQ(buf.append("data: hello\r\n\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, CRLF_CR_Mixed) {
    Buffer buf;
    // data: hello\r\n\r -> complete message
    ASSERT_EQ(buf.append("data: hello\r\n\r"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, CR_LF_Mixed) {
    Buffer buf;
    // data: hello\r\n -> complete message
    ASSERT_EQ(buf.append("data: hello\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, CR_CRLF_Mixed) {
    Buffer buf;
    // data: hello\r\r\n -> complete message
    ASSERT_EQ(buf.append("data: hello\r\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, CR_CR_ClassicMac) {
    Buffer buf;
    // data: hello\r\r -> complete message (Classic Mac)
    ASSERT_EQ(buf.append("data: hello\r\r"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

// Additional edge case tests

TEST(LineEndingsTest, IncompleteMessageNoBoundary) {
    Buffer buf;
    // data: hello\n -> incomplete (no second newline)
    ASSERT_EQ(buf.append("data: hello\n"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
}

TEST(LineEndingsTest, MultipleLinesInMessage) {
    Buffer buf;
    // event: test\ndata: hello\n\n -> complete message with 2 lines
    ASSERT_EQ(buf.append("event: test\ndata: hello\n\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "event: test");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "data: hello");
    
    auto line3 = buf.read_line();
    ASSERT_TRUE(line3.has_value());
    EXPECT_EQ(line3.value(), "");  // Boundary
}

TEST(LineEndingsTest, MixedLineEndingsInOneStream) {
    Buffer buf;
    // event: test\r\ndata: hello\n\r\n -> mixed CRLF and LF
    ASSERT_EQ(buf.append("event: test\r\ndata: hello\n\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "event: test");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "data: hello");
    
    auto line3 = buf.read_line();
    ASSERT_TRUE(line3.has_value());
    EXPECT_EQ(line3.value(), "");  // Boundary
}

TEST(LineEndingsTest, EmptyLineCRLF) {
    Buffer buf;
    // \r\n\r\n -> empty message
    ASSERT_EQ(buf.append("\r\n\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "");  // First empty line
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");  // Second empty line (boundary)
}

TEST(LineEndingsTest, TrailingCRHandled) {
    Buffer buf;
    // data: hello\r -> single CR at end
    ASSERT_EQ(buf.append("data: hello\r"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    // Now add another CR to complete
    ASSERT_EQ(buf.append("\r"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, TrailingLFHandled) {
    Buffer buf;
    // data: hello\n -> single LF at end
    ASSERT_EQ(buf.append("data: hello\n"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    // Now add another LF to complete
    ASSERT_EQ(buf.append("\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, TrailingCRLFHandled) {
    Buffer buf;
    // data: hello\r\n -> single CRLF at end
    ASSERT_EQ(buf.append("data: hello\r\n"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    // Now add another CRLF to complete
    ASSERT_EQ(buf.append("\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}
  </content>
</action>

### Task 2: Update CMakeLists.txt

<action type="edit_file">
  <file_path>CMakeLists.txt</file_path>
  <search>
set(TEST_SOURCES
    tests/test_buffer.cpp
    tests/test_error_codes.cpp
    tests/test_message.cpp
    tests/test_field_parser.cpp
)</search>
  <replace>
set(TEST_SOURCES
    tests/test_buffer.cpp
    tests/test_error_codes.cpp
    tests/test_message.cpp
    tests/test_field_parser.cpp
    tests/test_line_endings.cpp
)</replace>
</action>

## Verification

<action type="bash">
  <command>cd build && cmake .. && cmake --build . --target run_tests 2>&1 | grep -E "(PASSED|FAILED|LineEndings)"</command>
</action>

## Acceptance Criteria

<acceptance_criteria>
1. File created: `tests/test_line_endings.cpp` exists
2. All 9 line ending combinations are tested:
   - grep "TEST(LineEndingsTest, LF_LF" tests/test_line_endings.cpp (LF+LF)
   - grep "TEST(LineEndingsTest, LF_CRLF" tests/test_line_endings.cpp (LF+CRLF)
   - grep "TEST(LineEndingsTest, LF_CR" tests/test_line_endings.cpp (LF+CR)
   - grep "TEST(LineEndingsTest, CRLF_LF" tests/test_line_endings.cpp (CRLF+LF)
   - grep "TEST(LineEndingsTest, CRLF_CRLF" tests/test_line_endings.cpp (CRLF+CRLF)
   - grep "TEST(LineEndingsTest, CRLF_CR" tests/test_line_endings.cpp (CRLF+CR)
   - grep "TEST(LineEndingsTest, CR_LF" tests/test_line_endings.cpp (CR+LF)
   - grep "TEST(LineEndingsTest, CR_CRLF" tests/test_line_endings.cpp (CR+CRLF)
   - grep "TEST(LineEndingsTest, CR_CR" tests/test_line_endings.cpp (CR+CR)
3. Test verifies `has_complete_message()` returns true for each combination
4. Test verifies `read_line()` correctly extracts content without line endings
5. Edge case tests: incomplete messages, multiple lines, mixed endings
6. Total 15+ test cases covering matrix and edge cases
7. Tests pass: `grep "PASSED.*LineEndings" build/Testing/Temporary/LastTest.log`
8. CMakeLists.txt updated to include new test file
</acceptance_criteria>

## Traceability

| Req ID | Coverage |
|--------|----------|
| PAR-02 | Full - all 9 line ending combinations tested |
