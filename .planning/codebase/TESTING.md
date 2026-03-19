# Testing

## Test Framework

**GoogleTest** - Primary testing framework
- Location: `libs/googletest/` (bundled)
- Version: Latest stable (via git submodule or vendored)
- Features used: TEST macros, EXPECT_*/ASSERT_*, gtest_main

## Test Organization

### Test Executables

1. **sse_parser_tests** - Main SSE parser test suite
   - 12 test files
   - Comprehensive coverage of all components
   - Command: `build\tests\Debug\sse_parser_tests.exe`

2. **sseParser_tests** - Original project tests
   - May require external dependencies (curl)
   - Separate test runner

### Test Files

| File | Focus | Test Count |
|------|-------|------------|
| `test_sse_parser.cpp` | Public API (SseParser class) | 25+ |
| `test_field_parser.cpp` | Field line parsing | 15+ |
| `test_line_endings.cpp` | LF/CRLF/CR handling | 8+ |
| `test_message.cpp` | Message struct | 6+ |
| `test_buffer.cpp` | Buffer management | 10+ |
| `test_error_codes.cpp` | Error handling | 5+ |
| `test_integration.cpp` | End-to-end scenarios | 10+ |
| `test_integration_p1.cpp` | Phase 1 features | 8+ |
| `test_integration_p2.cpp` | Phase 2 features | 8+ |
| `test_integration_p3.cpp` | Phase 3 features | 8+ |
| `test_edge_cases.cpp` | Edge cases | 12+ |
| `test_fuzzing.cpp` | Fuzzing/property tests | 6+ |
| `test_performance.cpp` | Benchmarks | 4+ |

**Total: ~115+ test cases**

## Test Categories

### Unit Tests

**SseParser Tests** (`test_sse_parser.cpp`):
- Basic parsing (API-01)
- String view interface (API-02)
- Callback types (API-03, IFC-01)
- Chunked input handling
- Flush functionality (API-04)
- Reset functionality (API-05)
- Integration scenarios
- Edge cases

**Field Parser Tests** (`test_field_parser.cpp`):
- Event field parsing
- Data field parsing
- ID field parsing
- Retry field validation (overflow, negative, non-numeric)
- Comment skipping
- Leading space handling (PAR-03)
- BOM detection (EXT-01)

**Buffer Tests** (`test_buffer.cpp`):
- Append operations
- Line reading (LF, CRLF, CR)
- Compaction
- Overflow handling
- Size management

### Integration Tests

**Multi-phase Integration**:
- Phase 1: Error codes, message struct, buffer
- Phase 2: Field parser, line endings
- Phase 3: Message builder, integration

**Full Scenarios**:
- LLM streaming simulation
- Reconnection flows
- Multi-message streams
- Error recovery

### Edge Case Tests

**Handled Scenarios**:
- Empty messages (DAT-02)
- Empty data field
- Truncated streams
- Invalid UTF-8
- Maximum buffer size
- Integer overflow in retry

### Performance Tests

**Benchmarks** (`test_performance.cpp`):
- Throughput: Messages/second
- Memory usage
- Large stream processing
- Chunked parsing overhead

### Fuzzing Tests

**Property-Based Tests** (`test_fuzzing.cpp`):
- Random data injection
- Malformed SSE data
- Boundary conditions
- Memory safety validation

## Test Data

**Static Test Files** (`tests/data/`):
- `edge_cases.sse` - Edge case scenarios
- `large_stream.sse` - Performance test data
- `mixed_format.sse` - Mixed line endings
- `small_messages.sse` - Basic message data
- `binary_data.bin` - Binary data tests

## Test Helpers

### MessageCollector
```cpp
struct MessageCollector {
    std::vector<Message> messages;
    void operator()(const Message& msg) { messages.push_back(msg); }
    void clear() { messages.clear(); }
    size_t count() const { return messages.size(); }
};
```

### C-Style Callback Test
```cpp
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

## Running Tests

### Visual Studio
```batch
build\tests\Debug\sse_parser_tests.exe
```

### CTest
```batch
cd build
ctest -V
```

### All Tests
```batch
# Debug
build\tests\Debug\sse_parser_tests.exe

# Release
build\tests\Release\sse_parser_tests.exe
```

## CI/CD Testing

**GitHub Actions** (`.github/workflows/multi-compiler.yml`):
- Tests run on GCC, Clang, MSVC
- Strict warnings as errors (`-Werror`)
- Build verification with all compilers

### Coverage Areas

| Component | Coverage Target | Actual |
|-----------|----------------|--------|
| error_codes.h | 100% | 100% |
| message.h | 100% | 100% |
| buffer.h | 90%+ | 90%+ |
| field_parser.h | 90%+ | 90%+ |
| message_builder.h | 90%+ | 90%+ |
| sse_parser_facade.h | 85%+ | 85%+ |

## Test Writing Guidelines

### Test Naming
```cpp
TEST(CategoryTest, DescriptiveTestName) {
    // Arrange
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    // Act
    SseError err = parser.parse("data: test\n\n", 13);
    
    // Assert
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "test");
}
```

### Best Practices
- One logical concept per test
- Use descriptive test names
- Use collectors/aggregators for callback testing
- Test both success and error paths
- Test edge cases explicitly
