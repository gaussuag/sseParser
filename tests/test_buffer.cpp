#include <gtest/gtest.h>
#include "sse_parser/buffer.h"
#include <string>
#include <string_view>

using namespace sse_parser;

class BufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique<Buffer>();
    }

    void TearDown() override {
        buffer.reset();
    }

    std::unique_ptr<Buffer> buffer;
};

TEST_F(BufferTest, EmptyConstruction) {
    EXPECT_TRUE(buffer->empty());
    EXPECT_EQ(buffer->size(), 0);
    EXPECT_TRUE(buffer->view().empty());
}

TEST_F(BufferTest, AppendAndSizeTracking) {
    const char* data = "Hello, World!";
    size_t len = strlen(data);
    
    auto result = buffer->append(data, len);
    EXPECT_EQ(result, SseError::success);
    EXPECT_EQ(buffer->size(), len);
    EXPECT_FALSE(buffer->empty());
}

TEST_F(BufferTest, StringViewAppendOverload) {
    std::string_view data = "Hello from string_view";
    
    auto result = buffer->append(data);
    EXPECT_EQ(result, SseError::success);
    EXPECT_EQ(buffer->size(), data.size());
    EXPECT_EQ(buffer->view(), data);
}

TEST_F(BufferTest, MultipleAppendOperations) {
    buffer->append("First", 5);
    buffer->append("Second", 6);
    buffer->append("Third", 5);
    
    EXPECT_EQ(buffer->size(), 16);
    EXPECT_EQ(buffer->view(), "FirstSecondThird");
}

TEST_F(BufferTest, ReadLineLF) {
    buffer->append("Line 1\nLine 2\n");
    
    auto line1 = buffer->read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(*line1, "Line 1");
    
    auto line2 = buffer->read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(*line2, "Line 2");
    
    auto line3 = buffer->read_line();
    EXPECT_FALSE(line3.has_value());
}

TEST_F(BufferTest, ReadLineCRLF) {
    buffer->append("Line 1\r\nLine 2\r\n");
    
    auto line1 = buffer->read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(*line1, "Line 1");
    
    auto line2 = buffer->read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(*line2, "Line 2");
    
    auto line3 = buffer->read_line();
    EXPECT_FALSE(line3.has_value());
}

TEST_F(BufferTest, ReadLineCR) {
    buffer->append("Line 1\rLine 2\r");
    
    auto line1 = buffer->read_line();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(*line1, "Line 1");
    
    auto line2 = buffer->read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(*line2, "Line 2");
    
    auto line3 = buffer->read_line();
    EXPECT_FALSE(line3.has_value());
}

TEST_F(BufferTest, IncompleteLineNoNewline) {
    buffer->append("Partial line without newline");
    
    auto line = buffer->read_line();
    EXPECT_FALSE(line.has_value());
}

TEST_F(BufferTest, MixedLineEndings) {
    buffer->append("LF\nCRLF\r\nCR\r");
    
    auto lf = buffer->read_line();
    ASSERT_TRUE(lf.has_value());
    EXPECT_EQ(*lf, "LF");
    
    auto crlf = buffer->read_line();
    ASSERT_TRUE(crlf.has_value());
    EXPECT_EQ(*crlf, "CRLF");
    
    auto cr = buffer->read_line();
    ASSERT_TRUE(cr.has_value());
    EXPECT_EQ(*cr, "CR");
}

TEST_F(BufferTest, HasCompleteMessageLF) {
    buffer->append("data: hello\n\n");
    EXPECT_TRUE(buffer->has_complete_message());
}

TEST_F(BufferTest, HasCompleteMessageCRLF) {
    buffer->append("data: hello\r\n\r\n");
    EXPECT_TRUE(buffer->has_complete_message());
}

TEST_F(BufferTest, HasCompleteMessageCR) {
    buffer->append("data: hello\r\r");
    EXPECT_TRUE(buffer->has_complete_message());
}

TEST_F(BufferTest, NoCompleteMessage) {
    buffer->append("data: hello\n");
    EXPECT_FALSE(buffer->has_complete_message());
}

TEST_F(BufferTest, ClearResetsBuffer) {
    buffer->append("Some data");
    EXPECT_FALSE(buffer->empty());
    
    buffer->clear();
    EXPECT_TRUE(buffer->empty());
    EXPECT_EQ(buffer->size(), 0);
    EXPECT_TRUE(buffer->view().empty());
    
    auto line = buffer->read_line();
    EXPECT_FALSE(line.has_value());
}

TEST_F(BufferTest, EmptyReturnsCorrectValue) {
    EXPECT_TRUE(buffer->empty());
    
    buffer->append("x", 1);
    EXPECT_FALSE(buffer->empty());
    
    buffer->read_line();
    EXPECT_FALSE(buffer->empty());
    
    buffer->clear();
    EXPECT_TRUE(buffer->empty());
}

TEST_F(BufferTest, ViewMethod) {
    std::string_view data = "Test view data";
    buffer->append(data);
    
    std::string_view view = buffer->view();
    EXPECT_EQ(view, data);
    
    auto line = buffer->read_line();
    EXPECT_FALSE(line.has_value());
    
    view = buffer->view();
    EXPECT_EQ(view, data);
}

TEST_F(BufferTest, LargeData100KB) {
    buffer->set_max_size(200 * 1024);
    std::string large_data(100 * 1024, 'x');
    
    auto result = buffer->append(large_data);
    EXPECT_EQ(result, SseError::success);
    EXPECT_EQ(buffer->size(), large_data.size());
}

TEST_F(BufferTest, OverflowHandlingDefaultSize) {
    Buffer small_buffer(10);
    
    auto result = small_buffer.append("123456789012");
    EXPECT_EQ(result, SseError::buffer_overflow);
}

TEST_F(BufferTest, OverflowHandledByCompact) {
    Buffer small_buffer(20);
    
    small_buffer.append("Line1\nLine2\n");
    small_buffer.read_line();
    small_buffer.read_line();
    
    auto result = small_buffer.append("New data");
    EXPECT_EQ(result, SseError::success);
}

TEST_F(BufferTest, MaxSizeGetterSetter) {
    EXPECT_EQ(buffer->max_size(), 4096);
    
    buffer->set_max_size(2048);
    EXPECT_EQ(buffer->max_size(), 2048);
}

TEST_F(BufferTest, ConstructorWithMaxSize) {
    Buffer custom_buffer(1024);
    EXPECT_EQ(custom_buffer.max_size(), 1024);
}

TEST_F(BufferTest, AppendEmptyData) {
    auto result = buffer->append("", 0);
    EXPECT_EQ(result, SseError::success);
    EXPECT_TRUE(buffer->empty());
}

TEST_F(BufferTest, ReadLineEmptyBuffer) {
    auto line = buffer->read_line();
    EXPECT_FALSE(line.has_value());
}

TEST_F(BufferTest, HasCompleteMessageEmptyBuffer) {
    EXPECT_FALSE(buffer->has_complete_message());
}

TEST_F(BufferTest, CompleteMessageWithContentLines) {
    buffer->append("data: line 1\ndata: line 2\n\n");
    EXPECT_TRUE(buffer->has_complete_message());
}

TEST_F(BufferTest, CompleteMessageWithCRLFContentLines) {
    buffer->append("data: line 1\r\ndata: line 2\r\n\r\n");
    EXPECT_TRUE(buffer->has_complete_message());
}

TEST_F(BufferTest, CompleteMessageWithCRContentLines) {
    buffer->append("data: line 1\rdata: line 2\r\r");
    EXPECT_TRUE(buffer->has_complete_message());
}

TEST_F(BufferTest, ReadAfterPartialRead) {
    buffer->append("Line 1\nLine 2\n");
    
    auto line1 = buffer->read_line();
    ASSERT_TRUE(line1.has_value());
    
    buffer->append("Line 3\n");
    
    auto line2 = buffer->read_line();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(*line2, "Line 2");
    
    auto line3 = buffer->read_line();
    ASSERT_TRUE(line3.has_value());
    EXPECT_EQ(*line3, "Line 3");
}

TEST_F(BufferTest, MixedOperationsSequence) {
    buffer->append("First\nSecond\n");
    
    auto line = buffer->read_line();
    EXPECT_EQ(*line, "First");
    
    buffer->append("Third\n\n");
    
    EXPECT_TRUE(buffer->has_complete_message());
    
    line = buffer->read_line();
    EXPECT_EQ(*line, "Second");
    
    line = buffer->read_line();
    EXPECT_EQ(*line, "Third");
    
    auto empty_line = buffer->read_line();
    ASSERT_TRUE(empty_line.has_value());
    EXPECT_TRUE(empty_line->empty());
    
    buffer->clear();
    EXPECT_TRUE(buffer->empty());
}

TEST_F(BufferTest, ErrorOperatorNot) {
    SseError success = SseError::success;
    SseError overflow = SseError::buffer_overflow;
    
    EXPECT_TRUE(!success);
    EXPECT_FALSE(!overflow);
}

TEST_F(BufferTest, ErrorMessageFunction) {
    EXPECT_STREQ(error_message(SseError::success), "Success");
    EXPECT_STREQ(error_message(SseError::buffer_overflow), "Buffer overflow");
    EXPECT_STREQ(error_message(SseError::incomplete_message), "Message incomplete");
    EXPECT_STREQ(error_message(SseError::invalid_retry), "Invalid retry value");
    EXPECT_STREQ(error_message(SseError::out_of_memory), "Out of memory");
}
