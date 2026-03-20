#include <gtest/gtest.h>
#include "sse_parser/sse_parser.hpp"

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
    // Message with invalid retry - parse should detect error
    ASSERT_EQ(buf.append("retry: bad\n\n"), SseError::success);
    
    Message msg;
    // parse_message should detect error and return false
    bool result = parse_message(buf, msg);
    EXPECT_FALSE(result);
    // Note: retry should not be set due to validation error
    EXPECT_FALSE(msg.retry.has_value());
}
