#include <gtest/gtest.h>
#include <sse_parser/message.h>

using namespace sse_parser;

TEST(Message, DefaultConstruction) {
    Message msg;
    EXPECT_EQ(msg.event, "message");
    EXPECT_TRUE(msg.data.empty());
    EXPECT_FALSE(msg.id.has_value());
    EXPECT_FALSE(msg.retry.has_value());
}

TEST(Message, Clear) {
    Message msg;
    msg.event = "custom";
    msg.data = "test data";
    msg.id = "123";
    msg.retry = 5000;
    
    msg.clear();
    
    EXPECT_EQ(msg.event, "message");
    EXPECT_TRUE(msg.data.empty());
    EXPECT_FALSE(msg.id.has_value());
    EXPECT_FALSE(msg.retry.has_value());
}

TEST(Message, Empty) {
    Message msg;
    EXPECT_TRUE(msg.empty());
    
    msg.data = "test";
    EXPECT_FALSE(msg.empty());
    
    msg.clear();
    EXPECT_TRUE(msg.empty());
}

TEST(Message, OptionalFields) {
    Message msg;
    
    msg.id = "event-id";
    EXPECT_TRUE(msg.id.has_value());
    EXPECT_EQ(*msg.id, "event-id");
    
    msg.retry = 3000;
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(*msg.retry, 3000);
    
    msg.id.reset();
    EXPECT_FALSE(msg.id.has_value());
}

TEST(Message, LargeData) {
    Message msg;
    msg.data = std::string(10000, 'x');
    EXPECT_EQ(msg.data.size(), 10000);
    EXPECT_FALSE(msg.empty());
}

TEST(Message, EmptyStrings) {
    Message msg;
    msg.event = "";
    EXPECT_EQ(msg.event, "");
    EXPECT_TRUE(msg.empty());
    
    msg.data = "";
    EXPECT_TRUE(msg.empty());
    
    msg.id = "";
    EXPECT_TRUE(msg.id.has_value());
    EXPECT_EQ(*msg.id, "");
}

TEST(Message, CustomEvent) {
    Message msg;
    msg.event = "custom-event";
    EXPECT_EQ(msg.event, "custom-event");
    
    msg.clear();
    EXPECT_EQ(msg.event, "message");
}

TEST(Message, RetryEdgeCases) {
    Message msg;
    
    msg.retry = 0;
    EXPECT_TRUE(msg.retry.has_value());
    EXPECT_EQ(*msg.retry, 0);
    
    msg.retry = -1;
    EXPECT_EQ(*msg.retry, -1);
    
    msg.retry = INT_MAX;
    EXPECT_EQ(*msg.retry, INT_MAX);
}
