# Testing Strategy: SSE Parser

**Framework:** Google Test (gtest)  
**Standard:** C++17  
**Goal:** 100% P0 requirement coverage + edge cases

---

## Test Structure

```
tests/
├── CMakeLists.txt          # 测试构建配置
├── test_main.cpp           # gtest main (已有)
├── test_ring_buffer.cpp    # Phase 1: 环形缓冲区测试
├── test_error_codes.cpp    # Phase 1: 错误码测试
├── test_message.cpp        # Phase 1: Message结构体测试
├── test_field_parser.cpp   # Phase 2: 字段解析测试
├── test_line_endings.cpp   # Phase 2: 换行符处理测试
├── test_message_builder.cpp # Phase 3: 消息组装测试
├── test_sse_parser.cpp     # Phase 4: 完整解析器测试
├── test_integration.cpp    # Phase 5: 集成测试
├── test_fuzzing.cpp        # Phase 5: 模糊测试
├── test_performance.cpp    # Phase 5: 性能基准测试
└── helpers/
    ├── sse_test_data.h     # 测试数据生成器
    └── mock_callbacks.h    # Mock回调函数
```

---

## Phase-by-Phase Testing

### Phase 1: Foundation Tests

**文件:** `test_ring_buffer.cpp`, `test_error_codes.cpp`, `test_message.cpp`

**测试目标:**
| 需求 | 测试用例 | 数量 |
|------|----------|------|
| BUF-01 (环形缓冲区) | 基础操作（push/pop） | 5 |
| | 边界条件（空/满） | 4 |
| | 环绕行为（wrap-around） | 3 |
| | 溢出处理 | 2 |
| ERR-01/02 (错误码) | 错误码枚举值 | 2 |
| | 错误信息字符串 | 4 |
| MSG-01 (Message结构) | 字段默认值 | 3 |
| | 内存布局 | 1 |

**示例测试:**
```cpp
TEST(RingBufferTest, BasicOperations) {
    RingBuffer<64> buf;
    EXPECT_TRUE(buf.empty());
    EXPECT_FALSE(buf.full());
    
    EXPECT_TRUE(buf.push("hello", 5));
    EXPECT_EQ(buf.size(), 5);
    
    char out[6];
    EXPECT_TRUE(buf.pop(out, 5));
    EXPECT_EQ(std::string_view(out, 5), "hello");
}

TEST(RingBufferTest, WrapAround) {
    RingBuffer<8> buf;
    buf.push("1234", 4);  // head=4
    buf.pop(out, 4);      // tail=4
    buf.push("5678", 4);  // head wraps to 0
    // Verify correct wrap behavior
}

TEST(RingBufferTest, Overflow) {
    RingBuffer<4> buf;
    EXPECT_TRUE(buf.push("ab", 2));
    EXPECT_FALSE(buf.push("cdef", 4));  // 4 bytes won't fit
    EXPECT_EQ(buf.size(), 2);
}
```

**成功标准:**
- [ ] 所有缓冲区测试通过
- [ ] 错误码可正确转换为字符串
- [ ] Message结构体内存布局符合预期

---

### Phase 2: Core Parsing Tests

**文件:** `test_field_parser.cpp`, `test_line_endings.cpp`

**测试目标:**
| 需求 | 测试类别 | 数量 |
|------|----------|------|
| PAR-01 (字段解析) | 标准字段 | 4 |
| | 冒号位置 | 4 |
| | 空值 | 2 |
| | 未知字段 | 2 |
| PAR-02 (换行符) | LF (\n\n) | 3 |
| | CRLF (\r\n\r\n) | 3 |
| | CR (\r\r) | 3 |
| | 混合样式 | 4 |
| PAR-03 (前导空格) | 单个空格移除 | 2 |
| | 多个空格保留 | 2 |
| PAR-04 (注释) | 注释跳过 | 2 |
| VAL-01 (retry验证) | 有效整数 | 3 |
| | 无效格式 | 3 |
| | 溢出值 | 2 |

**换行符测试矩阵:**
```cpp
struct LineEndingTestCase {
    const char* name;
    const char* input;
    size_t input_len;
};

INSTANTIATE_TEST_SUITE_P(
    LineEndings,
    LineEndingTest,
    ::testing::Values(
        LineEndingTestCase{"LF", "data: hello\n\n", 15},
        LineEndingTestCase{"CRLF", "data: hello\r\n\r\n", 17},
        LineEndingTestCase{"CR", "data: hello\r\r", 15},
        LineEndingTestCase{"Mixed1", "data: a\r\ndata: b\n\n", 18},
        LineEndingTestCase{"Mixed2", "data: a\ndata: b\r\n\r\n", 19},
        // ... more combinations
    )
);
```

**Spec合规性测试:**
```cpp
TEST_F(SSEParserSpecTest, LeadingSpaceRule) {
    // Spec: "If value starts with space, remove exactly one space"
    Parse("data: hello\n\n");
    EXPECT_EQ(last_message_.data, "hello");
    
    Parse("data:  hello\n\n");  // two spaces
    EXPECT_EQ(last_message_.data, " hello");  // one kept
    
    Parse("data:   hello\n\n");  // three spaces
    EXPECT_EQ(last_message_.data, "  hello");  // two kept
}
```

**成功标准:**
- [ ] 通过换行符样式矩阵（9种组合）
- [ ] 前导空格处理符合spec
- [ ] retry验证正确检测无效值

---

### Phase 3: Message Assembly Tests

**文件:** `test_message_builder.cpp`

**测试目标:**
| 需求 | 测试类别 | 数量 |
|------|----------|------|
| DAT-01 (多行data) | 两行data | 2 |
| | 多行data | 3 |
| | 空data行 | 2 |
| DAT-02 (空消息) | 无字段 | 1 |
| | 仅event | 1 |
| | 仅id | 1 |
| EXT-02 (Last-ID) | ID跟踪 | 2 |
| | ID更新 | 2 |

**多行数据测试:**
```cpp
TEST(MessageBuilderTest, MultiLineData) {
    // Input:
    // data: hello
    // data: world
    //
    // Expected: data = "hello\nworld"
    
    Parse("data: hello\ndata: world\n\n");
    EXPECT_EQ(last_message_.data, "hello\nworld");
}

TEST(MessageBuilderTest, MultiLineWithEmpty) {
    // data: hello
    // data:
    // data: world
    //
    // Expected: "hello\n\nworld"
    
    Parse("data: hello\ndata:\ndata: world\n\n");
    EXPECT_EQ(last_message_.data, "hello\n\nworld");
}
```

**成功标准:**
- [ ] 多行data正确连接（用\n）
- [ ] 空消息触发回调
- [ ] Last-Event-ID正确跟踪

---

### Phase 4: Public API Tests

**文件:** `test_sse_parser.cpp`

**测试目标:**
| 需求 | 测试类别 | 数量 |
|------|----------|------|
| API-01/02 (parse接口) | const char*接口 | 3 |
| | string_view接口 | 3 |
| | 零拷贝验证 | 2 |
| API-03 (回调) | 回调触发时机 | 3 |
| | 回调参数正确性 | 3 |
| API-04 (flush) | 不完整消息 | 2 |
| | 空flush | 1 |
| API-05 (reset) | 状态清空 | 2 |
| | 重置后正常解析 | 2 |
| IFC-01 (函数指针) | 函数指针回调 | 2 |

**分块输入测试:**
```cpp
TEST_F(SSEParserTest, FragmentedInput) {
    // Simulate network chunks arriving separately
    parser.parse("data: hel");
    EXPECT_EQ(message_count_, 0);  // No complete message yet
    
    parser.parse("lo\n\n");
    EXPECT_EQ(message_count_, 1);
    EXPECT_EQ(last_message_.data, "hello");
}

TEST_F(SSEParserTest, FragmentedAcrossLines) {
    parser.parse("data: line1\r\n");
    EXPECT_EQ(message_count_, 0);
    
    parser.parse("data: line2\r\n");
    EXPECT_EQ(message_count_, 0);
    
    parser.parse("\r\n");  // End of message
    EXPECT_EQ(message_count_, 1);
    EXPECT_EQ(last_message_.data, "line1\nline2");
}
```

**回调类型测试:**
```cpp
TEST_F(SSEParserTest, StdFunctionCallback) {
    int call_count = 0;
    SseParser parser([&call_count](const Message&) {
        ++call_count;
    });
    
    parser.parse("data: test\n\n");
    EXPECT_EQ(call_count, 1);
}

TEST_F(SSEParserTest, FunctionPointerCallback) {
    static int call_count = 0;
    auto callback = [](const Message&) { ++call_count; };
    
    SseParser<4096, decltype(callback)> parser(callback);
    parser.parse("data: test\n\n");
    EXPECT_EQ(call_count, 1);
}
```

**flush测试:**
```cpp
TEST_F(SSEParserTest, FlushIncompleteMessage) {
    parser.parse("data: incomplete");  // No \n\n
    EXPECT_EQ(message_count_, 0);
    
    parser.flush();  // Deliver partial message
    EXPECT_EQ(message_count_, 1);
    EXPECT_EQ(last_message_.data, "incomplete");
}
```

**成功标准:**
- [ ] 分块输入正确处理
- [ ] 回调在正确时机触发
- [ ] flush()交付不完整消息
- [ ] reset()清空所有状态

---

### Phase 5: Integration & Edge Cases

**文件:** `test_integration.cpp`, `test_fuzzing.cpp`, `test_performance.cpp`

**测试目标:**
| 类别 | 测试内容 | 数量 |
|------|----------|------|
| 集成测试 | 完整SSE流 | 5 |
| | LLM API模拟 | 3 |
| | 错误恢复 | 3 |
| 边界测试 | 最大消息大小 | 2 |
| | 缓冲区边界 | 4 |
| | 空输入 | 2 |
| Fuzzing | 随机输入 | 持续运行 |
| 性能 | 解析速度 | 1 |
| | 内存使用 | 1 |

**集成测试（LLM场景）:**
```cpp
TEST(IntegrationTest, LLMStreamingSimulation) {
    // Simulate OpenAI-style SSE stream
    const char* stream = 
        "event: message\n"
        "data: {\"role\":\"assistant\"}\n\n"
        "event: message\n"
        "data: {\"content\":\"Hello\"}\n\n"
        "event: done\n"
        "data: [DONE]\n\n";
    
    std::vector<Message> messages;
    SseParser parser([&messages](const Message& m) {
        messages.push_back(m);
    });
    
    parser.parse(stream);
    
    EXPECT_EQ(messages.size(), 3);
    EXPECT_EQ(messages[0].event, "message");
    EXPECT_EQ(messages[0].data, "{\"role\":\"assistant\"}");
}
```

**边界测试:**
```cpp
TEST(BoundaryTest, MaxMessageSize) {
    // Test behavior when message exceeds buffer
    std::string large_data(5000, 'x');  // Larger than default buffer
    std::string input = "data: " + large_data + "\n\n";
    
    auto err = parser.parse(input);
    EXPECT_EQ(err, SseError::buffer_overflow);
}

TEST(BoundaryTest, EmptyInput) {
    auto err = parser.parse("");
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(message_count_, 0);
}
```

**Fuzzing测试:**
```cpp
TEST(FuzzingTest, RandomInput) {
    // Use gtest with random data
    std::mt19937 rng(42);
    std::uniform_int_distribution<> dist(0, 255);
    
    for (int i = 0; i < 1000; ++i) {
        std::vector<char> data(100);
        for (auto& c : data) {
            c = static_cast<char>(dist(rng));
        }
        
        SseParser parser([](const Message&) {});
        // Should not crash or hang
        parser.parse(data.data(), data.size());
        parser.flush();
        parser.reset();
    }
}
```

**性能基准:**
```cpp
TEST(PerformanceTest, ParseThroughput) {
    // Generate 10MB of SSE data
    std::string data = GenerateLargeSSEData(10 * 1024 * 1024);
    
    SseParser parser([](const Message&) {});
    
    auto start = std::chrono::high_resolution_clock::now();
    parser.parse(data);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double throughput = (data.size() / 1024.0 / 1024.0) / (duration.count() / 1000.0);
    
    EXPECT_GT(throughput, 100.0);  // > 100 MB/s
}
```

**成功标准:**
- [ ] 100% P0需求测试覆盖
- [ ] 随机输入不崩溃（1000+次迭代）
- [ ] 性能 >100MB/s
- [ ] 内存使用 <8KB每实例

---

## Test Data Helpers

**sse_test_data.h:**
```cpp
#pragma once
#include <string>
#include <vector>

namespace sse_test {

// Standard test messages
inline const char* kSimpleMessage = "data: hello\n\n";
inline const char* kMultiLineMessage = "data: line1\ndata: line2\n\n";
inline const char* kFullMessage = 
    "event: message\n"
    "data: content\n"
    "id: 123\n"
    "retry: 5000\n\n";

// Line ending variations
struct MessageVariant {
    const char* name;
    const char* data;
};

inline const std::vector<MessageVariant> kLineEndingVariants = {
    {"LF", "data: test\n\n"},
    {"CRLF", "data: test\r\n\r\n"},
    {"CR", "data: test\r\r"},
};

// Generate large SSE stream for performance testing
std::string GenerateLargeSSEData(size_t target_bytes);

} // namespace sse_test
```

---

## CI/CD Integration

**GitHub Actions workflow (.github/workflows/test.yml):**
```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        compiler: [gcc, clang, msvc]
        exclude:
          - os: windows-latest
            compiler: clang
          - os: macos-latest
            compiler: gcc
            
    steps:
    - uses: actions/checkout@v3
    
    - name: Configure
      run: cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
      
    - name: Build
      run: cmake --build build --config Release
      
    - name: Test
      run: ctest --test-dir build -C Release --output-on-failure
      
    - name: Coverage
      if: matrix.os == 'ubuntu-latest'
      run: |
        cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
        cmake --build build
        ctest --test-dir build
        gcovr --xml-pretty --output coverage.xml
```

---

## Coverage Goals

| 阶段 | 目标覆盖率 | 工具 |
|------|-----------|------|
| Phase 1 | 90% 行覆盖率 | gcov + gcovr |
| Phase 2 | 85% 行覆盖率 | gcov + gcovr |
| Phase 3 | 90% 行覆盖率 | gcov + gcovr |
| Phase 4 | 85% 行覆盖率 | gcov + gcovr |
| Phase 5 | 100% P0需求 | 需求追溯矩阵 |

**豁免区域:**
- `assert()` 语句（不应触发）
- 调试日志输出
- 平台特定的错误处理

---

## Running Tests

```bash
# Configure with tests
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
make -j$(nproc)

# Run all tests
ctest --output-on-failure

# Run specific test
./tests/PlaygroundCpp_tests --gtest_filter="RingBuffer*"

# Run with verbose output
./tests/PlaygroundCpp_tests --gtest_also_run_disabled_tests

# Generate coverage (Linux only)
cmake .. -DENABLE_COVERAGE=ON
make
ctest
gcovr -r .. --html --html-details -o coverage.html
```

---

*Created: 2025-03-17*  
*Framework: Google Test*  
*Target: 100% P0 requirement coverage*
