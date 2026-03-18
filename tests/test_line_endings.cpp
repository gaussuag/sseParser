#include <gtest/gtest.h>
#include "sse_parser/buffer.h"

using namespace sse;

// Helper: Create string with specific line ending
static std::string make_line_ending(const char* ending) {
    if (std::string_view(ending) == "LF") return "\n";
    if (std::string_view(ending) == "CRLF") return "\r\n";
    if (std::string_view(ending) == "CR") return "\r";
    return "";
}

// PAR-02: Test Matrix - 9 combinations
// Format: first_line_ending + second_line_ending = message boundary

TEST(LineEndingsTest, LF_LF_UnixStyle) {
    Buffer buf;
    // data: hello\n\n -> complete message
    ASSERT_EQ(buf.append("data: hello\n\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");  // Empty line = message boundary
}

TEST(LineEndingsTest, LF_CRLF_Mixed) {
    Buffer buf;
    // data: hello\n\r\n -> complete message
    ASSERT_EQ(buf.append("data: hello\n\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, LF_CR_Mixed) {
    Buffer buf;
    // data: hello\n\r -> complete message
    ASSERT_EQ(buf.append("data: hello\n\r"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, CRLF_LF_Mixed) {
    Buffer buf;
    // data: hello\r\n\n -> complete message
    ASSERT_EQ(buf.append("data: hello\r\n\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, CRLF_CRLF_HTTPStyle) {
    Buffer buf;
    // data: hello\r\n\r\n -> complete message (HTTP standard)
    ASSERT_EQ(buf.append("data: hello\r\n\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, CRLF_CR_Mixed) {
    Buffer buf;
    // data: hello\r\n\r -> complete message
    ASSERT_EQ(buf.append("data: hello\r\n\r"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, CR_LF_Mixed) {
    Buffer buf;
    // data: hello\r\n\n -> CR+LF creates complete message (CRLF + LF)
    ASSERT_EQ(buf.append("data: hello\r\n\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");  // Empty line from second LF
}

TEST(LineEndingsTest, CR_CRLF_Mixed) {
    Buffer buf;
    // data: hello\r\r\n -> complete message
    ASSERT_EQ(buf.append("data: hello\r\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, CR_CR_ClassicMac) {
    Buffer buf;
    // data: hello\r\r -> complete message (Classic Mac)
    ASSERT_EQ(buf.append("data: hello\r\r"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

// Additional edge case tests

TEST(LineEndingsTest, IncompleteMessageNoBoundary) {
    Buffer buf;
    // data: hello\n -> incomplete (no second newline)
    ASSERT_EQ(buf.append("data: hello\n"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
}

TEST(LineEndingsTest, MultipleLinesInMessage) {
    Buffer buf;
    // event: test\ndata: hello\n\n -> complete message with 2 lines
    ASSERT_EQ(buf.append("event: test\ndata: hello\n\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "event: test");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "data: hello");
    
    auto line3 = buf.read_line();
    ASSERT_TRUE(line3.has_value());
    EXPECT_EQ(line3.value(), "");  // Boundary
}

TEST(LineEndingsTest, MixedLineEndingsInOneStream) {
    Buffer buf;
    // event: test\r\ndata: hello\n\r\n -> mixed CRLF and LF
    ASSERT_EQ(buf.append("event: test\r\ndata: hello\n\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "event: test");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "data: hello");
    
    auto line3 = buf.read_line();
    ASSERT_TRUE(line3.has_value());
    EXPECT_EQ(line3.value(), "");  // Boundary
}

TEST(LineEndingsTest, EmptyLineCRLF) {
    Buffer buf;
    // \r\n\r\n -> empty message
    ASSERT_EQ(buf.append("\r\n\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "");  // First empty line
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");  // Second empty line (boundary)
}

TEST(LineEndingsTest, TrailingCRHandled) {
    Buffer buf;
    // data: hello\r\r -> CR+CR creates complete message (classic Mac style)
    ASSERT_EQ(buf.append("data: hello\r\r"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");  // Empty line from second CR
}

TEST(LineEndingsTest, TrailingLFHandled) {
    Buffer buf;
    // data: hello\n -> single LF at end
    ASSERT_EQ(buf.append("data: hello\n"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    // Now add another LF to complete
    ASSERT_EQ(buf.append("\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");
}

TEST(LineEndingsTest, TrailingCRLFHandled) {
    Buffer buf;
    // data: hello\r\n\r\n -> CRLF+CRLF creates complete message (HTTP standard)
    ASSERT_EQ(buf.append("data: hello\r\n\r\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());

    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(line1.value(), "data: hello");

    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(line2.value(), "");  // Empty line from second CRLF
}
