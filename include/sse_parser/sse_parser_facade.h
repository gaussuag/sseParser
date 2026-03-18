#pragma once

#include <functional>
#include <string>
#include <string_view>

#include "buffer.h"
#include "error_codes.h"
#include "message.h"
#include "message_builder.h"

namespace sse {

class SseParser {
public:
    using MessageCallback = std::function<void(const Message&)>;

    // IFC-01: Function pointer callback for zero overhead
    using MessageCallbackFn = void(*)(const Message* msg, void* user_data);

    SseParser() = default;

    explicit SseParser(MessageCallback callback) : callback_(std::move(callback)) {
        builder_.set_callback(callback_);
    }

    // API-01: parse raw bytes
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

    // API-02: Zero-copy string_view interface
    inline SseError parse(std::string_view data) {
        return parse(data.data(), data.size());
    }

    // API-03: set callback
    inline void set_callback(MessageCallback callback) {
        callback_ = std::move(callback);
        builder_.set_callback(callback_);
        fn_callback_ = nullptr;  // Clear function pointer when std::function is set
        user_data_ = nullptr;
    }

    // IFC-01: Function pointer callback for zero overhead
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

    // API-04: Flush incomplete message at stream end
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

    // API-05: Reset all state for reconnection scenarios
    inline void reset() noexcept {
        buffer_.clear();           // Clear internal buffer
        builder_.reset();          // Clear MessageBuilder state
        last_event_id_.clear();    // Reset last_event_id tracking
        // Note: callback_ is NOT cleared - keep registered callback
    }

    // Utility accessors
    inline const std::string& last_event_id() const {
        return last_event_id_;
    }

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
