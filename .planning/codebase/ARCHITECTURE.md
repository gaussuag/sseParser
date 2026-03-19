# Architecture

## Design Pattern

**Layered Architecture with Facade Pattern**

The SSE Parser follows a layered design where each layer has a single responsibility:

```
┌─────────────────────────────────────────────────────┐
│  Public API Layer (sse_parser_facade.h)            │
│  - SseParser class                                  │
│  - User-facing interface                            │
│  - Callback management                              │
└─────────────────────────────────────────────────────┘
                         │
┌─────────────────────────────────────────────────────┐
│  Message Assembly Layer (message_builder.h)        │
│  - MessageBuilder class                             │
│  - Multi-line data accumulation                     │
│  - Last-Event-ID tracking                           │
└─────────────────────────────────────────────────────┘
                         │
┌─────────────────────────────────────────────────────┐
│  Field Parsing Layer (field_parser.h)              │
│  - parse_field_line()                               │
│  - parse_retry_value()                              │
│  - BOM detection                                    │
└─────────────────────────────────────────────────────┘
                         │
┌─────────────────────────────────────────────────────┐
│  Buffer Management Layer (buffer.h)                │
│  - Buffer class                                     │
│  - Line-based reading                               │
│  - Automatic compaction                             │
└─────────────────────────────────────────────────────┘
                         │
┌─────────────────────────────────────────────────────┐
│  Data Model Layer                                   │
│  - Message struct (message.h)                       │
│  - SseError enum (error_codes.h)                    │
└─────────────────────────────────────────────────────┘
```

## Component Responsibilities

### 1. SseParser (Facade)
**File**: `include/sse_parser/sse_parser_facade.h`

- High-level interface for users
- Manages Buffer and MessageBuilder instances
- Provides multiple callback types (std::function, function pointer)
- Handles chunked input scenarios
- Implements flush() for incomplete messages
- Supports reset() for reconnection

### 2. MessageBuilder
**File**: `include/sse_parser/message_builder.h`

- Accumulates field lines into complete messages
- Handles multi-line data fields (DAT-01)
- Tracks last event ID for reconnection (EXT-02)
- Delivers messages via callback or queue
- Detects message boundaries (empty lines)

### 3. Field Parser
**File**: `include/sse_parser/field_parser.h`

- Parses individual SSE field lines
- Validates retry values with overflow protection
- Handles leading space removal (PAR-03)
- Skips comment lines
- UTF-8 BOM detection and skipping (EXT-01)

### 4. Buffer
**File**: `include/sse_parser/buffer.h`

- Dynamic buffer with configurable max size (default 4KB)
- Line-based reading with automatic compaction
- Supports LF, CRLF, CR line endings
- Overflow protection
- Zero-copy string_view interface

### 5. Data Structures

**Message** (`include/sse_parser/message.h`):
```cpp
struct Message {
    std::string event = "message";
    std::string data;
    std::optional<std::string> id;
    std::optional<int> retry;
};
```

**SseError** (`include/sse_parser/error_codes.h`):
```cpp
enum class SseError : uint8_t {
    success = 0,
    incomplete_message,
    invalid_retry,
    out_of_memory,
    buffer_overflow
};
```

## Data Flow

```
Raw SSE Stream
      │
      ▼
┌─────────────┐
│   Buffer    │ ← Accumulates chunks, reads lines
└─────────────┘
      │
      │ string_view lines
      ▼
┌─────────────┐
│Field Parser │ ← Parses field:value pairs
└─────────────┘
      │
      │ Populates Message fields
      ▼
┌─────────────┐
│  Message    │ ← Accumulates multi-line data
│   Builder   │ ← Tracks Last-Event-ID
└─────────────┘
      │
      │ Complete Message
      ▼
┌─────────────┐
│   Callback  │ ← User-defined handler
└─────────────┘
```

## Key Abstractions

### Zero-Copy Design
- Buffer uses `std::string` internally but returns `std::string_view` for lines
- No copying of data during parsing until Message fields are populated
- Efficient for high-throughput streaming

### Error Handling
- No exceptions - all errors returned as `SseError` enum
- Boolean NOT operator for natural error checking: `if (!parse(data, len))`
- Human-readable error messages via `error_message()` function

### Callback Flexibility
- Two callback types for different use cases:
  1. `std::function<void(const Message&)>` - Flexible, modern C++
  2. `void(*)(const Message*, void*)` - Zero-overhead, embedded-friendly

### Header-Only Design
- All implementation in headers for easy integration
- Templates and inline functions
- Single include: `#include "sse_parser/sse_parser.h"`

## Thread Safety

**Not thread-safe** by design:
- Each parser instance should be used by a single thread
- For multi-threading, use separate parser instances per thread
- External synchronization needed if shared

## Extension Points

- Custom buffer sizes via `Buffer(size_t max_size)`
- Custom callbacks via `set_callback()`
- Error handling via return codes
