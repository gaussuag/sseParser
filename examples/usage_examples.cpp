/**
 * @file usage_examples.cpp
 * @brief Usage examples for SSE Parser library
 *
 * This file demonstrates common usage patterns for the SSE Parser library.
 * These examples are compiled and tested as part of the documentation.
 */

#include <iostream>
#include <sstream>
#include <vector>
#include "../include/sse_parser/sse_parser.h"

// Example 1: Basic usage with callback
// Demonstrates simplest use case - parse SSE stream with callback
void example_basic_callback() {
    std::cout << "=== Example 1: Basic Callback ===\n";

    // Create parser with callback
    sse::SseParser parser([](const sse::Message& msg) {
        std::cout << "Event: " << msg.event << "\n";
        std::cout << "Data: " << msg.data << "\n";
        if (msg.id.has_value()) {
            std::cout << "ID: " << msg.id.value() << "\n";
        }
        std::cout << "---\n";
    });

    // Simulate chunked SSE data
    const char* chunk1 = "data: Hello\n";
    const char* chunk2 = "data: World\n\n";

    parser.parse(chunk1, strlen(chunk1));  // Buffered, no callback yet
    parser.parse(chunk2, strlen(chunk2));  // Complete message, callback fires

    std::cout << "\n";
}

// Example 2: LLM streaming simulation
// Shows how to handle streaming LLM responses with multi-line data
void example_llm_streaming() {
    std::cout << "=== Example 2: LLM Streaming ===\n";

    std::vector<std::string> responses;

    sse::SseParser parser([&responses](const sse::Message& msg) {
        // LLM responses often use "event: message" or custom event types
        if (msg.event == "message" || msg.event == "delta") {
            responses.push_back(msg.data);
        }
    });

    // Simulate LLM streaming with partial chunks
    parser.parse("event: delta\ndata: The ", 22);
    parser.parse("quick brown ", 12);
    parser.parse("fox jumps\n\n", 12);  // Complete message

    parser.parse("event: delta\ndata: over ", 21);
    parser.parse("the lazy ", 9);
    parser.parse("dog\n\n", 5);  // Complete message

    parser.flush();  // Ensure any buffered data is delivered

    std::cout << "Received " << responses.size() << " responses:\n";
    for (size_t i = 0; i < responses.size(); ++i) {
        std::cout << "  [" << i << "] " << responses[i] << "\n";
    }
    std::cout << "\n";
}

// Example 3: Error handling
// Demonstrates proper error handling and recovery
void example_error_handling() {
    std::cout << "=== Example 3: Error Handling ===\n";

    int message_count = 0;
    sse::SseError last_error = sse::SseError::success;

    sse::SseParser parser([&message_count](const sse::Message& msg) {
        ++message_count;
        std::cout << "Message #" << message_count << ": " << msg.data << "\n";
    });

    // Valid data
    sse::SseError err = parser.parse("data: valid\n\n", 14);
    if (err != sse::SseError::success) {
        std::cout << "Error: " << sse::error_message(err) << "\n";
    }

    // Invalid retry value (should fail)
    err = parser.parse("retry: not_a_number\n\n", 21);
    if (err != sse::SseError::success) {
        std::cout << "Expected error: " << sse::error_message(err) << "\n";
    }

    // Parser continues working after error
    err = parser.parse("data: after error\n\n", 20);
    if (err != sse::SseError::success) {
        std::cout << "Unexpected error: " << sse::error_message(err) << "\n";
    }

    std::cout << "Total messages: " << message_count << "\n\n";
}

// Example 4: Connection reset/reconnect
// Shows proper reset usage for reconnection scenarios
void example_reconnection() {
    std::cout << "=== Example 4: Reconnection ===\n";

    std::string last_id;

    sse::SseParser parser([&last_id](const sse::Message& msg) {
        std::cout << "Received: " << msg.data << "\n";
        if (msg.id.has_value()) {
            last_id = msg.id.value();
        }
    });

    // Simulate first connection
    parser.parse("id: 1\ndata: First message\n\n", 28);
    parser.parse("id: 2\ndata: Second message\n\n", 29);

    std::cout << "Last event ID: " << last_id << "\n";

    // Connection drops, simulate reconnect
    parser.reset();  // Clear state but keep callback

    // New connection (would include Last-Event-ID header in real HTTP)
    parser.parse("id: 3\ndata: After reconnect\n\n", 29);

    std::cout << "New last event ID: " << last_id << "\n\n";
}

// Example 5: Batch processing
// Demonstrates processing multiple SSE streams efficiently
void example_batch_processing() {
    std::cout << "=== Example 5: Batch Processing ===\n";

    // Process multiple streams with function pointer callback
    // This is more efficient than std::function for high-throughput scenarios

    struct StreamStats {
        int message_count = 0;
        size_t total_bytes = 0;
    };

    auto callback = [](const sse::Message* msg, void* user_data) {
        auto* stats = static_cast<StreamStats*>(user_data);
        ++stats->message_count;
        stats->total_bytes += msg->data.size();
    };

    // Stream 1
    StreamStats stats1;
    sse::SseParser parser1;
    parser1.set_callback(callback, &stats1);
    parser1.parse("data: Stream 1, Msg 1\n\ndata: Stream 1, Msg 2\n\n", 46);
    parser1.flush();

    // Stream 2
    StreamStats stats2;
    sse::SseParser parser2;
    parser2.set_callback(callback, &stats2);
    parser2.parse("data: Stream 2, Msg 1\n\n", 25);
    parser2.flush();

    std::cout << "Stream 1: " << stats1.message_count << " messages, "
              << stats1.total_bytes << " bytes\n";
    std::cout << "Stream 2: " << stats2.message_count << " messages, "
              << stats2.total_bytes << " bytes\n\n";
}

// Example 6: Using string_view interface
// Demonstrates zero-copy parsing with string_view
void example_string_view() {
    std::cout << "=== Example 6: String View Interface ===\n";

    std::vector<std::string> events;

    sse::SseParser parser([&events](const sse::Message& msg) {
        events.push_back(msg.event + ": " + msg.data);
    });

    // Parse using string_view - no data copy
    std::string_view data = "event: user_login\ndata: John Doe\n\n"
                            "event: user_logout\ndata: Jane Smith\n\n";

    parser.parse(data);  // Zero-copy parse
    parser.flush();

    for (const auto& event : events) {
        std::cout << event << "\n";
    }
    std::cout << "\n";
}

// Example 7: Custom event types
// Shows handling of different event types
void example_custom_events() {
    std::cout << "=== Example 7: Custom Event Types ===\n";

    sse::SseParser parser([](const sse::Message& msg) {
        if (msg.event == "notification") {
            std::cout << "[NOTIFY] " << msg.data << "\n";
        } else if (msg.event == "error") {
            std::cout << "[ERROR] " << msg.data << "\n";
        } else if (msg.event == "ping") {
            std::cout << "[PING] (heartbeat)\n";
        } else {
            std::cout << "[" << msg.event << "] " << msg.data << "\n";
        }
    });

    const char* events =
        "event: notification\ndata: New message received\n\n"
        "event: ping\ndata: \n\n"
        "event: error\ndata: Connection timeout\n\n"
        "data: Default event type (message)\n\n";

    parser.parse(events, strlen(events));

    std::cout << "\n";
}

// Example 8: BOM handling
// Demonstrates UTF-8 BOM detection and skipping
void example_bom_handling() {
    std::cout << "=== Example 8: BOM Handling ===\n";

    sse::SseParser parser([](const sse::Message& msg) {
        std::cout << "Data: " << msg.data << "\n";
    });

    // Data with UTF-8 BOM at start
    unsigned char bom_data[] = {
        0xEF, 0xBB, 0xBF,  // UTF-8 BOM
        'd', 'a', 't', 'a', ':', ' ',
        'H', 'e', 'l', 'l', 'o', '\n', '\n', 0
    };

    // Skip BOM before parsing
    std::string_view data_view(reinterpret_cast<char*>(bom_data));
    if (sse::has_bom(data_view)) {
        std::cout << "BOM detected, skipping...\n";
        sse::skip_bom(data_view);
    }

    parser.parse(data_view);

    std::cout << "\n";
}

// Main entry point - runs all examples
int main() {
    std::cout << "SSE Parser Library Usage Examples\n";
    std::cout << "=================================\n\n";

    example_basic_callback();
    example_llm_streaming();
    example_error_handling();
    example_reconnection();
    example_batch_processing();
    example_string_view();
    example_custom_events();
    example_bom_handling();

    std::cout << "All examples completed successfully!\n";
    return 0;
}
