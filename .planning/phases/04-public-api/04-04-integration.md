---
id: "04-04"
phase: "04-public-api"
wave: 4
autonomous: true
objective: "Create comprehensive integration tests for SseParser with chunked input, callbacks, flush, and reset scenarios (12+ test cases, 85% coverage)"
req_ids: ["API-01", "API-02", "API-03", "API-04", "API-05", "IFC-01"]
---

# Plan: Phase 4 Integration Tests

## Objective
Create comprehensive test suite for SseParser covering all Phase 4 requirements, chunked input scenarios, and both callback types.

## Requirements Covered
- **API-01**: parse(const char*, size_t) - Raw byte input
- **API-02**: parse(string_view) - Zero-copy interface
- **API-03**: Callback interface - std::function
- **API-04**: flush() - Incomplete message handling
- **API-05**: reset() - State clearing
- **IFC-01**: Function pointer callbacks

## Read First
<read_first>
- `.planning/phases/04-public-api/04-CONTEXT.md` — Success criteria (lines 291-298)
- `.planning/ROADMAP.md` — Phase 4 test requirements (lines 114-125)
- `.planning/TESTING.md` — Testing strategy and patterns
- `include/sse_parser/sse_parser_facade.h` — Complete API to test
- `tests/test_message_builder.cpp` — Reference for integration test patterns
</read_first>

## Actions

### 1. Create Test File
<action id="1" name="create-test-file">
**File:** `tests/test_sse_parser.cpp`

**Test Categories (minimum 12 tests):**

#### Category A: Basic Parse Tests (API-01)
1. `Parse_SingleCompleteMessage` - "data: hello\n\n" delivers one message
2. `Parse_MultipleMessages` - "data: a\n\ndata: b\n\n" delivers two messages
3. `Parse_EmptyData` - Empty input returns success, no callback
4. `Parse_NullDataZeroLength` - nullptr with len=0 returns success

#### Category B: String View Interface (API-02)
5. `ParseStringView_SingleMessage` - std::string_view input works
6. `ParseStringView_ZeroCopy` - Verify no data modification
7. `ParseStringView_Equivalence` - Same results as parse(const char*, size_t)

#### Category C: Callback Tests (API-03, IFC-01)
8. `Callback_StdFunction_Lambda` - Lambda captures message correctly
9. `Callback_StdFunction_MultipleCalls` - Callback invoked for each message
10. `Callback_FunctionPointer_Basic` - C-style callback with user_data
11. `Callback_FunctionPointer_UserData` - void* user_data passed correctly

#### Category D: Chunked Input Scenarios
12. `Chunked_SplitAcrossLines` - "data: hel" + "lo\n\n" → one message
13. `Chunked_MultipleChunks` - 5 chunks for 2 messages
14. `Chunked_EmptyChunks` - Empty chunks handled gracefully
15. `Chunked_MidLineSplit` - Split in middle of field value

#### Category E: Flush Tests (API-04)
16. `Flush_IncompleteMessage` - "data: hello" (no newline) → flush() delivers
17. `Flush_NoIncompleteMessage` - Empty buffer → success, no callback
18. `Flush_CompleteMessageAlready` - "data: hi\n\n" + flush() → only one message
19. `Flush_DoubleFlush` - Second flush is no-op

#### Category F: Reset Tests (API-05)
20. `Reset_ClearsBuffer` - Partial data cleared
21. `Reset_KeepsCallback` - Callback still works after reset
22. `Reset_ClearsLastEventId` - last_event_id() returns empty
23. `Reset_MidMessage` - Reset during incomplete message, fresh state

#### Category G: Integration Scenarios
24. `Integration_ReconnectionScenario` - Parse + reset + parse works
25. `Integration_ChunkedWithFlush` - Multiple chunks + final flush

</action>

### 2. Implement Test Helpers
<action id="2" name="implement-test-helpers">
**File:** `tests/test_sse_parser.cpp` (top of file)

**Helper Structures:**
```cpp
// Collects all messages for verification
struct MessageCollector {
    std::vector<Message> messages;
    
    void operator()(const Message& msg) {
        messages.push_back(msg);
    }
    
    void clear() { messages.clear(); }
    size_t count() const { return messages.size(); }
};

// C-style callback for IFC-01 tests
struct CCallbackData {
    std::vector<Message>* messages;
    int call_count = 0;
};

extern "C" void c_callback(const Message* msg, void* user_data) {
    auto* data = static_cast<CCallbackData*>(user_data);
    data->messages->push_back(*msg);
    ++data->call_count;
}
```
</action>

### 3. Implement Key Tests
<action id="3" name="implement-key-tests">
**Critical Test Implementations:**

```cpp
// Test chunked input (ROADMAP success criteria #1)
TEST(SseParserChunkedTest, SplitAcrossLines) {
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

// Test flush with incomplete message (ROADMAP success criteria #3)
TEST(SseParserFlushTest, IncompleteMessage) {
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

// Test reset clears state (ROADMAP success criteria #4)
TEST(SseParserResetTest, ClearsStateKeepsCallback) {
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    parser.parse("data: partial", 13);
    parser.reset();
    
    // After reset, should be fresh state
    parser.parse("data: new\n\n", 11);
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "new");
}

// Test function pointer callback (IFC-01)
TEST(SseParserCallbackTest, FunctionPointer) {
    std::vector<Message> messages;
    CCallbackData data{&messages};
    
    SseParser parser;
    parser.set_callback(c_callback, &data);
    
    parser.parse("data: test\n\n", 12);
    
    EXPECT_EQ(data.call_count, 1);
    EXPECT_EQ(messages[0].data, "test");
}
```
</action>

### 4. Verify Coverage
<action id="4" name="verify-coverage">
**Coverage Checklist:**

| Requirement | Test Coverage |
|-------------|---------------|
| API-01 | Tests 1-4, 12-15, 24-25 |
| API-02 | Tests 5-7 |
| API-03 | Tests 8-9 |
| API-04 | Tests 16-19 |
| API-05 | Tests 20-23 |
| IFC-01 | Tests 10-11 |

**Target:** 85% line coverage in sse_parser_facade.h
</action>

## Acceptance Criteria
<acceptance_criteria>
**Grep Verification:**

1. Test file exists:
```bash
ls -la tests/test_sse_parser.cpp
```

2. Minimum 12 test cases:
```bash
grep -c "^TEST(" tests/test_sse_parser.cpp
# Should be >= 12
```

3. All API methods tested:
```bash
grep -n "parse(" tests/test_sse_parser.cpp | wc -l
# Should have multiple parse calls
grep -n "flush()" tests/test_sse_parser.cpp
grep -n "reset()" tests/test_sse_parser.cpp
grep -n "set_callback" tests/test_sse_parser.cpp
```

4. Chunked input tests exist:
```bash
grep -n "Chunked" tests/test_sse_parser.cpp
```

5. Both callback types tested:
```bash
grep -n "std::function\|lambda" tests/test_sse_parser.cpp
grep -n "FunctionPointer\|c_callback" tests/test_sse_parser.cpp
```

6. Tests compile and pass:
```bash
mkdir -p build && cd build && cmake .. && make test_sse_parser
./tests/test_sse_parser
```

7. Coverage report shows >= 85% for facade:
```bash
# After running tests with coverage:
grep -A 5 "sse_parser_facade.h" build/coverage_report.txt
```
</acceptance_criteria>

## Dependencies
- 04-01: SseParser base class must be complete
- 04-02: parse(string_view) and flush() must be implemented
- 04-03: reset() and function pointer callbacks must be implemented

## Success Criteria Mapping
From ROADMAP.md Phase 4:
1. ✓ Handle chunked input correctly → Tests 12-15
2. ✓ Callback triggered on complete messages → Tests 8-11
3. ✓ flush() handles incomplete final message → Tests 16-19
4. ✓ reset() clears state correctly → Tests 20-23
5. ✓ string_view interface is zero-copy → Tests 5-7
