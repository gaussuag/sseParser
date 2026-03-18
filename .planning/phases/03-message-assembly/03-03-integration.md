---
plan: 03-03
phase: 3
wave: 3
autonomous: false
objective: Create comprehensive integration tests for Phase 3: end-to-end Buffer → field_parser → MessageBuilder flow
req_ids: DAT-01, DAT-02, EXT-02
---

# Plan 03-03: Phase 3 Integration Tests

## Context

This plan creates integration tests that verify the complete Phase 3 pipeline: Buffer data accumulation → line parsing → field parsing → message assembly. Tests cover multi-line data, empty messages, and Last-Event-ID tracking.

**Target Coverage:** 90% line coverage for Phase 3 code

**Test Categories:**
1. **DAT-01 Tests**: Multi-line data accumulation scenarios
2. **DAT-02 Tests**: Empty message handling
3. **EXT-02 Tests**: Last-Event-ID tracking
4. **Edge Cases**: Mixed scenarios, error handling, buffer boundaries

**Dependencies:**
- `include/sse_parser/buffer.h` - Buffer for data accumulation
- `include/sse_parser/field_parser.h` - Field parsing
- `include/sse_parser/message_builder.h` - Message assembly
- Google Test framework

## Read First (MANDATORY)

<read_first>
  <file>include/sse_parser/buffer.h</file>
  <file>include/sse_parser/field_parser.h</file>
  <file>include/sse_parser/message_builder.h</file>
  <file>include/sse_parser/message.h</file>
  <file>tests/test_integration_p2.cpp</file>
</read_first>

## Test Matrix

### DAT-01: Multi-line Data Tests

| Test Case | Input | Expected Result |
|-----------|-------|-----------------|
| single_data | `data: hello\n\n` | data="hello" |
| two_lines | `data: line1\ndata: line2\n\n` | data="line1\nline2" |
| three_lines | `data: a\ndata: b\ndata: c\n\n` | data="a\nb\nc" |
| empty_data_line | `data: \ndata: \n\n` | data="\n" |
| data_with_empty | `data: hello\ndata: \ndata: world\n\n` | data="hello\n\nworld" |

### DAT-02: Empty Message Tests

| Test Case | Input | Expected Result |
|-----------|-------|-----------------|
| pure_empty | `\n\n` | Message with empty data |
| event_only | `event: test\n\n` | event="test", data="" |
| data_empty_value | `data: \n\n` | data="" |
| empty_then_data | `\n\ndata: hello\n\n` | First: empty msg, Second: data="hello" |

### EXT-02: Last-Event-ID Tests

| Test Case | Input | Expected ID |
|-----------|-------|-------------|
| id_tracked | `id: 1\ndata: x\n\n` | "1" |
| id_updates | `id: 1\ndata: x\n\nid: 2\ndata: y\n\n` | "2" |
| id_persists | `id: 1\ndata: x\n\ndata: y\n\n` | "1" |
| id_empty | `id: \ndata: x\n\n` | "" |
| id_cleared | (after above, clear_last_event_id()) | "" |

### Edge Cases

| Test Case | Input | Expected Result |
|-----------|-------|-----------------|
| full_message | `event: msg\ndata: body\nid: 1\nretry: 5000\n\n` | All fields set |
| comment_ignored | `:comment\ndata: hello\n\n` | data="hello", no event |
| retry_validation | `retry: invalid\ndata: x\n\n` | Error returned |
| bom_at_start | `\xEF\xBB\xBFdata: hello\n\n` | data="hello" (BOM skipped) |

## Acceptance Criteria

<acceptance_criteria>
  <criterion>File exists: tests/test_integration_p3.cpp</criterion>
  <criterion>grep -c "TEST(" tests/test_integration_p3.cpp | grep -E "[1-9][0-9]*"</criterion>
  <criterion>grep -n "DAT-01\|DAT-02\|EXT-02" tests/test_integration_p3.cpp</criterion>
  <criterion>grep -n "message_builder.h" tests/test_integration_p3.cpp</criterion>
  <criterion>grep -n "MessageBuilder" tests/test_integration_p3.cpp</criterion>
  <criterion>grep -n "last_event_id" tests/test_integration_p3.cpp</criterion>
</acceptance_criteria>

## Implementation Actions

<action>
  <type>create</type>
  <file>tests/test_integration_p3.cpp</file>
  <description>Integration tests for Phase 3 Message Assembly</description>
  <template>
#include <gtest/gtest.h>
#include "sse_parser/buffer.h"
#include "sse_parser/message_builder.h"
#include "sse_parser/field_parser.h"

using namespace sse;

// Helper function to feed buffer content to MessageBuilder
std::vector<Message> parse_buffer_content(const std::string& content) {
    Buffer buffer;
    MessageBuilder builder;
    std::vector<Message> messages;

    builder.set_callback([&messages](const Message& msg) {
        messages.push_back(msg);
    });

    SseError err = buffer.append(content);
    EXPECT_EQ(err, SseError::success);

    // Process all lines
    while (auto line = buffer.read_line()) {
        SseError line_err = builder.feed_line(*line);
        EXPECT_EQ(line_err, SseError::success);
    }

    return messages;
}

// DAT-01: Multi-line Data Accumulation Tests
// ===========================================

TEST(MessageBuilderDat01, SingleDataLine) {
    // Requirement: DAT-01 - Single data line
    auto messages = parse_buffer_content("data: hello\n\n");
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].data, "hello");
    EXPECT_EQ(messages[0].event, "message");
    EXPECT_FALSE(messages[0].id.has_value());
}

TEST(MessageBuilderDat01, TwoDataLines) {
    // Requirement: DAT-01 - Two lines with newline separator
    auto messages = parse_buffer_content("data: line1\ndata: line2\n\n");
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].data, "line1\nline2");
}

TEST(MessageBuilderDat01, ThreeDataLines) {
    // Requirement: DAT-01 - Three lines
    auto messages = parse_buffer_content("data: a\ndata: b\ndata: c\n\n");
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].data, "a\nb\nc");
}

TEST(MessageBuilderDat01, EmptyDataLines) {
    // Requirement: DAT-01 - Empty data lines add newline
    auto messages = parse_buffer_content("data: \ndata: \n\n");
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].data, "\n");
}

TEST(MessageBuilderDat01, MixedDataWithEmpty) {
    // Requirement: DAT-01 - Mix of content and empty lines
    auto messages = parse_buffer_content("data: hello\ndata: \ndata: world\n\n");
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].data, "hello\n\nworld");
}

TEST(MessageBuilderDat01, NoTrailingNewline) {
    // Requirement: DAT-01 - No trailing newline after last line
    auto messages = parse_buffer_content("data: hello\ndata: world\n\n");
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].data, "hello\nworld");
}

// DAT-02: Empty Message Handling Tests
// =====================================

TEST(MessageBuilderDat02, PureEmptyMessage) {
    // Requirement: DAT-02 - Blank line with no fields = empty message
    auto messages = parse_buffer_content("\n\n");
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_TRUE(messages[0].empty());
    EXPECT_EQ(messages[0].data, "");
    EXPECT_EQ(messages[0].event, "message");
}

TEST(MessageBuilderDat02, EventOnlyNoData) {
    // Requirement: DAT-02 - Event field but no data
    auto messages = parse_buffer_content("event: test\n\n");
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].event, "test");
    EXPECT_TRUE(messages[0].empty());
}

TEST(MessageBuilderDat02, DataEmptyValue) {
    // Requirement: DAT-02 - Data field with empty value
    auto messages = parse_buffer_content("data: \n\n");
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].data, "");
    EXPECT_TRUE(messages[0].empty());
}

TEST(MessageBuilderDat02, EmptyMessageFollowedByData) {
    // Requirement: DAT-02 - Empty message followed by data message
    auto messages = parse_buffer_content("\n\ndata: hello\n\n");
    
    ASSERT_EQ(messages.size(), 2);
    EXPECT_TRUE(messages[0].empty());
    EXPECT_EQ(messages[1].data, "hello");
}

// EXT-02: Last-Event-ID Tracking Tests
// =====================================

TEST(MessageBuilderExt02, IdTracked) {
    // Requirement: EXT-02 - ID from first message
    Buffer buffer;
    MessageBuilder builder;
    
    buffer.append("id: 1\ndata: first\n\n");
    while (auto line = buffer.read_line()) {
        builder.feed_line(*line);
    }
    
    EXPECT_EQ(builder.last_event_id(), "1");
}

TEST(MessageBuilderExt02, IdUpdates) {
    // Requirement: EXT-02 - ID updates with new messages
    Buffer buffer;
    MessageBuilder builder;
    
    buffer.append("id: 1\ndata: first\n\nid: 2\ndata: second\n\n");
    while (auto line = buffer.read_line()) {
        builder.feed_line(*line);
    }
    
    EXPECT_EQ(builder.last_event_id(), "2");
}

TEST(MessageBuilderExt02, IdPersistsWithoutNewId) {
    // Requirement: EXT-02 - ID persists across messages without ID
    Buffer buffer;
    MessageBuilder builder;
    
    buffer.append("id: abc\ndata: first\n\ndata: no_id\n\n");
    while (auto line = buffer.read_line()) {
        builder.feed_line(*line);
    }
    
    EXPECT_EQ(builder.last_event_id(), "abc");
}

TEST(MessageBuilderExt02, IdEmptyClears) {
    // Requirement: EXT-02 - Empty ID clears tracking
    Buffer buffer;
    MessageBuilder builder;
    
    buffer.append("id: 1\ndata: first\n\nid: \ndata: empty_id\n\n");
    while (auto line = buffer.read_line()) {
        builder.feed_line(*line);
    }
    
    EXPECT_EQ(builder.last_event_id(), "");
}

TEST(MessageBuilderExt02, ClearLastEventId) {
    // Requirement: EXT-02 - Manual clear of ID
    Buffer buffer;
    MessageBuilder builder;
    
    buffer.append("id: 123\ndata: x\n\n");
    while (auto line = buffer.read_line()) {
        builder.feed_line(*line);
    }
    
    EXPECT_EQ(builder.last_event_id(), "123");
    builder.clear_last_event_id();
    EXPECT_EQ(builder.last_event_id(), "");
}

TEST(MessageBuilderExt02, IdSurvivesReset) {
    // Requirement: EXT-02 - ID tracking survives reset()
    Buffer buffer;
    MessageBuilder builder;
    
    buffer.append("id: persistent\ndata: x\n\n");
    while (auto line = buffer.read_line()) {
        builder.feed_line(*line);
    }
    
    builder.reset();
    EXPECT_EQ(builder.last_event_id(), "persistent");
}

// Edge Cases and Integration Tests
// ==================================

TEST(MessageBuilderIntegration, FullMessageAllFields) {
    // All fields present in single message
    auto messages = parse_buffer_content(
        "event: custom\n"
        "data: body content\n"
        "id: msg-123\n"
        "retry: 5000\n\n"
    );
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].event, "custom");
    EXPECT_EQ(messages[0].data, "body content");
    ASSERT_TRUE(messages[0].id.has_value());
    EXPECT_EQ(*messages[0].id, "msg-123");
    ASSERT_TRUE(messages[0].retry.has_value());
    EXPECT_EQ(*messages[0].retry, 5000);
}

TEST(MessageBuilderIntegration, CommentLinesIgnored) {
    // Comments should not affect message assembly
    auto messages = parse_buffer_content(
        ":This is a comment\n"
        "data: hello\n"
        ":Another comment\n\n"
    );
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].data, "hello");
}

TEST(MessageBuilderIntegration, MultipleMessagesSequential) {
    // Multiple complete messages
    auto messages = parse_buffer_content(
        "data: msg1\n\n"
        "data: msg2\n\n"
        "data: msg3\n\n"
    );
    
    ASSERT_EQ(messages.size(), 3);
    EXPECT_EQ(messages[0].data, "msg1");
    EXPECT_EQ(messages[1].data, "msg2");
    EXPECT_EQ(messages[2].data, "msg3");
}

TEST(MessageBuilderIntegration, CallbackAndQueueWorkTogether) {
    // Both callback and get_message() should work
    Buffer buffer;
    MessageBuilder builder;
    int callback_count = 0;
    
    builder.set_callback([&callback_count](const Message&) {
        callback_count++;
    });
    
    buffer.append("data: test\n\n");
    while (auto line = buffer.read_line()) {
        builder.feed_line(*line);
    }
    
    EXPECT_EQ(callback_count, 1);
    EXPECT_TRUE(builder.has_message());
    
    auto msg = builder.get_message();
    EXPECT_TRUE(msg.has_value());
    EXPECT_FALSE(builder.has_message());
}

TEST(MessageBuilderIntegration, GetMessageReturnsNulloptWhenEmpty) {
    MessageBuilder builder;
    
    auto msg = builder.get_message();
    EXPECT_FALSE(msg.has_value());
}

TEST(MessageBuilderIntegration, CurrentMessageAccess) {
    // Access to incomplete message state
    MessageBuilder builder;
    
    builder.feed_line("event: progress");
    
    EXPECT_EQ(builder.current_message().event, "progress");
    EXPECT_FALSE(builder.has_message());
}

TEST(MessageBuilderIntegration, ResetClearsCurrentMessage) {
    MessageBuilder builder;
    
    builder.feed_line("data: partial");
    EXPECT_EQ(builder.current_message().data, "partial");
    
    builder.reset();
    EXPECT_EQ(builder.current_message().data, "");
}

// CRLF Line Ending Tests
// ======================

TEST(MessageBuilderCRLF, MultiLineWithCRLF) {
    auto messages = parse_buffer_content("data: line1\r\ndata: line2\r\n\r\n");
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].data, "line1\nline2");
}

TEST(MessageBuilderCRLF, EmptyMessageWithCRLF) {
    auto messages = parse_buffer_content("\r\n\r\n");
    
    ASSERT_EQ(messages.size(), 1);
    EXPECT_TRUE(messages[0].empty());
}

// Test Count: 21 tests covering DAT-01, DAT-02, EXT-02, and edge cases
  </template>
</action>

## Verification Checklist

- [ ] All 21 tests compile and pass
- [ ] 90% line coverage for Phase 3 code
- [ ] DAT-01 tests verify multi-line data with `\n` separator
- [ ] DAT-02 tests verify empty messages
- [ ] EXT-02 tests verify Last-Event-ID tracking
- [ ] Edge case tests cover mixed scenarios
- [ ] Tests use Buffer → MessageBuilder pipeline

## Dependencies

- Requires Plan 03-01: MessageBuilder core
- Requires Plan 03-02: Last-Event-ID tracking
- Requires Google Test framework

## Traceability

| Requirement | Test Coverage |
|-------------|--------------|
| DAT-01 | MultiLine tests (6 tests) |
| DAT-02 | EmptyMessage tests (4 tests) |
| EXT-02 | LastEventId tests (6 tests) |
| Integration | Edge cases and pipeline tests (5 tests) |
