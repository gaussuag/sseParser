---
id: 01-07
phase: 1
wave: 2
autonomous: true
objective: Implement comprehensive tests for Message struct
req_ids: []
gap_closure: false
---

# Plan 01-07: Message Structure Tests

**Objective:** Implement comprehensive tests for the Message struct.

**Estimated Time:** 25 minutes

## Tasks

### Task 1: Create Test File
Create `tests/test_message.cpp`

### Task 2: Implement Default Construction Tests
```cpp
TEST(Message, DefaultConstruction) {
    Message msg;
    EXPECT_EQ(msg.event, "message");
    EXPECT_TRUE(msg.data.empty());
    EXPECT_FALSE(msg.id.has_value());
    EXPECT_FALSE(msg.retry.has_value());
}
```

### Task 3: Implement clear() Tests
```cpp
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
```

### Task 4: Implement empty() Tests
```cpp
TEST(Message, Empty) {
    Message msg;
    EXPECT_TRUE(msg.empty());
    
    msg.data = "test";
    EXPECT_FALSE(msg.empty());
    
    msg.clear();
    EXPECT_TRUE(msg.empty());
}
```

### Task 5: Implement Optional Fields Tests
```cpp
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
```

### Task 6: Add Large Data Test
```cpp
TEST(Message, LargeData) {
    Message msg;
    msg.data = std::string(10000, 'x');
    EXPECT_EQ(msg.data.size(), 10000);
    EXPECT_FALSE(msg.empty());
}
```

## Files Created

- `tests/test_message.cpp`

## Success Criteria

- [ ] All test cases pass
- [ ] 100% coverage of message.h
- [ ] Edge cases covered (empty, large data)
- [ ] Tests compile without warnings

## Dependencies

- 01-03: Message Structure

## Notes

Target: 100% coverage
