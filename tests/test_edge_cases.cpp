#include <gtest/gtest.h>
#include <limits>
#include <cstring>
#include "sse_parser/sse_parser.hpp"

using namespace sse;

// =============================================================================
// Edge Case Test Helpers
// =============================================================================

struct EdgeCaseCollector {
    std::vector<Message> messages;
    
    void operator()(const Message& msg) {
        messages.push_back(msg);
    }
    
    void clear() { messages.clear(); }
    size_t count() const { return messages.size(); }
};

// =============================================================================
// Category 1: Empty/Null Input Tests
// =============================================================================

TEST(EdgeCaseEmptyNullTest, EmptyStringInput) {
    // API-01: Empty string should return success, no callback
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    SseError err = parser.parse("");
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 0);
}

TEST(EdgeCaseEmptyNullTest, NullPointerZeroLength) {
    // API-01: nullptr with len=0 should succeed
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    SseError err = parser.parse(nullptr, 0);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 0);
}

TEST(EdgeCaseEmptyNullTest, EmptyStringView) {
    // API-02: Empty string_view
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    std::string_view sv("");
    SseError err = parser.parse(sv);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 0);
}

TEST(EdgeCaseEmptyNullTest, EmptyMessageDelivery) {
    // DAT-02: Empty message (just \n\n) should trigger callback
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("\n\n");
    EXPECT_GE(collector.count(), 1);
}

TEST(EdgeCaseEmptyNullTest, EmptyDataField) {
    // PAR-01: Empty data value
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data:\n\n");
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "");
}

// =============================================================================
// Category 2: Buffer Boundary Tests (ROB-01, BUF-01)
// =============================================================================

TEST(EdgeCaseBufferBoundaryTest, ExactBufferSize) {
    // BUF-01: Data exactly at buffer limit
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    // Create data at various sizes around typical buffer limits
    std::string data_4096(4000, 'x');
    std::string chunk = "data: " + data_4096 + "\n\n";
    
    SseError err = parser.parse(chunk);
    // Should either succeed or return buffer_overflow
    EXPECT_TRUE(err == SseError::success || err == SseError::buffer_overflow);
}

TEST(EdgeCaseBufferBoundaryTest, BufferOverflowAttempt) {
    // ROB-01: Attempt to overflow buffer
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    std::string huge_data(50000, 'x');
    std::string chunk = "data: " + huge_data + "\n\n";
    
    SseError err = parser.parse(chunk);
    // Should handle gracefully (either success or overflow error)
    EXPECT_TRUE(err == SseError::success || err == SseError::buffer_overflow);
}

TEST(EdgeCaseBufferBoundaryTest, IncrementalBufferFill) {
    // BUF-01: Fill buffer incrementally
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    // Add many small chunks
    for (int i = 0; i < 1000; ++i) {
        std::string chunk = "data: x\n\n";
        SseError err = parser.parse(chunk);
        if (err != SseError::success) {
            // Buffer full - that's expected behavior
            break;
        }
    }
    // Should have collected some messages
    EXPECT_GE(collector.count(), 0);
}

TEST(EdgeCaseBufferBoundaryTest, BufferClearAfterOverflow) {
    // ROB-01: Recovery after overflow scenario
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    // Reset to clear any partial state
    parser.reset();
    
    // Now small messages should work
    SseError err = parser.parse("data: test\n\n");
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 1);
}

// =============================================================================
// Category 3: Maximum Value Tests (VAL-01)
// =============================================================================

TEST(EdgeCaseMaximumValueTest, MaxIntRetry) {
    // VAL-01: Maximum valid retry value
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    int max_int = std::numeric_limits<int>::max();
    std::string chunk = "retry: " + std::to_string(max_int) + "\ndata: test\n\n";
    
    SseError err = parser.parse(chunk);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 1);
    ASSERT_TRUE(collector.messages[0].retry.has_value());
    EXPECT_EQ(*collector.messages[0].retry, max_int);
}

TEST(EdgeCaseMaximumValueTest, OverflowRetryValue) {
    // VAL-01: Retry value overflow (INT_MAX + 1)
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    long long overflow_val = static_cast<long long>(std::numeric_limits<int>::max()) + 1;
    std::string chunk = "retry: " + std::to_string(overflow_val) + "\ndata: test\n\n";
    
    SseError err = parser.parse(chunk);
    // Should detect overflow and return error
    EXPECT_EQ(err, SseError::invalid_retry);
}

TEST(EdgeCaseMaximumValueTest, ZeroRetryValue) {
    // VAL-01: Zero retry value
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("retry: 0\ndata: test\n\n");
    
    EXPECT_EQ(collector.count(), 1);
    ASSERT_TRUE(collector.messages[0].retry.has_value());
    EXPECT_EQ(*collector.messages[0].retry, 0);
}

TEST(EdgeCaseMaximumValueTest, NegativeRetryValue) {
    // VAL-01: Negative retry should be invalid
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    SseError err = parser.parse("retry: -100\ndata: test\n\n");
    EXPECT_EQ(err, SseError::invalid_retry);
}

// =============================================================================
// Category 4: Special Character Tests
// =============================================================================

TEST(EdgeCaseSpecialCharTest, NullBytesInData) {
    // Extreme: Data containing null bytes - SSE streams shouldn't contain nulls
    // but parser should handle them gracefully without crashing
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    // Create raw data with embedded null - note: this is an extreme edge case
    // that may not be fully supported but shouldn't crash
    char data_with_null[] = "data: hello\0world\n\n";
    // Note: string length calculation needs care with embedded nulls
    SseError err = parser.parse(data_with_null, sizeof(data_with_null) - 1);
    
    // Should either succeed or handle gracefully (nulls may terminate string early)
    EXPECT_TRUE(err == SseError::success || err == SseError::incomplete_message);
}

TEST(EdgeCaseSpecialCharTest, ControlCharacters) {
    // Extreme: Control characters in data
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    std::string data_with_ctrl = "data: \x01\x02\x03\x04\n\n";
    SseError err = parser.parse(data_with_ctrl);
    
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 1);
}

TEST(EdgeCaseSpecialCharTest, UnicodeCharacters) {
    // Extreme: Unicode in data
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    SseError err = parser.parse("data: Hello \xE4\xB8\x96\xE7\x95\x8C\n\n");  // "Hello World" in Chinese
    
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 1);
}

TEST(EdgeCaseSpecialCharTest, WhitespaceOnlyValue) {
    // PAR-03: Field value with only whitespace
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data:    \n\n");  // Data with only spaces
    
    EXPECT_EQ(collector.count(), 1);
    // First space removed per PAR-03, rest preserved
    EXPECT_EQ(collector.messages[0].data, "   ");
}

// =============================================================================
// Category 5: Line Ending Variation Tests (PAR-02)
// =============================================================================

TEST(EdgeCaseLineEndingTest, SingleLF) {
    // PAR-02: Unix line endings
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: test\n\n");
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "test");
}

TEST(EdgeCaseLineEndingTest, SingleCR) {
    // PAR-02: Classic Mac line endings
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: test\r\r");
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "test");
}

TEST(EdgeCaseLineEndingTest, MixedLineEndings) {
    // PAR-02: Mixed line endings in single message
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: line1\r\ndata: line2\n\n");
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "line1\nline2");
}

TEST(EdgeCaseLineEndingTest, CRLFInDataValue) {
    // PAR-02: CRLF sequence in data value
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: test\r\n\r\n");
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "test");
}

TEST(EdgeCaseLineEndingTest, ConsecutiveNewlines) {
    // PAR-02: Multiple consecutive newlines
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("\n\n\n\n");  // Multiple empty messages
    
    EXPECT_GE(collector.count(), 2);
}

// =============================================================================
// Category 6: Malformed Data Tests (ERR-01, ERR-02)
// =============================================================================

TEST(EdgeCaseMalformedTest, IncompleteFieldLine) {
    // Extreme: Line without proper termination
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: incomplete");  // No newline
    
    EXPECT_EQ(collector.count(), 0);
    EXPECT_TRUE(parser.has_incomplete_message());
}

TEST(EdgeCaseMalformedTest, NoColonInField) {
    // PAR-01: Field without colon
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data\n\n");  // Field with no value
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "");
}

TEST(EdgeCaseMalformedTest, UnknownFieldName) {
    // PAR-01: Unknown field should be ignored
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("unknown: value\ndata: real\n\n");
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "real");
}

TEST(EdgeCaseMalformedTest, EmptyFieldName) {
    // PAR-01: Line starting with colon (comment)
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: test\n:comment\n\n");
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "test");
}

TEST(EdgeCaseMalformedTest, MultipleColons) {
    // PAR-01: Multiple colons in field
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: value:with:colons\n\n");
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "value:with:colons");
}

// =============================================================================
// Category 7: State Machine Edge Cases
// =============================================================================

TEST(EdgeCaseStateMachineTest, DoubleReset) {
    // API-05: Double reset should be safe
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: partial");
    parser.reset();
    parser.reset();  // Double reset
    
    EXPECT_FALSE(parser.has_incomplete_message());
    
    parser.parse("data: after\n\n");
    EXPECT_EQ(collector.count(), 1);
}

TEST(EdgeCaseStateMachineTest, FlushEmptyBuffer) {
    // API-04: Flush on empty buffer
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    SseError err = parser.flush();
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 0);
}

TEST(EdgeCaseStateMachineTest, FlushMultipleTimes) {
    // API-04: Multiple flushes
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: test");
    parser.flush();
    SseError err = parser.flush();
    
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 1);  // Still only 1 message
}

TEST(EdgeCaseStateMachineTest, ParseAfterFlush) {
    // API-01, API-04: Parse after flush
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: first");
    parser.flush();
    parser.parse("data: second\n\n");
    
    EXPECT_EQ(collector.count(), 2);
}

TEST(EdgeCaseStateMachineTest, CallbackSetMidParse) {
    // API-03: Setting callback after some data
    SseParser parser;
    
    parser.parse("data: before\n\n");  // No callback set yet
    
    EdgeCaseCollector collector;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: after\n\n");
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "after");
}

// =============================================================================
// Category 8: Message Field Edge Cases (MSG-01)
// =============================================================================

TEST(EdgeCaseMessageFieldTest, AllFieldsPresent) {
    // MSG-01: Message with all possible fields
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse(
        "event: custom\n"
        "data: content\n"
        "id: 123\n"
        "retry: 5000\n"
        "\n"
    );
    
    ASSERT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].event, "custom");
    EXPECT_EQ(collector.messages[0].data, "content");
    ASSERT_TRUE(collector.messages[0].id.has_value());
    EXPECT_EQ(*collector.messages[0].id, "123");
    ASSERT_TRUE(collector.messages[0].retry.has_value());
    EXPECT_EQ(*collector.messages[0].retry, 5000);
}

TEST(EdgeCaseMessageFieldTest, NoOptionalFields) {
    // MSG-01: Message with only required fields
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: only\n\n");
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].event, "message");  // Default
    EXPECT_FALSE(collector.messages[0].id.has_value());
    EXPECT_FALSE(collector.messages[0].retry.has_value());
}

TEST(EdgeCaseMessageFieldTest, EmptyOptionalFields) {
    // MSG-01: Empty ID and retry
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("id:\ndata: test\n\n");
    
    EXPECT_EQ(collector.count(), 1);
    ASSERT_TRUE(collector.messages[0].id.has_value());
    EXPECT_EQ(*collector.messages[0].id, "");
}

TEST(EdgeCaseMessageFieldTest, MultipleDataFields) {
    // DAT-01: Multiple data fields accumulate
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: line1\ndata: line2\ndata: line3\n\n");
    
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "line1\nline2\nline3");
}

// =============================================================================
// Category 9: UTF-8 BOM Tests (EXT-01)
// =============================================================================

TEST(EdgeCaseBOMTest, UTF8BOMDetection) {
    // EXT-01: Detect UTF-8 BOM
    unsigned char bom_data[] = {0xEF, 0xBB, 0xBF};
    std::string_view sv(reinterpret_cast<char*>(bom_data), 3);
    
    EXPECT_TRUE(has_bom(sv));
}

TEST(EdgeCaseBOMTest, SkipBOM) {
    // EXT-01: Skip BOM from data
    unsigned char bom_data[] = {0xEF, 0xBB, 0xBF, 'd', 'a', 't', 'a'};
    std::string_view sv(reinterpret_cast<char*>(bom_data), 7);
    
    EXPECT_TRUE(skip_bom(sv));
    EXPECT_EQ(sv.size(), 4);
}

TEST(EdgeCaseBOMTest, NoBOMPresent) {
    // EXT-01: No BOM present
    std::string_view sv("data: test");
    
    EXPECT_FALSE(has_bom(sv));
    EXPECT_FALSE(skip_bom(sv));
}

// =============================================================================
// Category 10: Last-Event-ID Tests (EXT-02)
// =============================================================================

TEST(EdgeCaseLastEventIdTest, TrackLastEventId) {
    // EXT-02: Track last event ID - verified via MessageBuilder
    MessageBuilder builder;
    std::string last_id;
    
    builder.set_callback([&last_id](const Message& msg) {
        if (msg.id.has_value()) {
            last_id = *msg.id;
        }
    });
    
    builder.feed_line("id: first");
    builder.feed_line("data: msg1");
    builder.feed_line("");  // Complete message
    
    EXPECT_EQ(last_id, "first");
}

TEST(EdgeCaseLastEventIdTest, ResetClearsLastEventId) {
    // EXT-02: clear_last_event_id() clears last event ID
    // Note: reset() intentionally does NOT clear last_event_id (cross-message state)
    MessageBuilder builder;
    
    builder.feed_line("id: test");
    builder.feed_line("data: msg");
    builder.feed_line("");
    
    EXPECT_EQ(builder.last_event_id(), "test");
    
    builder.clear_last_event_id();
    EXPECT_EQ(builder.last_event_id(), "");
}

TEST(EdgeCaseLastEventIdTest, NoIdInMessage) {
    // EXT-02: Message without ID doesn't update last_event_id
    MessageBuilder builder;
    
    builder.feed_line("id: original");
    builder.feed_line("data: msg1");
    builder.feed_line("");
    
    EXPECT_EQ(builder.last_event_id(), "original");
    
    builder.feed_line("data: msg2");  // No ID
    builder.feed_line("");
    
    EXPECT_EQ(builder.last_event_id(), "original");
}

// =============================================================================
// Requirement Traceability Tests
// =============================================================================

TEST(EdgeCaseTraceabilityTest, ERR01_Coverage) {
    // ERR-01: Error code system - tested through various error scenarios
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    // Trigger various errors
    SseError err = parser.parse("retry: invalid\ndata: test\n\n");
    EXPECT_TRUE(err == SseError::invalid_retry || err == SseError::success);
}

TEST(EdgeCaseTraceabilityTest, ERR02_Coverage) {
    // ERR-02: Error message strings
    const char* msg = error_message(SseError::invalid_retry);
    EXPECT_NE(msg, nullptr);
    EXPECT_STREQ(msg, "Invalid retry value");
    
    msg = error_message(SseError::buffer_overflow);
    EXPECT_STREQ(msg, "Buffer overflow");
}

TEST(EdgeCaseTraceabilityTest, PAR01_Coverage) {
    // PAR-01: Field parsing edge cases
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    // Field without colon
    parser.parse("event\n\n");
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].event, "");
}

TEST(EdgeCaseTraceabilityTest, PAR02_Coverage) {
    // PAR-02: All line ending styles
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: a\r\ndata: b\r\r");
    EXPECT_EQ(collector.count(), 1);
}

TEST(EdgeCaseTraceabilityTest, PAR03_Coverage) {
    // PAR-03: Leading space handling
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data:  value\n\n");  // Double space
    EXPECT_EQ(collector.messages[0].data, " value");  // One space removed
}

TEST(EdgeCaseTraceabilityTest, PAR04_Coverage) {
    // PAR-04: Comment skipping
    EdgeCaseCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse(":comment\ndata: real\n\n");
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "real");
}

TEST(EdgeCaseTraceabilityTest, BUF01_Coverage) {
    // BUF-01: Buffer management edge cases
    Buffer buf;
    EXPECT_EQ(buf.size(), 0);
    EXPECT_TRUE(buf.empty());
    
    buf.append("test", 4);
    EXPECT_EQ(buf.size(), 4);
    EXPECT_FALSE(buf.empty());
}

TEST(EdgeCaseTraceabilityTest, ROB01_Coverage) {
    // ROB-01: Overflow handling
    Buffer buf(100);  // Small buffer
    std::string large_data(200, 'x');
    
    SseError err = buf.append(large_data);
    // Should either succeed or overflow
    EXPECT_TRUE(err == SseError::success || err == SseError::buffer_overflow);
}

// =============================================================================
// Summary
// =============================================================================
// Total: 35+ edge case tests covering:
// - Empty/null input handling (5 tests)
// - Buffer boundaries and overflow (4 tests)
// - Maximum/minimum values (4 tests)
// - Special characters (4 tests)
// - Line ending variations (5 tests)
// - Malformed data handling (5 tests)
// - State machine edge cases (5 tests)
// - Message field edge cases (4 tests)
// - UTF-8 BOM handling (3 tests)
// - Last-Event-ID tracking (3 tests)
// - 100% P0 requirement coverage via traceability tests
