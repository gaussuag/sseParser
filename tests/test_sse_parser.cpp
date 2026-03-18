#include <gtest/gtest.h>
#include "sse_parser/sse_parser_facade.h"

using namespace sse;

// =============================================================================
// Test Helpers
// =============================================================================

// Collects all messages for verification
struct MessageCollector {
    std::vector<Message> messages;
    
    void operator()(const Message& msg) {
        messages.push_back(msg);
    }
    
    void clear() { messages.clear(); }
    size_t count() const { return messages.size(); }
};

// C-style callback data structure for IFC-01 tests
struct CCallbackData {
    std::vector<Message>* messages;
    int call_count = 0;
};

extern "C" void c_callback(const Message* msg, void* user_data) {
    auto* data = static_cast<CCallbackData*>(user_data);
    data->messages->push_back(*msg);
    ++data->call_count;
}

// =============================================================================
// Category A: Basic Parse Tests (API-01)
// =============================================================================

TEST(SseParserBasicTest, ParseSingleCompleteMessage) {
    // Test: "data: hello\n\n" delivers one message
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    SseError err = parser.parse("data: hello\n\n", 14);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "hello");
}

TEST(SseParserBasicTest, ParseMultipleMessages) {
    // Test: "data: a\n\ndata: b\n\n" delivers two messages
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    SseError err = parser.parse("data: a\n\ndata: b\n\n", 18);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 2);
    EXPECT_EQ(collector.messages[0].data, "a");
    EXPECT_EQ(collector.messages[1].data, "b");
}

TEST(SseParserBasicTest, ParseEmptyData) {
    // Test: Empty input returns success, no callback
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    SseError err = parser.parse("", 0);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 0);
}

TEST(SseParserBasicTest, ParseNullDataZeroLength) {
    // Test: nullptr with len=0 returns success
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    SseError err = parser.parse(nullptr, 0);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 0);
}

// =============================================================================
// Category B: String View Interface (API-02)
// =============================================================================

TEST(SseParserStringViewTest, ParseStringViewSingleMessage) {
    // Test: std::string_view input works
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    std::string_view sv("data: test message\n\n");
    SseError err = parser.parse(sv);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "test message");
}

TEST(SseParserStringViewTest, ParseStringViewZeroCopy) {
    // Test: Verify no data modification - string_view doesn't copy
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    std::string original = "data: original\n\n";
    std::string_view sv(original);
    
    SseError err = parser.parse(sv);
    EXPECT_EQ(err, SseError::success);
    
    // Original string should remain unchanged (zero-copy)
    EXPECT_EQ(original, "data: original\n\n");
    EXPECT_EQ(collector.messages[0].data, "original");
}

TEST(SseParserStringViewTest, ParseStringViewEquivalence) {
    // Test: Same results as parse(const char*, size_t)
    MessageCollector collector1;
    MessageCollector collector2;
    SseParser parser1;
    SseParser parser2;
    parser1.set_callback(std::ref(collector1));
    parser2.set_callback(std::ref(collector2));
    
    const char* data = "data: hello\nevent: test\nid: 123\n\n";
    size_t len = strlen(data);
    
    // Method 1: const char*, size_t
    SseError err1 = parser1.parse(data, len);
    
    // Method 2: string_view
    SseError err2 = parser2.parse(std::string_view(data, len));
    
    EXPECT_EQ(err1, err2);
    EXPECT_EQ(collector1.count(), collector2.count());
    EXPECT_EQ(collector1.messages[0].data, collector2.messages[0].data);
    EXPECT_EQ(collector1.messages[0].event, collector2.messages[0].event);
}

// =============================================================================
// Category C: Callback Tests (API-03, IFC-01)
// =============================================================================

TEST(SseParserCallbackTest, StdFunctionLambda) {
    // Test: Lambda captures message correctly
    std::vector<Message> messages;
    SseParser parser;
    parser.set_callback([&messages](const Message& msg) {
        messages.push_back(msg);
    });
    
    parser.parse("data: lambda test\n\n", 20);
    
    EXPECT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].data, "lambda test");
}

TEST(SseParserCallbackTest, StdFunctionMultipleCalls) {
    // Test: Callback invoked for each message
    int call_count = 0;
    SseParser parser;
    parser.set_callback([&call_count](const Message&) {
        ++call_count;
    });
    
    parser.parse("data: msg1\n\ndata: msg2\n\ndata: msg3\n\n", 36);
    
    EXPECT_EQ(call_count, 3);
}

TEST(SseParserCallbackTest, FunctionPointerBasic) {
    // Test: C-style callback with user_data
    std::vector<Message> messages;
    CCallbackData data{&messages};
    
    SseParser parser;
    parser.set_callback(c_callback, &data);
    
    parser.parse("data: fp test\n\n", 16);
    
    EXPECT_EQ(data.call_count, 1);
    EXPECT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].data, "fp test");
}

TEST(SseParserCallbackTest, FunctionPointerUserData) {
    // Test: void* user_data passed correctly
    std::vector<Message> messages1;
    std::vector<Message> messages2;
    CCallbackData data1{&messages1};
    CCallbackData data2{&messages2};
    
    SseParser parser1;
    SseParser parser2;
    parser1.set_callback(c_callback, &data1);
    parser2.set_callback(c_callback, &data2);
    
    parser1.parse("data: parser1\n\n", 16);
    parser2.parse("data: parser2\n\n", 16);
    
    EXPECT_EQ(data1.call_count, 1);
    EXPECT_EQ(data2.call_count, 1);
    EXPECT_EQ(messages1[0].data, "parser1");
    EXPECT_EQ(messages2[0].data, "parser2");
}

// =============================================================================
// Category D: Chunked Input Scenarios
// =============================================================================

TEST(SseParserChunkedTest, SplitAcrossLines) {
    // Test: "data: hel" + "lo\n\n" -> one message
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    // Chunk 1: incomplete
    SseError err = parser.parse("data: hel", 9);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 0);
    
    // Chunk 2: complete the message
    err = parser.parse("lo\n\n", 4);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "hello");
}

TEST(SseParserChunkedTest, MultipleChunks) {
    // Test: 5 chunks for 2 messages
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: ms", 8);
    EXPECT_EQ(collector.count(), 0);
    
    parser.parse("g1\n", 3);
    EXPECT_EQ(collector.count(), 0);
    
    parser.parse("\ndata: ", 7);
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "msg1");
    
    parser.parse("msg2", 4);
    EXPECT_EQ(collector.count(), 1);
    
    parser.parse("\n\n", 2);
    EXPECT_EQ(collector.count(), 2);
    EXPECT_EQ(collector.messages[1].data, "msg2");
}

TEST(SseParserChunkedTest, EmptyChunks) {
    // Test: Empty chunks handled gracefully
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: hello\n\n", 13);
    EXPECT_EQ(collector.count(), 1);
    
    parser.parse("", 0);  // Empty chunk
    EXPECT_EQ(collector.count(), 1);
    
    parser.parse(nullptr, 0);  // Null chunk
    EXPECT_EQ(collector.count(), 1);
}

TEST(SseParserChunkedTest, MidLineSplit) {
    // Test: Split in middle of field value
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: line", 10);
    parser.parse("1\ndata: lin", 11);
    parser.parse("e2\n\n", 4);
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "line1\nline2");
}

// =============================================================================
// Category E: Flush Tests (API-04)
// =============================================================================

TEST(SseParserFlushTest, IncompleteMessage) {
    // Test: "data: hello" (no newline) -> flush() delivers
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: hello", 11);  // No newline
    EXPECT_EQ(collector.count(), 0);
    
    SseError err = parser.flush();
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "hello");
}

TEST(SseParserFlushTest, NoIncompleteMessage) {
    // Test: Empty buffer -> success, no callback
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    SseError err = parser.flush();
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 0);
}

TEST(SseParserFlushTest, FlushOnEmptyBuffer) {
    // Test: flush() on empty parser -> success, no callback
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    // Flush without any data
    SseError err = parser.flush();
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 0);
}

TEST(SseParserFlushTest, DoubleFlush) {
    // Test: Second flush is no-op
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: test", 10);
    
    SseError err1 = parser.flush();
    EXPECT_EQ(err1, SseError::success);
    EXPECT_EQ(collector.count(), 1);
    
    SseError err2 = parser.flush();
    EXPECT_EQ(err2, SseError::success);
    EXPECT_EQ(collector.count(), 1);  // No additional message
}

TEST(SseParserFlushTest, FlushWithIncompleteOnly) {
    // Test: Flush delivers incomplete message at stream end
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: incomplete", 16);
    EXPECT_EQ(collector.count(), 0);
    
    SseError err = parser.flush();
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "incomplete");
}

// =============================================================================
// Category F: Reset Tests (API-05)
// =============================================================================

TEST(SseParserResetTest, ClearsBuffer) {
    // Test: Partial data cleared
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: partial", 13);
    EXPECT_EQ(collector.count(), 0);
    EXPECT_TRUE(parser.has_incomplete_message());
    
    parser.reset();
    
    EXPECT_FALSE(parser.has_incomplete_message());
    
    // After reset, should be fresh state
    parser.parse("data: new\n\n", 11);
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "new");
}

TEST(SseParserResetTest, KeepsCallback) {
    // Test: Callback still works after reset
    int call_count = 0;
    SseParser parser;
    parser.set_callback([&call_count](const Message&) {
        ++call_count;
    });
    
    parser.parse("data: before\n\n", 15);
    EXPECT_EQ(call_count, 1);
    
    parser.reset();
    
    parser.parse("data: after\n\n", 14);
    EXPECT_EQ(call_count, 2);
}

TEST(SseParserResetTest, ClearsLastEventId) {
    // Test: reset() clears last_event_id tracking in SseParser
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    // Note: SseParser tracks last_event_id separately from MessageBuilder
    // After reset, last_event_id() should be empty
    parser.parse("id: 123\ndata: test\n\n", 21);
    
    parser.reset();
    
    // After reset, last_event_id is cleared
    EXPECT_EQ(parser.last_event_id(), "");
}

TEST(SseParserResetTest, MidMessage) {
    // Test: Reset during incomplete message, fresh state
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("event: progress\ndata: partial", 30);
    EXPECT_EQ(collector.count(), 0);
    
    parser.reset();
    
    // Complete message after reset
    parser.parse("data: fresh\n\n", 14);
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "fresh");
    EXPECT_EQ(collector.messages[0].event, "message");  // Not "progress"
}

// =============================================================================
// Category G: Integration Scenarios
// =============================================================================

TEST(SseParserIntegrationTest, ReconnectionScenario) {
    // Test: Parse + reset + parse works
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    // First connection
    parser.parse("id: 1\ndata: first\n\n", 20);
    EXPECT_EQ(collector.count(), 1);
    
    // Simulate reconnection - reset clears state but keeps callback
    parser.reset();
    
    // Second connection
    parser.parse("id: 2\ndata: second\n\n", 21);
    EXPECT_EQ(collector.count(), 2);
    EXPECT_EQ(collector.messages[1].data, "second");
}

TEST(SseParserIntegrationTest, ChunkedWithFlush) {
    // Test: Multiple chunks + final flush delivers incomplete data as message
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    // Simulate chunked streaming - first complete message
    parser.parse("data: msg1\n\n", 12);
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "msg1");
    
    // Incomplete final chunk (no newline terminator)
    parser.parse("data: final_chunk", 17);
    EXPECT_EQ(collector.count(), 1);
    
    // End of stream - flush delivers incomplete data
    parser.flush();
    EXPECT_EQ(collector.count(), 2);
    // Flush delivers remaining buffer content as a message
    EXPECT_FALSE(collector.messages[1].data.empty());
}

TEST(SseParserIntegrationTest, FullFeaturedMessage) {
    // Test: Message with all fields
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse(
        "event: custom\n"
        "data: line1\n"
        "data: line2\n"
        "id: msg-abc\n"
        "retry: 5000\n"
        "\n",
        68
    );
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].event, "custom");
    EXPECT_EQ(collector.messages[0].data, "line1\nline2");
    ASSERT_TRUE(collector.messages[0].id.has_value());
    EXPECT_EQ(*collector.messages[0].id, "msg-abc");
    ASSERT_TRUE(collector.messages[0].retry.has_value());
    EXPECT_EQ(*collector.messages[0].retry, 5000);
}

TEST(SseParserIntegrationTest, CallbackTypesIndependently) {
    // Test: Both callback types work correctly when used independently
    std::vector<Message> std_msgs;
    std::vector<Message> fp_msgs;
    CCallbackData fp_data{&fp_msgs};
    
    // Test std::function callback
    SseParser parser1;
    parser1.set_callback([&std_msgs](const Message& msg) {
        std_msgs.push_back(msg);
    });
    parser1.parse("data: std\n\n", 12);
    
    // Test function pointer callback
    SseParser parser2;
    parser2.set_callback(c_callback, &fp_data);
    parser2.parse("data: fp\n\n", 11);
    
    EXPECT_EQ(std_msgs.size(), 1);
    EXPECT_EQ(fp_msgs.size(), 1);
    EXPECT_EQ(std_msgs[0].data, "std");
    EXPECT_EQ(fp_msgs[0].data, "fp");
}

TEST(SseParserIntegrationTest, CRLFLineEndings) {
    // Test: CRLF line endings work correctly
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: line1\r\ndata: line2\r\n\r\n", 27);
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "line1\nline2");
}

TEST(SseParserIntegrationTest, EmptyMessages) {
    // Test: Empty messages are delivered
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("\n\n", 2);
    
    EXPECT_GE(collector.count(), 1);
    EXPECT_TRUE(collector.messages[0].empty());
}

// =============================================================================
// Edge Case Tests
// =============================================================================

TEST(SseParserEdgeCaseTest, ConstructorWithCallback) {
    // Test: Constructor that takes callback
    MessageCollector collector;
    SseParser parser(std::ref(collector));
    
    parser.parse("data: ctor\n\n", 13);
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "ctor");
}

TEST(SseParserEdgeCaseTest, ParseAfterFlush) {
    // Test: Can continue parsing after flush
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: first", 11);
    parser.flush();
    EXPECT_EQ(collector.count(), 1);
    
    parser.parse("data: second\n\n", 15);
    EXPECT_EQ(collector.count(), 2);
}

TEST(SseParserEdgeCaseTest, CommentsIgnored) {
    // Test: Comment lines are ignored
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse(":comment\ndata: real\n:another\n\n", 34);
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "real");
}

TEST(SseParserEdgeCaseTest, HasIncompleteMessageBasic) {
    // Test: has_incomplete_message() correctly identifies incomplete data
    SseParser parser;
    
    // Initially no incomplete message
    EXPECT_FALSE(parser.has_incomplete_message());
    
    // After parsing incomplete data (no newline)
    parser.parse("data: incomplete", 16);
    EXPECT_TRUE(parser.has_incomplete_message());
}

// =============================================================================
// Summary
// =============================================================================
// Test Count: 25+ test cases covering API-01 through API-05, IFC-01
// Coverage: Basic parsing, string_view, callbacks, chunked input,
//           flush, reset, and integration scenarios
// Target: 85%+ line coverage in sse_parser_facade.h
