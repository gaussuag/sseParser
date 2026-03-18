/**
 * @file message.h
 * @brief SSE message structure
 * @ingroup Core
 *
 * Represents a complete Server-Sent Events message
 * with all standard fields per W3C spec.
 */

#pragma once

#include <optional>
#include <string>

namespace sse {

/**
 * @struct Message
 * @brief Complete SSE message with all standard fields
 *
 * Represents a single Server-Sent Events message as defined by
 * the W3C EventSource specification.
 *
 * @var Message::event Event type (default: "message")
 * @var Message::data Message payload data
 * @var Message::id Optional event ID for replay/resume
 * @var Message::retry Optional reconnection time in milliseconds
 *
 * @code
 * Message msg;
 * msg.event = "update";
 * msg.data = "Hello, World!";
 * msg.id = "12345";
 * msg.retry = 5000;
 * @endcode
 */
struct Message {
    std::string event = "message";
    std::string data;
    std::optional<std::string> id;
    std::optional<int> retry;

    /**
     * @brief Reset all fields to defaults
     *
     * Clears data and resets event to "message".
     * Optional fields are reset to empty state.
     */
    void clear() noexcept {
        event = "message";
        data.clear();
        id.reset();
        retry.reset();
    }

    /**
     * @brief Check if message has no data payload
     * @return true if data field is empty
     *
     * An empty message still triggers the callback and may
     * contain event type, id, or retry fields.
     */
    bool empty() const noexcept {
        return data.empty();
    }
};

}
