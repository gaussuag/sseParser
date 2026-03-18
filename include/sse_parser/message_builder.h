#pragma once

#include <functional>
#include <optional>
#include <queue>
#include <string_view>
#include "error_codes.h"
#include "field_parser.h"
#include "message.h"

namespace sse {

class MessageBuilder {
public:
    using MessageCallback = std::function<void(const Message&)>;

    MessageBuilder() = default;

    void set_callback(MessageCallback callback) {
        callback_ = std::move(callback);
    }

    SseError feed_line(std::string_view line) {
        // Check for blank line (message boundary)
        if (line.empty()) {
            complete_message();
            return SseError::success;
        }

        // Parse field and accumulate
        return process_field(line);
    }

    std::optional<Message> get_message() {
        if (completed_messages_.empty()) {
            return std::nullopt;
        }
        Message msg = std::move(completed_messages_.front());
        completed_messages_.pop();
        return msg;
    }

    bool has_message() const noexcept {
        return !completed_messages_.empty();
    }

    const Message& current_message() const noexcept {
        return current_msg_;
    }

    void reset() noexcept {
        current_msg_.clear();
        accumulating_data_ = false;
        // Note: completed_messages_ is NOT cleared - already delivered messages remain
        // Note: last_event_id_ is NOT cleared - it's cross-message state for reconnection
    }

    // EXT-02: Last-Event-ID tracking
    const std::string& last_event_id() const noexcept {
        return last_event_id_;
    }

    void clear_last_event_id() noexcept {
        last_event_id_.clear();
    }

private:
    Message current_msg_;
    MessageCallback callback_;
    std::queue<Message> completed_messages_;
    bool accumulating_data_ = false;
    std::string last_event_id_;  // EXT-02: Track last event ID for reconnection

    void complete_message() {
        // EXT-02: Track last event ID
        if (current_msg_.id.has_value()) {
            last_event_id_ = *current_msg_.id;
        }

        // DAT-02: Even empty messages are delivered
        if (callback_) {
            callback_(current_msg_);
        }
        completed_messages_.push(std::move(current_msg_));
        current_msg_ = Message{};  // Reset with defaults
        accumulating_data_ = false;
    }

    SseError process_field(std::string_view line) {
        // Check if it's a data field first for multi-line handling
        if (line.size() >= 5 && line.substr(0, 5) == "data:") {
            std::string_view value = line.substr(5);
            // Remove exactly one leading space if present (PAR-03)
            if (!value.empty() && value[0] == ' ') {
                value = value.substr(1);
            }

            // DAT-01: Multi-line data accumulation
            if (accumulating_data_) {
                current_msg_.data += '\n';
                current_msg_.data += value;
            } else {
                current_msg_.data = std::string(value);
                accumulating_data_ = true;
            }
            return SseError::success;
        }

        // For non-data fields, use field_parser
        Message temp_msg;
        SseError err = parse_field_line(line, temp_msg);
        if (err != SseError::success) {
            return err;
        }

        // Copy fields to current message (data is already handled above)
        if (temp_msg.event != "message") {
            current_msg_.event = temp_msg.event;
        }
        if (temp_msg.id.has_value()) {
            current_msg_.id = temp_msg.id;
        }
        if (temp_msg.retry.has_value()) {
            current_msg_.retry = temp_msg.retry;
        }

        return SseError::success;
    }
};

} // namespace sse
