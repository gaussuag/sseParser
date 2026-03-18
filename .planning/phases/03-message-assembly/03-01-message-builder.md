---
plan: 03-01
phase: 3
wave: 1
autonomous: true
objective: Create MessageBuilder class that assembles parsed fields into complete SSE messages, handling multi-line data accumulation (DAT-01) and empty messages (DAT-02)
req_ids: DAT-01, DAT-02
---

# Plan 03-01: Message Builder Core

## Context

Phase 3 builds upon the completed Phase 2 (field parsing) to assemble complete SSE messages. The MessageBuilder accumulates parsed fields until a message boundary (blank line) is detected, then delivers the assembled message.

**Key Requirements:**
- **DAT-01**: Multi-line data accumulation - multiple `data:` lines accumulate with `\n` separator
- **DAT-02**: Empty message handling - blank lines with no fields produce empty messages

**Dependencies:**
- `include/sse_parser/buffer.h` - Buffer class for data accumulation
- `include/sse_parser/message.h` - Message struct for assembled messages
- `include/sse_parser/field_parser.h` - parse_field_line() for field parsing
- `include/sse_parser/error_codes.h` - SseError for error reporting

## Read First (MANDATORY)

Before implementation, read these files to understand existing patterns:

<read_first>
  <file>include/sse_parser/message.h</file>
  <file>include/sse_parser/field_parser.h</file>
  <file>include/sse_parser/buffer.h</file>
  <file>.planning/REQUIREMENTS.md</file>
</read_first>

## Technical Design

### MessageBuilder Interface

```cpp
#pragma once

#include <functional>
#include <optional>
#include <string_view>
#include <queue>
#include "message.h"
#include "error_codes.h"

namespace sse {

class MessageBuilder {
public:
    using MessageCallback = std::function<void(const Message&)>;

    MessageBuilder() = default;

    // Set callback for completed messages
    void set_callback(MessageCallback callback);

    // Feed a line to the builder (returns error if parsing fails)
    SseError feed_line(std::string_view line);

    // Get next completed message from internal queue (for testing)
    std::optional<Message> get_message();

    // Check if there are completed messages
    bool has_message() const noexcept;

    // Access current (incomplete) message state
    const Message& current_message() const noexcept;

    // Clear all state
    void reset() noexcept;

private:
    Message current_msg_;
    MessageCallback callback_;
    std::queue<Message> completed_messages_;
    bool accumulating_data_ = false;

    void complete_message();
    SseError process_field(std::string_view line);
};

} // namespace sse
```

### Implementation Details

**Multi-line Data (DAT-01):**
- Track `accumulating_data_` flag
- First `data:` field: `current_msg_.data = value`
- Subsequent `data:` fields: `current_msg_.data += "\n" + value`
- Empty `data:` adds `\n` separator only

**Empty Messages (DAT-02):**
- Blank line with no fields: deliver message with empty data
- `Message.empty()` returns true, but it's still a valid message
- Must trigger callback / add to queue

**Message Boundaries:**
- Detect blank lines (empty string after read_line())
- Trigger `complete_message()` to deliver
- Reset `current_msg_` and `accumulating_data_` for next message

## Acceptance Criteria

<acceptance_criteria>
  <criterion>File exists: include/sse_parser/message_builder.h</criterion>
  <criterion>grep -n "class MessageBuilder" include/sse_parser/message_builder.h</criterion>
  <criterion>grep -n "feed_line" include/sse_parser/message_builder.h</criterion>
  <criterion>grep -n "complete_message" include/sse_parser/message_builder.h</criterion>
  <criterion>grep -n "accumulating_data_" include/sse_parser/message_builder.h</criterion>
  <criterion>grep -n "DAT-01\|DAT-02" include/sse_parser/message_builder.h</criterion>
</acceptance_criteria>

## Implementation Actions

<action>
  <type>create</type>
  <file>include/sse_parser/message_builder.h</file>
  <description>MessageBuilder class with multi-line data support and empty message handling</description>
  <template>
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
    }

private:
    Message current_msg_;
    MessageCallback callback_;
    std::queue<Message> completed_messages_;
    bool accumulating_data_ = false;

    void complete_message() {
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
  </template>
</action>

## Verification Checklist

- [ ] Header compiles with C++17
- [ ] Multi-line data accumulates with `\n` separator (DAT-01)
- [ ] Empty messages delivered correctly (DAT-02)
- [ ] Comment lines handled (fields ignored)
- [ ] Non-data fields (event, id, retry) set correctly
- [ ] Callback invoked when set
- [ ] get_message() returns messages in FIFO order
- [ ] reset() clears current state but preserves delivered messages

## Dependencies

- Requires Phase 1: Buffer, Message, Error codes
- Requires Phase 2: field_parser
- Required by: Phase 3 integration tests

## Traceability

| Requirement | Implementation | Tests |
|-------------|---------------|-------|
| DAT-01 | Multi-line data with `\n` separator | TBD in 03-03 |
| DAT-02 | Empty line triggers complete_message() | TBD in 03-03 |
