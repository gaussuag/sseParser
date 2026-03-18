/**
 * @file sse_parser_facade.h
 * @brief Main public API for SSE parsing
 * @ingroup PublicAPI
 *
 * Primary interface for parsing Server-Sent Events streams.
 * Supports both std::function and function pointer callbacks.
 *
 * @code
 * // Example usage:
 * SseParser parser([](const Message& msg) {
 *     std::cout << "Event: " << msg.event << "\n";
 *     std::cout << "Data: " << msg.data << "\n";
 * });
 *
 * parser.parse(data, length);
 * parser.flush();
 * @endcode
 */

#pragma once

#include <functional>
#include <string>
#include <string_view>

#include "buffer.h"
#include "error_codes.h"
#include "message.h"
#include "message_builder.h"

namespace sse {

/**
 * @class SseParser
 * @brief Main parser class for Server-Sent Events streams
 *
 * High-level facade combining Buffer, MessageBuilder, and field parsing
 * into a simple, easy-to-use interface.
 *
 * Features:
 * - Chunked input handling (API-01)
 * - Zero-copy string_view interface (API-02)
 * - Callback-based message delivery (API-03)
 * - Flush incomplete messages (API-04)
 * - Reset for reconnection (API-05)
 * - Function pointer support for zero overhead (IFC-01)
 *
 * Thread safety: Not thread-safe. Use external synchronization
 * or separate parser instances per thread.
 */
class SseParser {
public:
    /**
     * @brief Callback type for message delivery
     *
     * std::function based callback for flexibility.
     * Called synchronously when a complete message is parsed.
     */
    using MessageCallback = std::function<void(const Message&)>;

    /**
     * @brief Function pointer callback type (IFC-01)
     *
     * Zero-overhead callback for embedded systems.
     * Slightly faster than std::function, no allocation.
     *
     * @param msg Pointer to complete message (valid only during call)
     * @param user_data Opaque pointer passed through from set_callback
     */
    using MessageCallbackFn = void(*)(const Message* msg, void* user_data);

    /**
     * @brief Default constructor
     *
     * Creates parser without callback. Use set_callback() later.
     */
    SseParser() = default;

    /**
     * @brief Construct parser with callback
     * @param callback Function to call when messages are complete
     */
    explicit SseParser(MessageCallback callback) : callback_(std::move(callback)) {
        builder_.set_callback(callback_);
    }

    /**
     * @brief Parse raw bytes from SSE stream (API-01)
     * @param data Pointer to byte buffer
     * @param len Number of bytes in buffer
     * @return SseError::success or error code
     *
     * Handles partial messages - incomplete data is buffered
     * internally until complete message arrives.
     *
     * Callback is called synchronously for each complete message.
     */
    inline SseError parse(const char* data, size_t len) {
        if (len == 0) {
            return SseError::success;
        }

        SseError err = buffer_.append(data, len);
        if (err != SseError::success) {
            return err;
        }

        // Process all complete lines
        while (auto line = buffer_.read_line()) {
            err = builder_.feed_line(*line);
            if (err != SseError::success) {
                return err;
            }
        }

        return SseError::success;
    }

    /**
     * @brief Parse data from string_view (API-02)
     * @param data String view containing SSE data
     * @return SseError::success or error code
     *
     * Zero-copy wrapper around parse(const char*, size_t).
     * Data is not copied - view must remain valid for duration of call.
     */
    inline SseError parse(std::string_view data) {
        return parse(data.data(), data.size());
    }

    /**
     * @brief Set std::function callback (API-03)
     * @param callback Function to call for each complete message
     *
     * Replaces any existing callback. Clears function pointer callback if set.
     * Callback is called synchronously during parse() or flush().
     */
    inline void set_callback(MessageCallback callback) {
        callback_ = std::move(callback);
        builder_.set_callback(callback_);
        fn_callback_ = nullptr;  // Clear function pointer when std::function is set
        user_data_ = nullptr;
    }

    /**
     * @brief Set function pointer callback (IFC-01)
     * @param callback Function pointer for message delivery
     * @param user_data Opaque pointer passed to callback
     *
     * Zero-overhead callback option for embedded systems.
     * Slightly faster than std::function alternative.
     *
     * @code
     * void on_message(const Message* msg, void* ctx) {
     *     auto* handler = static_cast<MyHandler*>(ctx);
     *     handler->process(*msg);
     * }
     *
     * SseParser parser;
     * parser.set_callback(on_message, &my_handler);
     * @endcode
     */
    inline void set_callback(MessageCallbackFn callback, void* user_data) {
        fn_callback_ = callback;
        user_data_ = user_data;

        // Wrap function pointer in std::function for MessageBuilder compatibility
        if (callback) {
            builder_.set_callback([this](const Message& msg) {
                fn_callback_(&msg, user_data_);
            });
        } else {
            builder_.set_callback(nullptr);
        }
    }

    /**
     * @brief Flush any incomplete message at stream end (API-04)
     * @return SseError::success or error code
     *
     * Call when stream ends to deliver any buffered partial message.
     * Processes remaining complete lines, then handles final incomplete
     * line as a complete message (without requiring trailing newline).
     *
     * @code
     * // Stream ends without trailing newline
     * parser.parse("data: final", 11);
     * parser.flush();  // Delivers the message
     * @endcode
     */
    inline SseError flush() {
        // Case 1: No incomplete message → success, no callback
        if (buffer_.empty()) {
            return SseError::success;
        }

        // Case 2: Process any remaining complete lines first
        while (auto line = buffer_.read_line()) {
            SseError err = builder_.feed_line(*line);
            if (err != SseError::success) {
                return err;
            }
        }

        // Case 3: Handle incomplete final line (no trailing newline)
        auto leftover = buffer_.view();
        if (!leftover.empty()) {
            // Feed as final line without newline
            SseError err = builder_.feed_line(leftover);
            if (err != SseError::success) {
                return err;
            }
            // Force message completion
            builder_.feed_line("");  // Empty line triggers completion
        }

        buffer_.clear();
        return SseError::success;
    }

    /**
     * @brief Reset all state for reconnection scenarios (API-05)
     *
     * Clears internal buffer, message builder state, and last_event_id.
     * Preserves the registered callback.
     *
     * Call this when reconnecting to server with Last-Event-ID header.
     * Ensures clean state without losing callback registration.
     *
     * @code
     * // Connection dropped, reconnecting
     * parser.reset();
     * // Reconnect with last_event_id() for server
     * @endcode
     */
    inline void reset() noexcept {
        buffer_.clear();           // Clear internal buffer
        builder_.reset();          // Clear MessageBuilder state
        last_event_id_.clear();    // Reset last_event_id tracking
        // Note: callback_ is NOT cleared - keep registered callback
    }

    /**
     * @brief Get last seen event ID
     * @return const reference to last event ID string
     *
     * Returns empty string if no message with ID has been received.
     * Updated automatically when messages with id field are parsed.
     *
     * Use this value for EventSource Last-Event-ID header on reconnect.
     */
    inline const std::string& last_event_id() const {
        return last_event_id_;
    }

    /**
     * @brief Check if parser has buffered incomplete data
     * @return true if incomplete message is buffered
     *
     * Indicates whether flush() would deliver a message.
     * Useful for determining if stream ended cleanly.
     */
    inline bool has_incomplete_message() const {
        return builder_.has_message() || !buffer_.empty();
    }

private:
    Buffer buffer_;
    MessageBuilder builder_;
    MessageCallback callback_;
    std::string last_event_id_;

    // IFC-01: Function pointer callback members
    MessageCallbackFn fn_callback_ = nullptr;
    void* user_data_ = nullptr;
};

}  // namespace sse
