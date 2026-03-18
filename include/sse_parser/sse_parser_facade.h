#pragma once

#include <functional>
#include <string>

#include "buffer.h"
#include "error_codes.h"
#include "message.h"
#include "message_builder.h"

namespace sse {

class SseParser {
public:
    using MessageCallback = std::function<void(const Message&)>;

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

    // API-03: set callback
    inline void set_callback(MessageCallback callback) {
        callback_ = std::move(callback);
        builder_.set_callback(callback_);
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
};

}  // namespace sse
