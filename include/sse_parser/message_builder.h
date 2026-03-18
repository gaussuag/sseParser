/**
 * @file message_builder.h
 * @brief Message assembly from field lines
 * @ingroup Core
 *
 * Accumulates field lines into complete messages.
 * Handles multi-line data accumulation and Last-Event-ID tracking.
 */

#pragma once

#include <functional>
#include <optional>
#include <queue>
#include <string_view>
#include "error_codes.h"
#include "field_parser.h"
#include "message.h"

namespace sse {

/**
 * @class MessageBuilder
 * @brief Assembles SSE field lines into complete Message objects
 *
 * Accumulates field lines until a blank line (message boundary) is encountered,
 * then delivers the complete message via callback or queue.
 *
 * Features:
 * - Multi-line data accumulation (DAT-01)
 * - Empty message delivery (DAT-02)
 * - Last-Event-ID tracking (EXT-02)
 * - Callback and queue-based message retrieval
 *
 * @code
 * MessageBuilder builder;
 * builder.set_callback([](const Message& msg) {
 *     std::cout << "Received: " << msg.data << "\n";
 * });
 *
 * builder.feed_line("data: hello");
 * builder.feed_line("");  // Triggers callback
 * @endcode
 */
class MessageBuilder {
public:
    /**
     * @brief Callback type for message delivery
     *
     * Called synchronously when a message is complete.
     * Message reference is valid only during the callback.
     */
    using MessageCallback = std::function<void(const Message&)>;

    /**
     * @brief Default constructor
     */
    MessageBuilder() = default;

    /**
     * @brief Set callback for message delivery
     * @param callback Function to call when message is complete
     *
     * Callback is called synchronously when a blank line is encountered.
     * Set to nullptr to disable callback delivery.
     */
    void set_callback(MessageCallback callback) {
        callback_ = std::move(callback);
    }

    /**
     * @brief Feed a single field line to the builder
     * @param line The field line to process
     * @return SseError::success or SseError::invalid_retry
     *
     * Empty line triggers message completion and delivery.
     * Handles multi-line data fields and unknown fields.
     */
    SseError feed_line(std::string_view line) {
        // Check for blank line (message boundary)
        if (line.empty()) {
            complete_message();
            return SseError::success;
        }

        // Parse field and accumulate
        return process_field(line);
    }

    /**
     * @brief Retrieve and remove next completed message from queue
     * @return Message if available, std::nullopt if queue empty
     *
     * Use this method for pull-based message retrieval.
     * Queue is populated even if callback is set.
     */
    std::optional<Message> get_message() {
        if (completed_messages_.empty()) {
            return std::nullopt;
        }
        Message msg = std::move(completed_messages_.front());
        completed_messages_.pop();
        return msg;
    }

    /**
     * @brief Check if any completed messages are available
     * @return true if message queue is not empty
     */
    bool has_message() const noexcept {
        return !completed_messages_.empty();
    }

    /**
     * @brief Get reference to message currently being built
     * @return const reference to in-progress message
     *
     * Useful for inspecting partial message state.
     * Reference becomes invalid after next feed_line call.
     */
    const Message& current_message() const noexcept {
        return current_msg_;
    }

    /**
     * @brief Reset builder state for new stream or reconnection
     *
     * Clears current message being built but preserves:
     * - Completed messages in queue (already delivered)
     * - Last-Event-ID for reconnection (EXT-02)
     * - Callback registration
     */
    void reset() noexcept {
        current_msg_.clear();
        accumulating_data_ = false;
        // Note: completed_messages_ is NOT cleared - already delivered messages remain
        // Note: last_event_id_ is NOT cleared - it's cross-message state for reconnection
    }

    /**
     * @brief Get last seen event ID (EXT-02)
     * @return const reference to last event ID string
     *
     * Returns empty string if no ID has been received.
     * Used for EventSource reconnection with Last-Event-ID header.
     */
    const std::string& last_event_id() const noexcept {
        return last_event_id_;
    }

    /**
     * @brief Clear the last event ID tracking
     *
     * Call when establishing a new connection to reset ID tracking.
     */
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
