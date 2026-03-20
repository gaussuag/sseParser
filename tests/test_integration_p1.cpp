#include <gtest/gtest.h>
#include "sse_parser/sse_parser.hpp"

#include <string>
#include <string_view>

using namespace sse;

// Phase 1 Integration Test Suite
// Verifies all Phase 1 components work together correctly

TEST(Phase1Integration, HeadersCompile) {
    // If this compiles and links, headers are correct
    EXPECT_TRUE(true);
}

TEST(Phase1Integration, BufferWithErrors) {
    Buffer buf;
    
    EXPECT_EQ(buf.append("event: test\n"), SseError::success);
    EXPECT_EQ(buf.append("data: hello\n"), SseError::success);
    EXPECT_EQ(buf.append("\n"), SseError::success);
    
    EXPECT_TRUE(buf.has_complete_message());
    
    int line_count = 0;
    while (buf.read_line().has_value()) {
        ++line_count;
    }
    EXPECT_EQ(line_count, 3);
}

TEST(Phase1Integration, ErrorHandling) {
    Buffer buf;
    
    EXPECT_EQ(buf.append("test\n"), SseError::success);
    EXPECT_STREQ(error_message(SseError::success), "Success");
    EXPECT_STRNE(error_message(SseError::out_of_memory), "");
}

TEST(Phase1Integration, RealisticSSEParsing) {
    // Simulate realistic SSE parsing scenario
    Buffer buf;
    Message msg;
    
    // Simulate receiving data in chunks (network fragmentation)
    EXPECT_EQ(buf.append("event: user"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    EXPECT_EQ(buf.append("_update\n"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    EXPECT_EQ(buf.append("data: {\"name\": \"John\""), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    EXPECT_EQ(buf.append(", \"age\": 30}\n"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    EXPECT_EQ(buf.append("id: 12345\n"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    EXPECT_EQ(buf.append("retry: 5000\n"), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());
    
    EXPECT_EQ(buf.append("\n"), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    // Read all lines
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(*line1, "event: user_update");
    
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(*line2, "data: {\"name\": \"John\", \"age\": 30}");
    
    auto line3 = buf.read_line();
    ASSERT_TRUE(line3.has_value());
    EXPECT_EQ(*line3, "id: 12345");
    
    auto line4 = buf.read_line();
    ASSERT_TRUE(line4.has_value());
    EXPECT_EQ(*line4, "retry: 5000");
    
    auto line5 = buf.read_line();
    ASSERT_TRUE(line5.has_value());
    EXPECT_TRUE(line5->empty());  // Empty line indicating message end
    
    // Simulate populating Message struct from parsed lines
    msg.event = "user_update";
    msg.data = "{\"name\": \"John\", \"age\": 30}";
    msg.id = "12345";
    msg.retry = 5000;
    
    EXPECT_EQ(msg.event, "user_update");
    EXPECT_EQ(msg.data, "{\"name\": \"John\", \"age\": 30}");
    EXPECT_TRUE(msg.id.has_value());
    EXPECT_EQ(*msg.id, "12345");
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(*msg.retry, 5000);
}

TEST(Phase1Integration, MultipleMessages) {
    Buffer buf;
    
    // Simulate stream with multiple SSE messages
    EXPECT_EQ(buf.append("data: message1\n\ndata: message2\n\n"), SseError::success);
    
    EXPECT_TRUE(buf.has_complete_message());
    
    // Read first message
    auto line1 = buf.read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(*line1, "data: message1");
    
    auto empty1 = buf.read_line();
    ASSERT_TRUE(empty1.has_value());
    EXPECT_TRUE(empty1->empty());
    
    EXPECT_TRUE(buf.has_complete_message());
    
    // Read second message
    auto line2 = buf.read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(*line2, "data: message2");
    
    auto empty2 = buf.read_line();
    ASSERT_TRUE(empty2.has_value());
    EXPECT_TRUE(empty2->empty());
    
    EXPECT_FALSE(buf.has_complete_message());
}

TEST(Phase1Integration, ErrorPropagation) {
    // Test error propagation through components
    Buffer small_buf(20);
    
    // Fill buffer to near capacity
    EXPECT_EQ(small_buf.append("0123456789\n"), SseError::success);
    EXPECT_EQ(small_buf.append("0123456\n"), SseError::success);
    
    // Read some data to free space (but compact hasn't happened yet)
    auto line = small_buf.read_line();
    ASSERT_TRUE(line.has_value());
    
    // Now this should succeed after compaction
    EXPECT_EQ(small_buf.append("new data\n"), SseError::success);
    
    // But this should fail - buffer full even after compact
    auto result = small_buf.append("too much data here");
    EXPECT_EQ(result, SseError::buffer_overflow);
    
    // Verify error message
    EXPECT_STREQ(error_message(result), "Buffer overflow");
    
    // Verify bool conversion
    EXPECT_TRUE(!SseError::success);
    EXPECT_FALSE(!SseError::buffer_overflow);
}

TEST(Phase1Integration, MessageReset) {
    Message msg;
    
    // Populate message
    msg.event = "custom_event";
    msg.data = "some data";
    msg.id = "id123";
    msg.retry = 1000;
    
    // Verify populated
    EXPECT_EQ(msg.event, "custom_event");
    EXPECT_FALSE(msg.empty());
    
    // Clear and verify reset to defaults
    msg.clear();
    
    EXPECT_EQ(msg.event, "message");
    EXPECT_TRUE(msg.data.empty());
    EXPECT_FALSE(msg.id.has_value());
    EXPECT_FALSE(msg.retry.has_value());
    EXPECT_TRUE(msg.empty());
}

TEST(Phase1Integration, CompletePhase1Workflow) {
    // End-to-end workflow simulating real SSE parsing
    Buffer buf;
    Message current_msg;
    int message_count = 0;
    
    // Simulate chunked network data
    std::string chunk1 = "event: message\n";
    std::string chunk2 = "data: Hello, ";
    std::string chunk3 = "World!\n";
    std::string chunk4 = "\n";
    
    // Process chunks
    EXPECT_EQ(buf.append(chunk1), SseError::success);
    EXPECT_EQ(buf.append(chunk2), SseError::success);
    EXPECT_EQ(buf.append(chunk3), SseError::success);
    EXPECT_FALSE(buf.has_complete_message());  // Still need empty line
    
    EXPECT_EQ(buf.append(chunk4), SseError::success);
    EXPECT_TRUE(buf.has_complete_message());
    
    // Parse lines (in a real parser, this would populate Message)
    while (true) {
        auto line = buf.read_line();
        if (!line.has_value()) break;
        
        if (line->empty()) {
            // Message complete
            message_count++;
            EXPECT_EQ(current_msg.event, "message");
            current_msg.clear();
        } else if (line->find("event: ") == 0) {
            current_msg.event = std::string(line->substr(7));
        } else if (line->find("data: ") == 0) {
            if (!current_msg.data.empty()) {
                current_msg.data += "\n";
            }
            current_msg.data += std::string(line->substr(6));
        }
    }
    
    EXPECT_EQ(message_count, 1);
}
