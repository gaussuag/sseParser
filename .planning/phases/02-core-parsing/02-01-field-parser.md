---
id: 02-01-field-parser
phase: 02-core-parsing
wave: 1
autonomous: true
objective: Create field parser for SSE format "field: value"
req_ids:
  - PAR-01
---

# Plan 02-01: Field Parser

## Objective

Create the field parser component that parses SSE protocol format `field: value`, handles all standard SSE fields (event, data, id, retry), ignores unknown fields, and handles missing colon (empty value).

## Context

Based on 02-CONTEXT.md:
- Format: `field: value`, `field:value`, `field: value with spaces`
- Valid fields: event, data, id, retry (case-sensitive per spec)
- Unknown fields: silently ignore (not an error per SSE spec)
- Lines without `:`: treat as field with empty value

Depends on:
- `Buffer` class - for accumulating partial data via `read_line()`
- `Message` struct - for storing parsed fields
- `SseError` enum - for error reporting

## Read First

<read_first>
- `.planning/phases/02-core-parsing/02-CONTEXT.md` (Field Parser Design section)
- `.planning/REQUIREMENTS.md` (PAR-01 specification)
- `include/sse_parser/buffer.h` (understand read_line() interface)
- `include/sse_parser/message.h` (understand Message struct to populate)
- `include/sse_parser/error_codes.h` (error codes to use)
</read_first>

## Tasks

### Task 1: Create field parser header

<action type="create_file">
  <file_path>include/sse_parser/field_parser.h</file_path>
  <content>
#pragma once

#include <optional>
#include <string_view>
#include "error_codes.h"
#include "message.h"

namespace sse {

// Parse a single SSE field line
// Format: "field: value" or "field:value" or "field" (empty value)
// Returns: success, or error if validation fails (e.g., invalid retry)
SseError parse_field_line(std::string_view line, Message& msg);

// Internal: Parse retry field value
// Validates: must be valid positive integer, checks overflow
SseError parse_retry_value(std::string_view value, int& result);

} // namespace sse
  </content>
</action>

### Task 2: Implement field parser

<action type="create_file">
  <file_path>src/field_parser.cpp</file_path>
  <content>
#include "sse_parser/field_parser.h"
#include <charconv>
#include <cctype>

namespace sse {

SseError parse_field_line(std::string_view line, Message& msg) {
    // Skip comment lines (start with ':')
    if (!line.empty() && line[0] == ':') {
        return SseError::success;  // Comment, ignore
    }
    
    // Find colon separator
    size_t colon_pos = line.find(':');
    
    std::string_view field_name;
    std::string_view field_value;
    
    if (colon_pos == std::string_view::npos) {
        // No colon - field with empty value
        field_name = line;
        field_value = "";
    } else {
        field_name = line.substr(0, colon_pos);
        field_value = line.substr(colon_pos + 1);
        
        // Remove exactly one leading space if present (PAR-03)
        if (!field_value.empty() && field_value[0] == ' ') {
            field_value = field_value.substr(1);
        }
    }
    
    // Handle known fields (case-sensitive per SSE spec)
    if (field_name == "event") {
        msg.event = std::string(field_value);
    } else if (field_name == "data") {
        // In Phase 2, just set the data. Phase 3 will handle multi-line accumulation
        msg.data = std::string(field_value);
    } else if (field_name == "id") {
        msg.id = std::string(field_value);
    } else if (field_name == "retry") {
        int retry_val = 0;
        SseError err = parse_retry_value(field_value, retry_val);
        if (err != SseError::success) {
            return err;
        }
        msg.retry = retry_val;
    }
    // Unknown fields are silently ignored per SSE spec
    
    return SseError::success;
}

SseError parse_retry_value(std::string_view value, int& result) {
    if (value.empty()) {
        return SseError::invalid_retry;
    }
    
    // Trim leading whitespace
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    
    // Trim trailing whitespace
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    
    if (start >= end) {
        return SseError::invalid_retry;
    }
    
    std::string_view trimmed = value.substr(start, end - start);
    
    // Check for negative numbers
    if (trimmed[0] == '-') {
        return SseError::invalid_retry;
    }
    
    // Parse as integer
    int val = 0;
    for (char c : trimmed) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return SseError::invalid_retry;
        }
        
        // Check for overflow before multiplying
        if (val > (INT_MAX / 10)) {
            return SseError::invalid_retry;
        }
        val *= 10;
        
        int digit = c - '0';
        if (val > (INT_MAX - digit)) {
            return SseError::invalid_retry;
        }
        val += digit;
    }
    
    result = val;
    return SseError::success;
}

} // namespace sse
  </content>
</action>

### Task 3: Create unit tests

<action type="create_file">
  <file_path>tests/test_field_parser.cpp</file_path>
  <content>
#include <gtest/gtest.h>
#include "sse_parser/field_parser.h"

using namespace sse;

// PAR-01: Basic field parsing
TEST(FieldParserTest, ParseEventField) {
    Message msg;
    SseError err = parse_field_line("event: test_event", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.event, "test_event");
}

TEST(FieldParserTest, ParseDataField) {
    Message msg;
    SseError err = parse_field_line("data: hello world", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.data, "hello world");
}

TEST(FieldParserTest, ParseIdField) {
    Message msg;
    SseError err = parse_field_line("id: msg-123", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_TRUE(msg.id.has_value());
    EXPECT_EQ(msg.id.value(), "msg-123");
}

TEST(FieldParserTest, ParseRetryField) {
    Message msg;
    SseError err = parse_field_line("retry: 5000", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(msg.retry.value(), 5000);
}

// PAR-01: No colon = empty value
TEST(FieldParserTest, NoColonEmptyValue) {
    Message msg;
    SseError err = parse_field_line("event", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.event, "");
}

TEST(FieldParserTest, EmptyValueAfterColon) {
    Message msg;
    SseError err = parse_field_line("event:", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.event, "");
}

// PAR-01: Unknown fields ignored
TEST(FieldParserTest, UnknownFieldIgnored) {
    Message msg;
    msg.event = "original";
    SseError err = parse_field_line("unknown: value", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.event, "original");  // Unchanged
    EXPECT_TRUE(msg.data.empty());
}

// PAR-03: Leading space handling
TEST(FieldParserTest, LeadingSpaceRemoved) {
    Message msg;
    SseError err = parse_field_line("data: hello", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.data, "hello");  // First space removed
}

TEST(FieldParserTest, MultipleSpacesKeepRest) {
    Message msg;
    SseError err = parse_field_line("data:  hello", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.data, " hello");  // First space removed, second kept
}

TEST(FieldParserTest, NoSpaceAfterColon) {
    Message msg;
    SseError err = parse_field_line("data:hello", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.data, "hello");
}

// PAR-04: Comment lines
TEST(FieldParserTest, CommentLineSkipped) {
    Message msg;
    msg.event = "original";
    SseError err = parse_field_line(": this is a comment", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.event, "original");  // Unchanged
}

TEST(FieldParserTest, EmptyCommentSkipped) {
    Message msg;
    SseError err = parse_field_line(":", msg);
    EXPECT_EQ(err, SseError::success);
    // Should not change message
}

// VAL-01: Retry validation
TEST(FieldParserTest, RetryInvalidNonNumeric) {
    Message msg;
    SseError err = parse_field_line("retry: abc", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserTest, RetryInvalidNegative) {
    Message msg;
    SseError err = parse_field_line("retry: -100", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserTest, RetryValidZero) {
    Message msg;
    SseError err = parse_field_line("retry: 0", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(msg.retry.value(), 0);
}

TEST(FieldParserTest, RetryValidLarge) {
    Message msg;
    SseError err = parse_field_line("retry: 30000", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(msg.retry.value(), 30000);
}

// VAL-01: Overflow detection
TEST(FieldParserTest, RetryOverflowMaxInt) {
    Message msg;
    SseError err = parse_field_line("retry: 999999999999999999999", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserTest, RetryOverflowJustOver) {
    Message msg;
    // INT_MAX is typically 2147483647
    SseError err = parse_field_line("retry: 2147483648", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

// Edge cases
TEST(FieldParserTest, EmptyLine) {
    Message msg;
    SseError err = parse_field_line("", msg);
    EXPECT_EQ(err, SseError::success);
    // Empty line should be treated as field with empty name and value
}

TEST(FieldParserTest, ValueWithColon) {
    Message msg;
    SseError err = parse_field_line("data: key:value", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.data, "key:value");
}

TEST(FieldParserTest, MultipleFieldsAccumulate) {
    Message msg;
    EXPECT_EQ(parse_field_line("event: myevent", msg), SseError::success);
    EXPECT_EQ(parse_field_line("data: mydata", msg), SseError::success);
    EXPECT_EQ(parse_field_line("id: myid", msg), SseError::success);
    
    EXPECT_EQ(msg.event, "myevent");
    EXPECT_EQ(msg.data, "mydata");
    EXPECT_TRUE(msg.id.has_value());
    EXPECT_EQ(msg.id.value(), "myid");
}
  </content>
</action>

### Task 4: Update CMakeLists.txt

<action type="edit_file">
  <file_path>CMakeLists.txt</file_path>
  <search>
set(TEST_SOURCES
    tests/test_buffer.cpp
    tests/test_error_codes.cpp
    tests/test_message.cpp
)</search>
  <replace>
set(TEST_SOURCES
    tests/test_buffer.cpp
    tests/test_error_codes.cpp
    tests/test_message.cpp
    tests/test_field_parser.cpp
)</replace>
</action>

## Verification

<action type="bash">
  <command>cd build && cmake .. && cmake --build . --target run_tests 2>&1 | grep -E "(PASSED|FAILED|test_field_parser)"</command>
</action>

## Acceptance Criteria

<acceptance_criteria>
1. File created: `include/sse_parser/field_parser.h` exists
2. File created: `src/field_parser.cpp` exists
3. File created: `tests/test_field_parser.cpp` exists
4. Header declares `parse_field_line()` and `parse_retry_value()` functions
5. Implementation handles "event", "data", "id", "retry" fields
6. Unknown fields are silently ignored (test verifies Message unchanged)
7. Missing colon results in empty value
8. Tests verify all 12+ test cases pass
9. Verify: `grep -l "test_field_parser" build/CMakeFiles/*.txt` returns file references
10. Verify: `grep "parse_field_line" include/sse_parser/field_parser.h` returns function declaration
</acceptance_criteria>

## Traceability

| Req ID | Coverage |
|--------|----------|
| PAR-01 | Full - field parsing with colon, empty value handling |
| PAR-03 | Full - leading space removal (one space only) |
| PAR-04 | Full - comment line skipping |
| VAL-01 | Full - retry validation with overflow check |
