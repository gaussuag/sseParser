#include <gtest/gtest.h>
#include "sse_parser/error_codes.h"

using namespace sse;

TEST(ErrorCodes, EnumValues) {
    EXPECT_EQ(static_cast<uint8_t>(SseError::success), 0);
    EXPECT_NE(static_cast<uint8_t>(SseError::incomplete_message), 0);
    EXPECT_NE(static_cast<uint8_t>(SseError::invalid_retry), 0);
    EXPECT_NE(static_cast<uint8_t>(SseError::out_of_memory), 0);
    EXPECT_NE(static_cast<uint8_t>(SseError::buffer_overflow), 0);
}

TEST(ErrorCodes, ErrorMessages) {
    EXPECT_STREQ(error_message(SseError::success), "Success");
    EXPECT_STREQ(error_message(SseError::incomplete_message), "Message incomplete");
    EXPECT_STREQ(error_message(SseError::invalid_retry), "Invalid retry value");
    EXPECT_STREQ(error_message(SseError::out_of_memory), "Out of memory");
    EXPECT_STREQ(error_message(SseError::buffer_overflow), "Buffer overflow");
}

TEST(ErrorCodes, BoolConversion) {
    EXPECT_TRUE(!SseError::success);
    EXPECT_FALSE(!SseError::incomplete_message);
    EXPECT_FALSE(!SseError::invalid_retry);
    EXPECT_FALSE(!SseError::out_of_memory);
    EXPECT_FALSE(!SseError::buffer_overflow);
}

TEST(ErrorCodes, EnumDistinctness) {
    EXPECT_NE(static_cast<uint8_t>(SseError::success), static_cast<uint8_t>(SseError::incomplete_message));
    EXPECT_NE(static_cast<uint8_t>(SseError::incomplete_message), static_cast<uint8_t>(SseError::invalid_retry));
    EXPECT_NE(static_cast<uint8_t>(SseError::invalid_retry), static_cast<uint8_t>(SseError::out_of_memory));
    EXPECT_NE(static_cast<uint8_t>(SseError::out_of_memory), static_cast<uint8_t>(SseError::buffer_overflow));
}

TEST(ErrorCodes, AllValuesHaveNonEmptyMessages) {
    EXPECT_STRNE(error_message(SseError::success), "");
    EXPECT_STRNE(error_message(SseError::incomplete_message), "");
    EXPECT_STRNE(error_message(SseError::invalid_retry), "");
    EXPECT_STRNE(error_message(SseError::out_of_memory), "");
    EXPECT_STRNE(error_message(SseError::buffer_overflow), "");
}

TEST(ErrorCodes, UnknownErrorMessage) {
    // Test that an unknown error code returns "Unknown error"
    // This tests the default case in the switch statement
    SseError unknown = static_cast<SseError>(255);
    EXPECT_STREQ(error_message(unknown), "Unknown error");
}
