/**
 * @mainpage SSE Parser Library
 * @brief C++17 Header-Only Server-Sent Events Parser
 *
 * High-performance, header-only C++17 library for parsing
 * Server-Sent Events (SSE) streams. Zero dependencies.
 *
 * @version 2.0.0
 * @date 2026-03-20
 */

#pragma once

#define SSE_PARSER_VERSION_MAJOR 2
#define SSE_PARSER_VERSION_MINOR 0
#define SSE_PARSER_VERSION_PATCH 0

#include <cstddef>
#include <cstdint>
#include <cctype>
#include <climits>
#include <functional>
#include <optional>
#include <queue>
#include <string>
#include <string_view>

#pragma region Error Codes
/**
 * @file error_codes.h
 * @brief Error codes and utilities for SSE parser
 * @ingroup Core
 *
 * Defines SseError enum and error message conversion.
 * All errors are reported via return codes (no exceptions).
 */

namespace sse {

/**
 * @enum SseError
 * @brief Error codes returned by SSE parser operations
 *
 * All parser operations return SseError to indicate success or failure.
 * No exceptions are thrown - check return values.
 *
 * @var SseError::success Operation completed successfully
 * @var SseError::incomplete_message Message waiting for more data
 * @var SseError::invalid_retry Retry field contains invalid value
 * @var SseError::out_of_memory Memory allocation failed
 * @var SseError::buffer_overflow Input exceeds buffer capacity
 */
enum class SseError : uint8_t {
    success = 0,
    incomplete_message,
    invalid_retry,
    out_of_memory,
    buffer_overflow
};

/**
 * @brief Convert error code to human-readable string
 * @param error The error code to convert
 * @return const char* Human-readable error description
 *
 * @code
 * SseError err = parser.parse(data, len);
 * if (err != SseError::success) {
 *     std::cerr << "Parse error: " << error_message(err) << "\n";
 * }
 * @endcode
 */
inline const char* error_message(SseError error) noexcept {
    switch (error) {
        case SseError::success:
            return "Success";
        case SseError::incomplete_message:
            return "Message incomplete";
        case SseError::invalid_retry:
            return "Invalid retry value";
        case SseError::out_of_memory:
            return "Out of memory";
        case SseError::buffer_overflow:
            return "Buffer overflow";
    }
    return "Unknown error";
}

/**
 * @brief Boolean NOT operator for error checking
 * @param e Error code to check
 * @return true if error is NOT success (i.e., an error occurred)
 *
 * Enables natural error checking: if (!parse(data, len)) { handle_error(); }
 */
inline bool operator!(SseError e) noexcept {
    return e == SseError::success;
}

}  // namespace sse
#pragma endregion

#pragma region Message
/**
 * @file message.h
 * @brief SSE message structure
 * @ingroup Core
 *
 * Represents a complete Server-Sent Events message
 * with all standard fields per W3C spec.
 */

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
#pragma endregion

#pragma region Buffer
/**
 * @file buffer.h
 * @brief Dynamic buffer for SSE stream buffering
 * @ingroup Core
 *
 * Provides line-based reading with automatic compaction.
 * Supports configurable max size with overflow protection.
 */

namespace sse {

/**
 * @class Buffer
 * @brief Dynamic buffer for accumulating and reading SSE stream data
 *
 * Provides efficient line-based reading with automatic compaction
 * to maximize usable space. Supports LF, CRLF, and CR line endings.
 *
 * @code
 * Buffer buf(8192);  // 8KB max size
 * buf.append("data: hello\n\n", 13);
 * auto line = buf.read_line();  // "data: hello"
 * @endcode
 */
class Buffer {
public:
    /**
     * @brief Default constructor with 4KB max size
     */
    Buffer() = default;

    /**
     * @brief Construct buffer with custom max size
     * @param max_size Maximum bytes the buffer can hold
     */
    explicit Buffer(size_t max_size) : max_size_(max_size) {}

    /**
     * @brief Append raw bytes to buffer
     * @param data Pointer to data to append
     * @param len Number of bytes to append
     * @return SseError::success on success, SseError::buffer_overflow if full
     *
     * Automatically compacts buffer if needed to make room.
     */
    SseError append(const char* data, size_t len);

    /**
     * @brief Append string_view to buffer (zero-copy wrapper)
     * @param data String view to append
     * @return SseError indicating success or overflow
     */
    SseError append(std::string_view data);

    /**
     * @brief Read next complete line from buffer
     * @return string_view of line content, or nullopt if no complete line
     *
     * Handles LF, CRLF, and CR line endings. Returns view into internal
     * buffer - copy if persistence needed beyond next operation.
     */
    std::optional<std::string_view> read_line();

    /**
     * @brief Check if buffer contains a complete message (empty line)
     * @return true if an empty line (message boundary) exists
     */
    bool has_complete_message() const;

    /**
     * @brief Get current buffer size in bytes
     * @return Number of bytes stored
     */
    size_t size() const noexcept { return buffer_.size(); }

    /**
     * @brief Check if buffer is empty
     * @return true if no data stored
     */
    bool empty() const noexcept { return buffer_.empty(); }

    /**
     * @brief Clear all data and reset read position
     */
    void clear() noexcept;

    /**
     * @brief Get view of entire buffer contents
     * @return string_view of raw buffer data
     */
    std::string_view view() const { return buffer_; }

    /**
     * @brief Get maximum buffer size
     * @return Maximum bytes buffer can hold
     */
    size_t max_size() const noexcept { return max_size_; }

    /**
     * @brief Set maximum buffer size
     * @param max_size New maximum size in bytes
     */
    void set_max_size(size_t max_size) noexcept { max_size_ = max_size; }

private:
    std::string buffer_;
    size_t read_pos_ = 0;
    size_t max_size_ = 4096;

    /**
     * @brief Find position of next line ending
     * @param start Position to start searching from
     * @return Position of line ending, or npos if not found
     */
    size_t find_line_end(size_t start) const;

    /**
     * @brief Compact buffer by removing consumed data
     *
     * Moves unread data to beginning of buffer to maximize
     * available space for new data.
     */
    void compact();
};

inline SseError Buffer::append(const char* data, size_t len) {
    if (len == 0) {
        return SseError::success;
    }

    if (buffer_.size() + len > max_size_) {
        compact();
        if (buffer_.size() + len > max_size_) {
            return SseError::buffer_overflow;
        }
    }

    buffer_.append(data, len);
    return SseError::success;
}

inline SseError Buffer::append(std::string_view data) {
    return append(data.data(), data.size());
}

inline std::optional<std::string_view> Buffer::read_line() {
    if (read_pos_ >= buffer_.size()) {
        return std::nullopt;
    }

    size_t line_end = find_line_end(read_pos_);
    if (line_end == std::string::npos) {
        return std::nullopt;
    }

    size_t line_start = read_pos_;
    size_t line_length = line_end - line_start;

    read_pos_ = line_end;

    // Handle line endings: LF, CRLF, or CR
    if (read_pos_ < buffer_.size() && buffer_[read_pos_] == '\r') {
        ++read_pos_;  // Skip the CR
        // If this is CRLF, also skip the LF
        if (read_pos_ < buffer_.size() && buffer_[read_pos_] == '\n') {
            ++read_pos_;
        }
    } else if (read_pos_ < buffer_.size() && buffer_[read_pos_] == '\n') {
        ++read_pos_;  // Skip the LF
    }

    while (line_length > 0 && 
           (buffer_[line_start + line_length - 1] == '\r' ||
            buffer_[line_start + line_length - 1] == '\n')) {
        --line_length;
    }

    return std::string_view(buffer_.data() + line_start, line_length);
}

inline bool Buffer::has_complete_message() const {
    size_t pos = read_pos_;

    while (pos < buffer_.size()) {
        size_t line_end = find_line_end(pos);
        if (line_end == std::string::npos) {
            return false;
        }

        if (line_end == pos ||
            (line_end == pos + 1 && buffer_[pos] == '\r')) {
            return true;
        }

        pos = line_end;
        if (pos < buffer_.size() && buffer_[pos] == '\n') {
            ++pos;
        }
    }

    return false;
}

inline void Buffer::clear() noexcept {
    buffer_.clear();
    read_pos_ = 0;
}

inline size_t Buffer::find_line_end(size_t start) const {
    for (size_t i = start; i < buffer_.size(); ++i) {
        if (buffer_[i] == '\n') {
            return i;
        }
        if (buffer_[i] == '\r') {
            if (i + 1 < buffer_.size() && buffer_[i + 1] == '\n') {
                return i;
            }
            return i;
        }
    }
    return std::string::npos;
}

inline void Buffer::compact() {
    if (read_pos_ > 0) {
        buffer_.erase(0, read_pos_);
        read_pos_ = 0;
    }
}

}  // namespace sse
#pragma endregion

#pragma region Field Parser
/**
 * @file field_parser.h
 * @brief SSE field line parsing functions
 * @ingroup Parsing
 *
 * Parses SSE field lines per W3C spec including:
 * - Field:value format
 * - Leading space handling
 * - Comment skipping
 * - Retry validation
 * - UTF-8 BOM detection
 */

namespace sse {

/**
 * @brief Parse retry field value with overflow protection
 * @param value String containing retry value
 * @param result Output parameter for parsed integer
 * @return SseError::success on valid retry, SseError::invalid_retry on error
 *
 * Validates retry field per SSE spec:
 * - Must be valid positive integer
 * - Detects integer overflow
 * - Rejects negative numbers and non-digits
 *
 * @code
 * int retry_ms;
 * SseError err = parse_retry_value("5000", retry_ms);
 * // retry_ms == 5000, err == SseError::success
 * @endcode
 */
inline SseError parse_retry_value(std::string_view value, int& result) {
    if (value.empty()) {
        return SseError::invalid_retry;
    }
    
    // Trim leading whitespace
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    
    // Trim trailing whitespace
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    
    if (start >= end) {
        return SseError::invalid_retry;
    }
    
    std::string_view trimmed = value.substr(start, end - start);
    
    // Check for negative numbers
    if (trimmed[0] == '-') {
        return SseError::invalid_retry;
    }
    
    // Parse as integer
    int val = 0;
    for (char c : trimmed) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return SseError::invalid_retry;
        }
        
        // Check for overflow before multiplying
        if (val > (INT_MAX / 10)) {
            return SseError::invalid_retry;
        }
        val *= 10;
        
        int digit = c - '0';
        if (val > (INT_MAX - digit)) {
            return SseError::invalid_retry;
        }
        val += digit;
    }
    
    result = val;
    return SseError::success;
}

/**
 * @brief Parse a single SSE field line
 * @param line The field line to parse
 * @param msg Message struct to populate with parsed data
 * @return SseError::success on success, SseError::invalid_retry on bad retry value
 *
 * Parses field lines per W3C SSE spec:
 * - Format: "field: value" or "field:value" or "field" (empty value)
 * - Removes exactly one leading space after colon (PAR-03)
 * - Skips comment lines starting with ':'
 * - Handles event, data, id, retry fields
 * - Silently ignores unknown fields
 *
 * @code
 * Message msg;
 * SseError err = parse_field_line("data: hello", msg);
 * // msg.data == "hello"
 * @endcode
 */
inline SseError parse_field_line(std::string_view line, Message& msg) {
    // Skip comment lines (start with ':')
    if (!line.empty() && line[0] == ':') {
        return SseError::success;  // Comment, ignore
    }
    
    // Find colon separator
    size_t colon_pos = line.find(':');
    
    std::string_view field_name;
    std::string_view field_value;
    
    if (colon_pos == std::string_view::npos) {
        // No colon - field with empty value
        field_name = line;
        field_value = "";
    } else {
        field_name = line.substr(0, colon_pos);
        field_value = line.substr(colon_pos + 1);
        
        // Remove exactly one leading space if present (PAR-03)
        if (!field_value.empty() && field_value[0] == ' ') {
            field_value = field_value.substr(1);
        }
    }
    
    // Handle known fields (case-sensitive per SSE spec)
    if (field_name == "event") {
        msg.event = std::string(field_value);
    } else if (field_name == "data") {
        // In Phase 2, just set the data. Phase 3 will handle multi-line accumulation
        msg.data = std::string(field_value);
    } else if (field_name == "id") {
        msg.id = std::string(field_value);
    } else if (field_name == "retry") {
        int retry_val = 0;
        SseError err = parse_retry_value(field_value, retry_val);
        if (err != SseError::success) {
            return err;
        }
        msg.retry = retry_val;
    }
    // Unknown fields are silently ignored per SSE spec
    
    return SseError::success;
}

/**
 * @brief Check if data starts with UTF-8 BOM (EXT-01)
 * @param data String view to check
 * @return true if data starts with UTF-8 BOM (0xEF 0xBB 0xBF)
 *
 * UTF-8 BOM detection for stream initialization. SSE streams may
 * include BOM at the start which should be skipped.
 */
inline bool has_bom(std::string_view data) {
    return data.size() >= 3 &&
           static_cast<unsigned char>(data[0]) == 0xEF &&
           static_cast<unsigned char>(data[1]) == 0xBB &&
           static_cast<unsigned char>(data[2]) == 0xBF;
}

/**
 * @brief Check and skip UTF-8 BOM if present (EXT-01)
 * @param data String view to check and modify
 * @return true if BOM was found and skipped, false otherwise
 *
 * If BOM is detected, advances the view past the 3 BOM bytes.
 * Safe to call on any data - only modifies if BOM present.
 *
 * @code
 * std::string_view data = "\xEF\xBB\xBFdata: hello";
 * skip_bom(data);  // data now starts with "data: hello"
 * @endcode
 */
inline bool skip_bom(std::string_view& data) {
    if (has_bom(data)) {
        data = data.substr(3);
        return true;
    }
    return false;
}

} // namespace sse
#pragma endregion

#pragma region Message Builder
/**
 * @file message_builder.h
 * @brief Message assembly from field lines
 * @ingroup Core
 *
 * Accumulates field lines into complete messages.
 * Handles multi-line data accumulation and Last-Event-ID tracking.
 */

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
#pragma endregion

#pragma region Sse Parser Facade
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
#pragma endregion

namespace sse_parser = sse;
