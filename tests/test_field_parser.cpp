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
