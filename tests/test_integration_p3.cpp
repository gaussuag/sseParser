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
    // Note: "\n\n" produces two empty lines from Buffer = two empty messages
    auto messages = parse_buffer_content("\n\n");
    
    ASSERT_EQ(messages.size(), 2);
    EXPECT_TRUE(messages[0].empty());
    EXPECT_EQ(messages[0].data, "");
    EXPECT_EQ(messages[0].event, "message");
    EXPECT_TRUE(messages[1].empty());
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
    // Note: "\n\n" produces two empty lines from Buffer = two empty messages
    auto messages = parse_buffer_content("\n\ndata: hello\n\n");
    
    ASSERT_EQ(messages.size(), 3);
    EXPECT_TRUE(messages[0].empty());
    EXPECT_TRUE(messages[1].empty());
    EXPECT_EQ(messages[2].data, "hello");
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
    // Note: "\r\n\r\n" produces two empty lines from Buffer = two empty messages
    auto messages = parse_buffer_content("\r\n\r\n");
    
    ASSERT_EQ(messages.size(), 2);
    EXPECT_TRUE(messages[0].empty());
    EXPECT_TRUE(messages[1].empty());
}

// Test Count: 21 tests covering DAT-01, DAT-02, EXT-02, and edge cases
