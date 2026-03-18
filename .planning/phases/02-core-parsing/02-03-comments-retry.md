---
id: 02-03-comments-retry
phase: 02-core-parsing
wave: 2
autonomous: true
objective: Comments handling, retry validation, and leading space tests
req_ids:
  - PAR-03
  - PAR-04
  - VAL-01
---

# Plan 02-03: Comments, Validation, and Leading Space Tests

## Objective

Create comprehensive tests for:
- Leading space handling (PAR-03): Remove exactly one space after colon
- Comment line skipping (PAR-04): Lines starting with `:` are comments
- Retry field validation (VAL-01): Integer parsing with overflow detection

## Context

Based on 02-CONTEXT.md:

**Leading Space Handling (PAR-03):**
```cpp
// Per SSE spec: remove exactly one space after colon if present
"data: hello"   → field="data", value="hello"
"data:  hello"  → field="data", value=" hello" (keep second space)
"data:hello"    → field="data", value="hello"
```

**Comment Handling (PAR-04):**
- Start with `:` (colon as first character)
- Entire line is ignored
- Do NOT produce empty messages

**Retry Validation (VAL-01):**
- Must be valid integer
- Range: positive integer (typically milliseconds)
- Overflow detection: return `invalid_retry` error
- Non-numeric: return `invalid_retry` error

## Read First

<read_first>
- `.planning/phases/02-core-parsing/02-CONTEXT.md` (Leading Space, Comment Handling, Retry Validation sections)
- `.planning/REQUIREMENTS.md` (PAR-03, PAR-04, VAL-01 specifications)
- `include/sse_parser/field_parser.h` (if exists from Plan 02-01)
- `include/sse_parser/message.h` (Message struct for retry field)
- `include/sse_parser/error_codes.h` (SseError::invalid_retry)
</read_first>

## Tasks

### Task 1: Update field parser with retry validation improvements

<action type="read_file">
  <file_path>src/field_parser.cpp</file_path>
</action>

<action type="edit_file">
  <file_path>src/field_parser.cpp</file_path>
  <search>
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
}</search>
  <replace>
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
    
    // Check for leading zeros (optional: could allow single zero)
    // SSE spec allows any integer format
    
    // Parse as integer with overflow detection
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

// Validate that retry value is non-negative (0 is valid per SSE spec)
SseError validate_retry(int value) {
    if (value < 0) {
        return SseError::invalid_retry;
    }
    return SseError::success;
}</replace>
</action>

### Task 2: Create comprehensive test file

<action type="edit_file">
  <file_path>tests/test_field_parser.cpp</file_path>
  <search>
TEST(FieldParserTest, MultipleFieldsAccumulate) {
    Message msg;
    EXPECT_EQ(parse_field_line("event: myevent", msg), SseError::success);
    EXPECT_EQ(parse_field_line("data: mydata", msg), SseError::success);
    EXPECT_EQ(parse_field_line("id: myid", msg), SseError::success);
    
    EXPECT_EQ(msg.event, "myevent");
    EXPECT_EQ(msg.data, "mydata");
    EXPECT_TRUE(msg.id.has_value());
    EXPECT_EQ(msg.id.value(), "myid");
}</search>
  <replace>
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

// PAR-03: Comprehensive leading space tests
TEST(FieldParserLeadingSpace, SingleSpaceRemoved) {
    Message msg;
    SseError err = parse_field_line("data: hello", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.data, "hello");
}

TEST(FieldParserLeadingSpace, SingleSpaceOnlyOneRemoved) {
    Message msg;
    SseError err = parse_field_line("data: hello world", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.data, "hello world");  // Only first space after colon removed
}

TEST(FieldParserLeadingSpace, MultipleSpacesKeepRest) {
    Message msg;
    SseError err = parse_field_line("data:   hello", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.data, "  hello");  // First space removed, two kept
}

TEST(FieldParserLeadingSpace, TabNotRemoved) {
    Message msg;
    SseError err = parse_field_line("data:\thello", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.data, "\thello");  // Tab is not a space, kept as-is
}

TEST(FieldParserLeadingSpace, AppliesToAllFields) {
    Message msg;
    
    // Event field
    EXPECT_EQ(parse_field_line("event: myevent", msg), SseError::success);
    EXPECT_EQ(msg.event, "myevent");
    
    msg.clear();
    
    // ID field
    EXPECT_EQ(parse_field_line("id: myid", msg), SseError::success);
    EXPECT_TRUE(msg.id.has_value());
    EXPECT_EQ(msg.id.value(), "myid");
    
    msg.clear();
    
    // Retry field (whitespace trimming happens in parse_retry_value)
    EXPECT_EQ(parse_field_line("retry: 5000", msg), SseError::success);
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(msg.retry.value(), 5000);
}

// PAR-04: Comprehensive comment tests
TEST(FieldParserComment, SimpleCommentSkipped) {
    Message msg;
    msg.event = "original";
    SseError err = parse_field_line(": this is a comment", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.event, "original");
}

TEST(FieldParserComment, EmptyCommentLine) {
    Message msg;
    msg.event = "original";
    SseError err = parse_field_line(":", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.event, "original");
}

TEST(FieldParserComment, CommentWithColon) {
    Message msg;
    msg.event = "original";
    SseError err = parse_field_line(": comment: with colon", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.event, "original");
}

TEST(FieldParserComment, CommentWithFieldLikeContent) {
    Message msg;
    msg.event = "original";
    SseError err = parse_field_line(": data: fake field", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(msg.event, "original");
    EXPECT_TRUE(msg.data.empty());
}

TEST(FieldParserComment, CommentAfterField) {
    // Parse a field first, then comment
    Message msg;
    EXPECT_EQ(parse_field_line("event: myevent", msg), SseError::success);
    EXPECT_EQ(msg.event, "myevent");
    
    EXPECT_EQ(parse_field_line(": this is a comment", msg), SseError::success);
    EXPECT_EQ(msg.event, "myevent");  // Unchanged
}

// VAL-01: Comprehensive retry validation tests
TEST(FieldParserRetryValidation, ValidSmall) {
    Message msg;
    SseError err = parse_field_line("retry: 1", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(msg.retry.value(), 1);
}

TEST(FieldParserRetryValidation, ValidZero) {
    Message msg;
    SseError err = parse_field_line("retry: 0", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(msg.retry.value(), 0);
}

TEST(FieldParserRetryValidation, ValidWithLeadingZeros) {
    Message msg;
    SseError err = parse_field_line("retry: 0005000", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(msg.retry.value(), 5000);
}

TEST(FieldParserRetryValidation, ValidWithWhitespace) {
    Message msg;
    SseError err = parse_field_line("retry:  5000  ", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(msg.retry.value(), 5000);
}

TEST(FieldParserRetryValidation, InvalidEmpty) {
    Message msg;
    SseError err = parse_field_line("retry:", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserRetryValidation, InvalidWhitespaceOnly) {
    Message msg;
    SseError err = parse_field_line("retry:   ", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserRetryValidation, InvalidNegative) {
    Message msg;
    SseError err = parse_field_line("retry: -1", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserRetryValidation, InvalidNegativeLarge) {
    Message msg;
    SseError err = parse_field_line("retry: -999999", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserRetryValidation, InvalidNonNumeric) {
    Message msg;
    SseError err = parse_field_line("retry: abc", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserRetryValidation, InvalidMixed) {
    Message msg;
    SseError err = parse_field_line("retry: 123abc", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserRetryValidation, InvalidDecimal) {
    Message msg;
    SseError err = parse_field_line("retry: 123.45", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserRetryValidation, InvalidHex) {
    Message msg;
    SseError err = parse_field_line("retry: 0xFF", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserRetryValidation, OverflowMaxInt) {
    Message msg;
    // INT_MAX + 1 should overflow
    SseError err = parse_field_line("retry: 2147483648", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserRetryValidation, OverflowLargeNumber) {
    Message msg;
    SseError err = parse_field_line("retry: 999999999999999999999", msg);
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(FieldParserRetryValidation, OverflowJustUnderLimit) {
    Message msg;
    // INT_MAX (typically 2147483647) should be valid
    SseError err = parse_field_line("retry: 2147483647", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(msg.retry.value(), 2147483647);
}</replace>
</action>

### Task 3: Add BOM detection header (EXT-01)

<action type="edit_file">
  <file_path>include/sse_parser/field_parser.h</file_path>
  <search>
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

} // namespace sse</search>
  <replace>
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

// EXT-01: Check and skip UTF-8 BOM
// Returns: true if BOM was found and skipped, false otherwise
// If BOM found, the view is advanced past the BOM bytes
bool skip_bom(std::string_view& data);

// EXT-01: Check if data starts with UTF-8 BOM
bool has_bom(std::string_view data);

} // namespace sse</replace>
</action>

### Task 4: Implement BOM detection

<action type="edit_file">
  <file_path>src/field_parser.cpp</file_path>
  <search>
SseError validate_retry(int value) {
    if (value < 0) {
        return SseError::invalid_retry;
    }
    return SseError::success;
}

} // namespace sse</search>
  <replace>
SseError validate_retry(int value) {
    if (value < 0) {
        return SseError::invalid_retry;
    }
    return SseError::success;
}

// EXT-01: UTF-8 BOM detection and skipping
// UTF-8 BOM sequence: \xEF\xBB\xBF
bool skip_bom(std::string_view& data) {
    if (has_bom(data)) {
        data = data.substr(3);  // Skip 3-byte BOM
        return true;
    }
    return false;
}

bool has_bom(std::string_view data) {
    static constexpr unsigned char BOM[] = {0xEF, 0xBB, 0xBF};
    return data.size() >= 3 &&
           static_cast<unsigned char>(data[0]) == BOM[0] &&
           static_cast<unsigned char>(data[1]) == BOM[1] &&
           static_cast<unsigned char>(data[2]) == BOM[2];
}

} // namespace sse</replace>
</action>

### Task 5: Add BOM tests

<action type="edit_file">
  <file_path>tests/test_field_parser.cpp</file_path>
  <search>
TEST(FieldParserRetryValidation, OverflowJustUnderLimit) {
    Message msg;
    // INT_MAX (typically 2147483647) should be valid
    SseError err = parse_field_line("retry: 2147483647", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(msg.retry.value(), 2147483647);
}</search>
  <replace>
TEST(FieldParserRetryValidation, OverflowJustUnderLimit) {
    Message msg;
    // INT_MAX (typically 2147483647) should be valid
    SseError err = parse_field_line("retry: 2147483647", msg);
    EXPECT_EQ(err, SseError::success);
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(msg.retry.value(), 2147483647);
}

// EXT-01: UTF-8 BOM detection tests
TEST(FieldParserBOM, HasBOM_True) {
    std::string_view data("\xEF\xBB\xBFdata: hello");
    EXPECT_TRUE(has_bom(data));
}

TEST(FieldParserBOM, HasBOM_False) {
    std::string_view data("data: hello");
    EXPECT_FALSE(has_bom(data));
}

TEST(FieldParserBOM, HasBOM_TooShort) {
    std::string_view data("\xEF\xBB");
    EXPECT_FALSE(has_bom(data));
}

TEST(FieldParserBOM, HasBOM_Empty) {
    std::string_view data("");
    EXPECT_FALSE(has_bom(data));
}

TEST(FieldParserBOM, SkipBOM_Success) {
    std::string_view data("\xEF\xBB\xBFdata: hello");
    bool skipped = skip_bom(data);
    EXPECT_TRUE(skipped);
    EXPECT_EQ(data, "data: hello");
}

TEST(FieldParserBOM, SkipBOM_NoBOM) {
    std::string_view data("data: hello");
    bool skipped = skip_bom(data);
    EXPECT_FALSE(skipped);
    EXPECT_EQ(data, "data: hello");
}</replace>
</action>

## Verification

<action type="bash">
  <command>cd build && cmake .. && cmake --build . --target run_tests 2>&1 | grep -E "(PASSED|FAILED|LeadingSpace|Comment|RetryValidation|BOM)"</command>
</action>

## Acceptance Criteria

<acceptance_criteria>
1. Leading space tests (PAR-03) - verify with grep:
   - `grep "SingleSpaceRemoved" tests/test_field_parser.cpp` ✓
   - `grep "MultipleSpacesKeepRest" tests/test_field_parser.cpp` ✓
   - `grep "TabNotRemoved" tests/test_field_parser.cpp` ✓
2. Comment tests (PAR-04) - verify with grep:
   - `grep "SimpleCommentSkipped" tests/test_field_parser.cpp` ✓
   - `grep "CommentWithColon" tests/test_field_parser.cpp` ✓
   - `grep "CommentWithFieldLikeContent" tests/test_field_parser.cpp` ✓
3. Retry validation tests (VAL-01) - verify with grep:
   - `grep "ValidSmall\|ValidZero\|ValidWithLeadingZeros" tests/test_field_parser.cpp` ✓
   - `grep "InvalidNegative\|InvalidNonNumeric" tests/test_field_parser.cpp` ✓
   - `grep "OverflowMaxInt\|OverflowLargeNumber" tests/test_field_parser.cpp` ✓
4. BOM detection (EXT-01) - verify with grep:
   - `grep "skip_bom\|has_bom" include/sse_parser/field_parser.h` ✓
   - `grep "HasBOM_\|SkipBOM_" tests/test_field_parser.cpp` ✓
5. Total test count: `grep -c "TEST(FieldParser" tests/test_field_parser.cpp` should be ≥ 30
6. All tests pass: `grep "PASSED.*FieldParser" build/Testing/Temporary/LastTest.log`
</acceptance_criteria>

## Traceability

| Req ID | Coverage |
|--------|----------|
| PAR-03 | Full - leading space removal with edge cases |
| PAR-04 | Full - comment line skipping with variations |
| VAL-01 | Full - retry validation including overflow |
| EXT-01 | Full - UTF-8 BOM detection and skipping |
