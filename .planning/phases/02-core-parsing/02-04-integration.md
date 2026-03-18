---
id: 02-04-integration
phase: 02-core-parsing
wave: 2
autonomous: true
objective: Phase 2 integration tests - end-to-end parsing scenarios
req_ids:
  - PAR-01
  - PAR-02
  - PAR-03
  - PAR-04
  - VAL-01
---

# Plan 02-04: Phase 2 Integration Tests

## Objective

Create comprehensive integration tests that combine all Phase 2 components:
- Field parsing (PAR-01)
- Line ending handling (PAR-02)
- Leading space handling (PAR-03)
- Comment skipping (PAR-04)
- Retry validation (VAL-01)

Tests real-world scenarios with mixed line endings, error propagation, and complete message flows.

## Context

Based on 02-CONTEXT.md:

Phase 2 implements the core SSE protocol parsing logic. This plan creates integration tests that exercise the complete parsing pipeline from Buffer → read_line() → parse_field_line() → Message population.

**Key Integration Points:**
1. Buffer reads lines with any line ending
2. Field parser processes each line
3. Messages are populated with parsed values
4. Errors propagate correctly through the pipeline

## Read First

<read_first>
- `.planning/phases/02-core-parsing/02-CONTEXT.md` (all sections)
- `.planning/REQUIREMENTS.md` (PAR-01/02/03/04, VAL-01)
- `.planning/ROADMAP.md` (Phase 2 success criteria)
- `include/sse_parser/buffer.h` (Buffer class for integration)
- `include/sse_parser/field_parser.h` (parse_field_line function)
- `include/sse_parser/message.h` (Message struct for verification)
- `include/sse_parser/error_codes.h` (Error handling)
</read_first>

## Tasks

### Task 1: Create integration test file

<action type="create_file">
  <file_path>tests/test_integration_p2.cpp</file_path>
  <content>
#include <gtest/gtest.h>
#include "sse_parser/buffer.h"
#include "sse_parser/field_parser.h"
#include "sse_parser/message.h"

using namespace sse;

// Helper: Parse a complete SSE message from buffer
// Returns true if a complete message was parsed
static bool parse_message(Buffer& buf, Message& msg) {
    if (!buf.has_complete_message()) {
        return false;
    }
    
    msg.clear();
    
    while (true) {
        auto line_opt = buf.read_line();
        if (!line_opt.has_value()) {
            break;
        }
        
        std::string_view line = line_opt.value();
        
        // Empty line indicates message boundary
        if (line.empty()) {
            return true;
        }
        
        SseError err = parse_field_line(line, msg);
        if (err != SseError::success) {
            // Error in field parsing - propagate
            return false;
        }
    }
    
    return true;
}

// Helper: Parse a simple data-only message
static SseError parse_simple_message(const char* input, Message& msg) {
    Buffer buf;
    SseError err = buf.append(input, std::strlen(input));
    if (err != SseError::success) {
        return err;
    }
    
    if (!parse_message(buf, msg)) {
        return SseError::incomplete_message;
    }
    
    return SseError::success;
}

// =============================================================================
// Integration Test Suite: Complete Message Parsing
// =============================================================================

// PAR-01 + PAR-02: Basic complete message parsing
TEST(Phase2Integration, CompleteDataMessage_LF) {
    Buffer buf;
    ASSERT_EQ(buf.append("data: hello\n\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.data, "hello");
}

TEST(Phase2Integration, CompleteDataMessage_CRLF) {
    Buffer buf;
    ASSERT_EQ(buf.append("data: hello\r\n\r\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.data, "hello");
}

TEST(Phase2Integration, CompleteDataMessage_CR) {
    Buffer buf;
    ASSERT_EQ(buf.append("data: hello\r\r"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.data, "hello");
}

// PAR-01: Multi-field message
TEST(Phase2Integration, MultiFieldMessage) {
    Buffer buf;
    ASSERT_EQ(buf.append("event: myevent\n"
                         "data: mydata\n"
                         "id: myid\n"
                         "retry: 5000\n"
                         "\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.event, "myevent");
    EXPECT_EQ(msg.data, "mydata");
    EXPECT_TRUE(msg.id.has_value());
    EXPECT_EQ(msg.id.value(), "myid");
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(msg.retry.value(), 5000);
}

// PAR-02: Mixed line endings in single stream
TEST(Phase2Integration, MixedLineEndings) {
    Buffer buf;
    // Mix LF and CRLF in one message
    ASSERT_EQ(buf.append("event: test\r\n"
                         "data: hello\n"
                         "\r\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.event, "test");
    EXPECT_EQ(msg.data, "hello");
}

// PAR-03: Leading space preserved correctly
TEST(Phase2Integration, LeadingSpaceHandling) {
    Buffer buf;
    ASSERT_EQ(buf.append("data:  hello world\n\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.data, " hello world");  // First space removed
}

// PAR-04: Comments skipped in message
TEST(Phase2Integration, CommentLinesSkipped) {
    Buffer buf;
    ASSERT_EQ(buf.append("event: test\n"
                         ": this is a comment\n"
                         "data: hello\n"
                         ": another comment\n"
                         "\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.event, "test");
    EXPECT_EQ(msg.data, "hello");
}

// VAL-01: Retry validation error propagation
TEST(Phase2Integration, RetryValidationError) {
    Buffer buf;
    ASSERT_EQ(buf.append("retry: invalid\n\n"), SseError::success);
    
    Message msg;
    // parse_message should detect error and return false
    bool result = parse_message(buf, msg);
    EXPECT_FALSE(result);
    // Note: retry should not be set due to validation error
    EXPECT_FALSE(msg.retry.has_value());
}

// VAL-01: Retry overflow error
TEST(Phase2Integration, RetryOverflowError) {
    Buffer buf;
    ASSERT_EQ(buf.append("retry: 999999999999999\n\n"), SseError::success);
    
    Message msg;
    bool result = parse_message(buf, msg);
    EXPECT_FALSE(result);
}

// =============================================================================
// Integration Test Suite: Chunked Input Simulation
// =============================================================================

TEST(Phase2Integration, ChunkedDataArrival) {
    Buffer buf;
    Message msg;
    
    // First chunk: incomplete line
    ASSERT_EQ(buf.append("data: hel"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    // Second chunk: complete line but no boundary
    ASSERT_EQ(buf.append("lo\ndata: world"), SseError::success);
    // Still no complete message (need double newline)
    EXPECT_FALSE(buf.has_complete_message());
    
    // Third chunk: message boundary
    ASSERT_EQ(buf.append("\n\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    // Parse the message
    ASSERT_TRUE(parse_message(buf, msg));
    // Note: In Phase 2, each data line overwrites, so we get last value
    EXPECT_EQ(msg.data, "world");
}

TEST(Phase2Integration, MultipleMessagesInBuffer) {
    Buffer buf;
    ASSERT_EQ(buf.append("data: first\n\n"
                         "data: second\n\n"), SseError::success);
    
    // Parse first message
    Message msg1;
    ASSERT_TRUE(parse_message(buf, msg1));
    EXPECT_EQ(msg1.data, "first");
    
    // Parse second message
    Message msg2;
    ASSERT_TRUE(parse_message(buf, msg2));
    EXPECT_EQ(msg2.data, "second");
    
    // No more messages
    EXPECT_FALSE(buf.has_complete_message());
}

TEST(Phase2Integration, NetworkFragmentation) {
    Buffer buf;
    Message msg;
    
    // Simulate fragmented network packet 1
    ASSERT_EQ(buf.append("ev"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    // Packet 2
    ASSERT_EQ(buf.append("ent: test\nda"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    // Packet 3
    ASSERT_EQ(buf.append("ta: hello\n\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    // Parse
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.event, "test");
    EXPECT_EQ(msg.data, "hello");
}

// =============================================================================
// Integration Test Suite: Real-World Scenarios
// =============================================================================

TEST(Phase2Integration, RealWorld_Heartbeat) {
    Buffer buf;
    // Common pattern: :heartbeat comment
    ASSERT_EQ(buf.append(":heartbeat\n"
                         "data: pong\n"
                         "\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.data, "pong");
}

TEST(Phase2Integration, RealWorld_EventTypes) {
    Buffer buf;
    ASSERT_EQ(buf.append("event: user_join\n"
                         "data: {\"user\": \"alice\"}\n"
                         "id: 123\n"
                         "\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.event, "user_join");
    EXPECT_EQ(msg.data, "{\"user\": \"alice\"}");
    EXPECT_EQ(msg.id.value(), "123");
}

TEST(Phase2Integration, RealWorld_RetryConfiguration) {
    Buffer buf;
    ASSERT_EQ(buf.append("retry: 10000\n"
                         "event: reconnect\n"
                         "data: please reconnect with new retry\n"
                         "\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.retry.value(), 10000);
    EXPECT_EQ(msg.event, "reconnect");
}

// =============================================================================
// Integration Test Suite: Edge Cases
// =============================================================================

TEST(Phase2Integration, EmptyMessage) {
    Buffer buf;
    // Just double newline = empty message
    ASSERT_EQ(buf.append("\n\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    // Should have default event and empty data
    EXPECT_EQ(msg.event, "message");
    EXPECT_TRUE(msg.data.empty());
}

TEST(Phase2Integration, FieldWithColonInValue) {
    Buffer buf;
    ASSERT_EQ(buf.append("data: time: 10:30:00\n\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.data, "time: 10:30:00");
}

TEST(Phase2Integration, UnknownFieldsIgnored) {
    Buffer buf;
    ASSERT_EQ(buf.append("custom: value\n"
                         "data: realdata\n"
                         "another: ignored\n"
                         "\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.data, "realdata");
    // custom and another should be ignored without error
}

TEST(Phase2Integration, FieldWithoutValue) {
    Buffer buf;
    ASSERT_EQ(buf.append("event\n\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.event, "");
}

TEST(Phase2Integration, FieldWithEmptyValue) {
    Buffer buf;
    ASSERT_EQ(buf.append("event:\n\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.event, "");
}

TEST(Phase2Integration, MultipleComments) {
    Buffer buf;
    ASSERT_EQ(buf.append(": comment 1\n"
                         ": comment 2\n"
                         ":\n"
                         "data: test\n"
                         ": comment 3\n"
                         "\n"), SseError::success);
    
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.data, "test");
}

TEST(Phase2Integration, BufferCompactionDuringParse) {
    Buffer buf;
    // First, fill buffer partially
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(buf.append("data: x\n"), SseError::success);
    }
    
    // Read some lines to advance read position
    for (int i = 0; i < 5; ++i) {
        buf.read_line();
    }
    
    // Add message boundary
    ASSERT_EQ(buf.append("\n"), SseError::success);
    
    // Buffer should handle compaction internally
    Message msg;
    ASSERT_TRUE(parse_message(buf, msg));
    EXPECT_EQ(msg.data, "x");
}

// Error handling integration
TEST(Phase2Integration, ErrorRecovery_InvalidRetry) {
    Buffer buf;
    // Message with invalid retry followed by valid message
    ASSERT_EQ(buf.append("retry: bad\n\n"
                         "data: valid\n\n"), SseError::success);
    
    Message msg;
    // First message has error
    bool result1 = parse_message(buf, msg);
    EXPECT_FALSE(result1);
    
    // Buffer should still have the second message
    ASSERT_TRUE(buf.has_complete_message());
    
    // Parse second message
    Message msg2;
    ASSERT_TRUE(parse_message(buf, msg2));
    EXPECT_EQ(msg2.data, "valid");
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
    tests/test_line_endings.cpp
)</search>
  <replace>
set(TEST_SOURCES
    tests/test_buffer.cpp
    tests/test_error_codes.cpp
    tests/test_message.cpp
    tests/test_field_parser.cpp
    tests/test_line_endings.cpp
    tests/test_integration_p2.cpp
)</replace>
</action>

### Task 3: Update main header to include field_parser

<action type="read_file">
  <file_path>include/sse_parser.h</file_path>
</action>

<action type="edit_file">
  <file_path>include/sse_parser.h</file_path>
  <search>
// Core components
#include "sse_parser/error_codes.h"
#include "sse_parser/message.h"
#include "sse_parser/buffer.h"</search>
  <replace>
// Core components
#include "sse_parser/error_codes.h"
#include "sse_parser/message.h"
#include "sse_parser/buffer.h"
#include "sse_parser/field_parser.h"</replace>
</action>

## Verification

<action type="bash">
  <command>cd build && cmake .. && cmake --build . --target run_tests 2>&1 | tail -20</command>
</action>

<action type="bash">
  <command>cd build && ctest -R Phase2Integration --output-on-failure</command>
</action>

## Acceptance Criteria

<acceptance_criteria>
1. File created: `tests/test_integration_p2.cpp` exists
2. Test file includes Buffer + field_parser integration
3. Test groups present (grep verify):
   - `grep "CompleteDataMessage" tests/test_integration_p2.cpp` (3+ LF/CRLF/CR tests) ✓
   - `grep "MultiFieldMessage" tests/test_integration_p2.cpp` ✓
   - `grep "MixedLineEndings" tests/test_integration_p2.cpp` ✓
   - `grep "CommentLinesSkipped" tests/test_integration_p2.cpp` ✓
   - `grep "RetryValidationError\|RetryOverflowError" tests/test_integration_p2.cpp` ✓
   - `grep "ChunkedDataArrival\|NetworkFragmentation" tests/test_integration_p2.cpp` ✓
   - `grep "RealWorld" tests/test_integration_p2.cpp` (3+ scenario tests) ✓
4. End-to-end flow verified: Buffer → read_line → parse_field_line → Message
5. Error propagation tested: retry validation errors stop message parsing
6. Total integration tests: `grep -c "TEST(Phase2Integration" tests/test_integration_p2.cpp` ≥ 20
7. All Phase 2 tests pass together
8. Main header updated to include field_parser.h
</acceptance_criteria>

## Phase 2 Success Criteria Verification

<acceptance_criteria>
From ROADMAP.md Phase 2 success criteria:
1. ✓ Correctly parse single-field messages (tests/Phase2Integration/CompleteDataMessage_*)
2. ✓ Pass line ending test matrix (9 combinations) (tests/test_line_endings.cpp)
3. ✓ Handle leading spaces correctly (tests/test_field_parser.cpp LeadingSpace tests)
4. ✓ Skip comment lines (tests/test_field_parser.cpp Comment tests)
5. ✓ Return error for invalid retry values (tests/test_field_parser.cpp RetryValidation tests)

Coverage verification:
- `grep -c "TEST.*LineEndings" tests/test_line_endings.cpp` should be ≥ 15
- `grep -c "TEST.*FieldParser" tests/test_field_parser.cpp` should be ≥ 30
- `grep -c "TEST.*Phase2Integration" tests/test_integration_p2.cpp` should be ≥ 20
</acceptance_criteria>

## Traceability

| Req ID | Coverage |
|--------|----------|
| PAR-01 | Full - field parsing integrated with Buffer |
| PAR-02 | Full - line endings in complete message flow |
| PAR-03 | Full - leading space in end-to-end scenarios |
| PAR-04 | Full - comments in real message streams |
| VAL-01 | Full - retry validation with error propagation |
