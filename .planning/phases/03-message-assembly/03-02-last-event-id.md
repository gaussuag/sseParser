---
plan: 03-02
phase: 3
wave: 2
autonomous: true
objective: Implement Last-Event-ID tracking in MessageBuilder as required by EXT-02
req_ids: EXT-02
---

# Plan 03-02: Last-Event-ID Tracking

## Context

This plan extends the MessageBuilder from Plan 03-01 to track the last received event ID across messages, as required by **EXT-02**.

**Key Requirements:**
- **EXT-02**: Track the last seen `id` field value automatically
- Provide accessor method: `const std::string& last_event_id() const`
- Update on every message that contains an ID field
- Used for reconnection scenarios (client sends Last-Event-ID header)

**Usage Example:**
```
id: 1\n
data: first\n\n
id: 2\n
data: second\n\n
→ last_event_id() returns "2"
```

**Dependencies:**
- Plan 03-01 must be complete (MessageBuilder base class)
- `include/sse_parser/message.h` - Message struct with id field

## Read First (MANDATORY)

<read_first>
  <file>.planning/phases/03-message-assembly/03-01-message-builder.md</file>
  <file>include/sse_parser/message_builder.h</file>
  <file>include/sse_parser/message.h</file>
  <file>.planning/REQUIREMENTS.md</file>
</read_first>

## Technical Design

### Extended MessageBuilder Interface

Add to existing MessageBuilder class:

```cpp
// EXT-02: Last-Event-ID tracking
const std::string& last_event_id() const noexcept {
    return last_event_id_;
}

// Reset Last-Event-ID (for reconnection scenarios)
void clear_last_event_id() noexcept {
    last_event_id_.clear();
}
```

### Implementation Strategy

**ID Tracking:**
- Add `std::string last_event_id_` member
- In `complete_message()`, check if `current_msg_.id` has value
- If yes, update `last_event_id_ = *current_msg_.id`
- The ID persists across messages (only updates when new ID received)

**Edge Cases:**
- No ID field yet: `last_event_id()` returns empty string
- ID field with empty value: `last_event_id()` becomes empty string
- Multiple messages without ID: previous ID persists
- reset() should NOT clear last_event_id_ (it's cross-message state)

## Acceptance Criteria

<acceptance_criteria>
  <criterion>grep -n "last_event_id_" include/sse_parser/message_builder.h</criterion>
  <criterion>grep -n "const std::string& last_event_id" include/sse_parser/message_builder.h</criterion>
  <criterion>grep -n "clear_last_event_id" include/sse_parser/message_builder.h</criterion>
  <criterion>grep -n "EXT-02" include/sse_parser/message_builder.h</criterion>
</acceptance_criteria>

## Implementation Actions

<action>
  <type>edit</type>
  <file>include/sse_parser/message_builder.h</file>
  <description>Add Last-Event-ID tracking members and methods to MessageBuilder</description>
  <changes>
    // Add member variable after accumulating_data_:
    std::string last_event_id_;
    
    // Add public methods:
    const std::string& last_event_id() const noexcept {
        return last_event_id_;
    }
    
    void clear_last_event_id() noexcept {
        last_event_id_.clear();
    }
    
    // Modify complete_message() to update last_event_id_:
    void complete_message() {
        // EXT-02: Track last event ID
        if (current_msg_.id.has_value()) {
            last_event_id_ = *current_msg_.id;
        }
        
        // ... rest of existing complete_message() code
    }
  </changes>
</action>

## Verification Checklist

- [ ] `last_event_id()` returns empty string initially
- [ ] ID from first message is tracked
- [ ] ID updates when new message has different ID
- [ ] ID persists when message has no ID field
- [ ] `clear_last_event_id()` resets to empty
- [ ] ID with empty value clears tracking
- [ ] Tracking survives `reset()` call

## Dependencies

- Requires Plan 03-01: MessageBuilder core
- Required by: Phase 3 integration tests

## Traceability

| Requirement | Implementation | Tests |
|-------------|---------------|-------|
| EXT-02 | `last_event_id_` member, updated in `complete_message()` | TBD in 03-03 |
