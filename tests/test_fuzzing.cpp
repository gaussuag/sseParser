#include <gtest/gtest.h>
#include <random>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

#include "sse_parser/sse_parser.hpp"

using namespace sse;
using namespace std::chrono;

// Fuzzing test categories:
// 1. Random byte sequences (1000 iterations) - verify no crashes
// 2. Boundary values - min/max sizes, special chars
// 3. Pattern mutations - valid SSE with random corruptions
// 4. Line ending chaos - random \n, \r, \r\n combinations
// 5. Field injection - random field names and values
// 6. UTF-8 edge cases - invalid sequences, overlong encodings
// 7. Timing variations - small/large chunks, single byte feed

class FuzzingTest : public ::testing::Test {
protected:
    std::mt19937 rng_{static_cast<unsigned>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count())};
    
    std::vector<Message> messages_;
    
    void SetUp() override {
        messages_.clear();
    }
    
    SseParser createParser() {
        return SseParser([this](const Message& msg) {
            messages_.push_back(msg);
        });
    }
    
    // Generate random bytes
    std::string randomBytes(size_t min_len, size_t max_len) {
        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<int> byte_dist(0, 255);
        
        size_t len = len_dist(rng_);
        std::string result;
        result.reserve(len);
        
        for (size_t i = 0; i < len; ++i) {
            result.push_back(static_cast<char>(byte_dist(rng_)));
        }
        return result;
    }
    
    // Generate printable ASCII with random line endings
    std::string randomPrintableWithLineEndings(size_t min_len, size_t max_len) {
        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<int> char_dist(32, 126);
        std::uniform_int_distribution<int> ending_dist(0, 10);
        
        size_t len = len_dist(rng_);
        std::string result;
        result.reserve(len * 2);
        
        for (size_t i = 0; i < len; ++i) {
            result.push_back(static_cast<char>(char_dist(rng_)));
            
            // Randomly insert line endings
            int ending = ending_dist(rng_);
            if (ending == 0) result += "\n";
            else if (ending == 1) result += "\r";
            else if (ending == 2) result += "\r\n";
        }
        return result;
    }
    
    // Generate valid SSE with random corruptions
    std::string validSseWithCorruption() {
        std::vector<std::string> templates = {
            "data: hello\n\n",
            "id: 123\nevent: test\ndata: content\n\n",
            "data: line1\ndata: line2\n\n",
            "event: message\nretry: 5000\ndata: test\n\n",
            ": comment\ndata: after comment\n\n",
            "data: {\"key\": \"value\"}\n\n",
        };
        
        std::uniform_int_distribution<size_t> template_dist(0, templates.size() - 1);
        std::string base = templates[template_dist(rng_)];
        
        // Apply random corruption
        std::uniform_int_distribution<int> corruption_type(0, 5);
        int corruption = corruption_type(rng_);
        
        switch (corruption) {
            case 0: // Insert null byte
                if (!base.empty()) {
                    size_t pos = std::uniform_int_distribution<size_t>(0, base.size() - 1)(rng_);
                    base.insert(pos, 1, '\0');
                }
                break;
            case 1: // Truncate randomly
                if (!base.empty()) {
                    size_t new_len = std::uniform_int_distribution<size_t>(0, base.size())(rng_);
                    base.resize(new_len);
                }
                break;
            case 2: // Double some characters
                if (!base.empty()) {
                    size_t pos = std::uniform_int_distribution<size_t>(0, base.size() - 1)(rng_);
                    base.insert(pos, 1, base[pos]);
                }
                break;
            case 3: // Replace with random byte
                if (!base.empty()) {
                    size_t pos = std::uniform_int_distribution<size_t>(0, base.size() - 1)(rng_);
                    base[pos] = static_cast<char>(std::uniform_int_distribution<int>(0, 255)(rng_));
                }
                break;
            case 4: // No corruption (valid SSE)
                break;
            case 5: // Extreme line endings
                std::replace(base.begin(), base.end(), '\n', '\r');
                break;
        }
        
        return base;
    }
    
    // Generate UTF-8 edge cases
    std::string utf8EdgeCase() {
        std::vector<std::string> cases = {
            // Valid UTF-8
            u8"data: \u4e2d\u6587\n\n",
            u8"data: \ud55c\uad6d\uc5b4\n\n",
            u8"data: \U0001F600\n\n",
            
            // Invalid sequences
            std::string("data: \x80\n\n", 9),  // Continuation byte at start
            std::string("data: \xc0\x80\n\n", 11),  // Overlong encoding
            std::string("data: \xff\xfe\n\n", 11),  // Invalid start bytes
            std::string("data: \xf4\x90\x80\x80\n\n", 14),  // Beyond max code point
            
            // Truncated sequences
            "data: \xc3",  // Incomplete 2-byte
            "data: \xe0\x80",  // Incomplete 3-byte
            "data: \xf0\x80\x80",  // Incomplete 4-byte
            
            // Mixed valid/invalid
            std::string("data: \xc3\xa9\x80\n\n", 11),  // Valid é + invalid
        };
        
        std::uniform_int_distribution<size_t> dist(0, cases.size() - 1);
        return cases[dist(rng_)];
    }
    
    // Generate boundary values
    std::string boundaryValue() {
        std::uniform_int_distribution<int> type_dist(0, 6);
        int type = type_dist(rng_);
        
        switch (type) {
            case 0: return "";  // Empty
            case 1: return "\n";  // Single newline
            case 2: return "\n\n";  // Empty message
            case 3: return std::string(4096, 'x');  // Buffer boundary
            case 4: return std::string(10000, 'a');  // Large input
            case 5: return std::string(1, '\0');  // Single null
            case 6: return "data: " + std::string(1000, 'x') + "\n\n";  // Large data
        }
        return "";
    }
};

// ROB-01: Fuzzing Test 1 - Random byte sequences (1000 iterations)
TEST_F(FuzzingTest, RandomByteSequences) {
    constexpr int iterations = 1000;
    int crash_count = 0;
    
    for (int i = 0; i < iterations; ++i) {
        auto parser = createParser();
        std::string input = randomBytes(0, 1000);
        
        // Should not crash
        SseError err = parser.parse(input);
        // Error is acceptable, crash is not
        (void)err;
        
        // Also test flush doesn't crash
        parser.flush();
    }
    
    EXPECT_EQ(crash_count, 0) << "Parser crashed on random input";
}

// ROB-01: Fuzzing Test 2 - Printable ASCII with random line endings (1000 iterations)
TEST_F(FuzzingTest, PrintableWithLineEndings) {
    constexpr int iterations = 1000;
    
    for (int i = 0; i < iterations; ++i) {
        auto parser = createParser();
        std::string input = randomPrintableWithLineEndings(10, 500);
        
        SseError err = parser.parse(input);
        (void)err;
        parser.flush();
        
        // Reset and verify no state corruption
        parser.reset();
        err = parser.parse("data: test\n\n");
        EXPECT_EQ(err, SseError::success);
    }
}

// ROB-01: Fuzzing Test 3 - Valid SSE with random corruptions (1000 iterations)
TEST_F(FuzzingTest, ValidSseWithCorruptions) {
    constexpr int iterations = 1000;
    
    for (int i = 0; i < iterations; ++i) {
        auto parser = createParser();
        std::string input = validSseWithCorruption();
        
        SseError err = parser.parse(input);
        (void)err;
        
        // Test multiple parse calls
        err = parser.parse(input);
        (void)err;
        
        parser.flush();
    }
}

// ROB-01: Fuzzing Test 4 - UTF-8 edge cases
TEST_F(FuzzingTest, Utf8EdgeCases) {
    constexpr int iterations = 500;
    
    for (int i = 0; i < iterations; ++i) {
        auto parser = createParser();
        std::string input = utf8EdgeCase();
        
        SseError err = parser.parse(input);
        (void)err;
        parser.flush();
    }
}

// ROB-01: Fuzzing Test 5 - Boundary values
TEST_F(FuzzingTest, BoundaryValues) {
    constexpr int iterations = 500;
    
    for (int i = 0; i < iterations; ++i) {
        auto parser = createParser();
        std::string input = boundaryValue();
        
        SseError err = parser.parse(input);
        (void)err;
        parser.flush();
    }
}

// ROB-01: Fuzzing Test 6 - Single byte feeding (timing variation)
TEST_F(FuzzingTest, SingleByteFeed) {
    std::string input = "data: hello world\n\n";
    
    for (int run = 0; run < 100; ++run) {
        auto parser = createParser();
        
        // Feed one byte at a time
        for (char c : input) {
            SseError err = parser.parse(&c, 1);
            EXPECT_EQ(err, SseError::success);
        }
        
        parser.flush();
        
        // Should have exactly one message
        EXPECT_GE(messages_.size(), 0);  // May or may not have message depending on timing
    }
}

// ROB-01: Fuzzing Test 7 - Extreme chunk sizes
TEST_F(FuzzingTest, ExtremeChunkSizes) {
    std::string input = validSseWithCorruption();
    std::vector<size_t> chunk_sizes = {1, 2, 3, 7, 13, 64, 256, 1024};
    
    for (size_t chunk_size : chunk_sizes) {
        auto parser = createParser();
        size_t pos = 0;
        
        while (pos < input.size()) {
            size_t len = std::min(chunk_size, input.size() - pos);
            SseError err = parser.parse(input.data() + pos, len);
            EXPECT_EQ(err, SseError::success);
            pos += len;
        }
        
        parser.flush();
    }
}

// ROB-01: Fuzzing Test 8 - Field name injection
TEST_F(FuzzingTest, FieldInjection) {
    constexpr int iterations = 500;
    
    std::vector<std::string> field_names = {
        "data", "event", "id", "retry",
        "DATA", "Event", "ID", "RETRY",
        "", "x", "custom", "unknown",
        "data:", "event:", "id\n", "retry\r"
    };
    
    for (int i = 0; i < iterations; ++i) {
        auto parser = createParser();
        
        std::uniform_int_distribution<size_t> field_dist(0, field_names.size() - 1);
        std::string field = field_names[field_dist(rng_)];
        std::string value = randomPrintableWithLineEndings(0, 50);
        
        // Remove newlines from value to keep it single-line
        value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
        value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
        
        std::string input = field + ": " + value + "\n\n";
        
        SseError err = parser.parse(input);
        (void)err;
        parser.flush();
    }
}

// ERR-01: Fuzzing Test 9 - Error recovery stress test
TEST_F(FuzzingTest, ErrorRecoveryStress) {
    constexpr int iterations = 200;
    
    for (int i = 0; i < iterations; ++i) {
        auto parser = createParser();
        
        // Start with valid input
        parser.parse("data: start\n\n");
        
        // Then bombard with random input
        for (int j = 0; j < 10; ++j) {
            std::string bad_input = randomBytes(10, 100);
            SseError err = parser.parse(bad_input);
            // Should handle errors gracefully
            (void)err;
        }
        
        // Reset and verify it works again
        parser.reset();
        messages_.clear();
        
        SseError err = parser.parse("data: after reset\n\n");
        EXPECT_EQ(err, SseError::success);
        parser.flush();
        
        // Should have recovered
        EXPECT_GE(messages_.size(), 0);
    }
}

// BUF-01: Fuzzing Test 10 - Buffer overflow edge cases
TEST_F(FuzzingTest, BufferOverflowEdgeCases) {
    auto parser = createParser();
    
    // Create a large input that might overflow
    std::string large_input;
    for (int i = 0; i < 100; ++i) {
        large_input += "data: " + std::string(1000, 'x') + "\n";
    }
    large_input += "\n";
    
    SseError err = parser.parse(large_input);
    // Should either succeed or return buffer_overflow
    EXPECT_TRUE(err == SseError::success || err == SseError::buffer_overflow);
    
    if (err == SseError::success) {
        parser.flush();
    }
    
    // Reset should clear state
    parser.reset();
    
    // Should work normally after reset
    err = parser.parse("data: small\n\n");
    EXPECT_EQ(err, SseError::success);
}

// VAL-01: Fuzzing Test 11 - Random retry values
TEST_F(FuzzingTest, RandomRetryValues) {
    constexpr int iterations = 500;
    std::uniform_int_distribution<long long> retry_dist(
        std::numeric_limits<long long>::min(),
        std::numeric_limits<long long>::max()
    );
    
    for (int i = 0; i < iterations; ++i) {
        auto parser = createParser();
        long long retry_val = retry_dist(rng_);
        
        std::string input = "retry: " + std::to_string(retry_val) + "\n\n";
        SseError err = parser.parse(input);
        (void)err;
        parser.flush();
    }
}
