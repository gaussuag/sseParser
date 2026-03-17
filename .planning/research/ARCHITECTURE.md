# Research: SSE Parser Architecture

## System Context

```
┌─────────────────┐     raw bytes      ┌─────────────────┐
│   User Network  │ ─────────────────→ │  SSE Parser     │
│   Implementation│                    │  (this library) │
└─────────────────┘                    └────────┬────────┘
                                                │
                                                │ callback
                                                ↓
                                         ┌──────────────┐
                                         │ User Handler │
                                         │ (process msg)│
                                         └──────────────┘
```

## Component Architecture

### High-Level Structure

```
SseParser (main class)
├── BufferManager (internal buffer)
├── StateMachine (parsing logic)
├── MessageBuilder (message assembly)
└── CallbackInterface (user-provided handler)
```

### Component Details

#### 1. SseParser (Facade)

**Responsibilities:**
- Public API entry point
- Owns all internal components
- Manages parser lifecycle

**Interface:**
```cpp
template<size_t BufferSize = 4096>
class SseParser {
public:
    using MessageCallback = std::function<void(const Message&)>;
    
    explicit SseParser(MessageCallback callback);
    
    // Main entry: feed data chunk
    std::error_code parse(const char* data, size_t len);
    std::error_code parse(std::string_view data);
    
    // Flush any pending partial message
    std::error_code flush();
    
    // Reset state (e.g., on reconnect)
    void reset();
    
private:
    RingBuffer<BufferSize> buffer_;
    ParseState state_;
    MessageBuilder builder_;
    MessageCallback callback_;
};
```

#### 2. BufferManager (RingBuffer)

**Responsibilities:**
- Store incoming bytes
- Provide sequential access to lines
- Handle overflow gracefully

**Key Design:**
```cpp
template<size_t N>
class RingBuffer {
    std::array<char, N> data_;
    size_t head_ = 0;  // write position
    size_t tail_ = 0;  // read position
    
public:
    bool append(std::string_view chunk);
    std::optional<std::string_view> read_line();
    bool full() const;
    void clear();
};
```

#### 3. StateMachine

**Responsibilities:**
- Parse field: value format
- Track message boundaries
- Dispatch to message builder

**States:**
```
START
├── READING_FIELD   → reading field name
├── READING_VALUE   → reading value after colon
├── AWAITING_EOL    → looking for end of line
├── AWAITING_EOM    → looking for second newline (end of message)
└── DISPATCH        → message complete, dispatch callback
```

#### 4. MessageBuilder

**Responsibilities:**
- Accumulate message fields
- Handle multi-line data
- Construct final Message object

**Structure:**
```cpp
struct Message {
    std::string event;  // defaults to "message"
    std::string data;
    std::optional<std::string> id;
    std::optional<int> retry;
};

class MessageBuilder {
    Message current_;
    bool has_data_ = false;
    
public:
    void add_field(std::string_view name, std::string_view value);
    void add_data_line(std::string_view value);
    Message finalize();
    void reset();
};
```

## Data Flow

```
1. User calls parse(chunk)
   ↓
2. BufferManager.append(chunk)
   ↓
3. While buffer has complete lines:
   ├── StateMachine.parse_line(line)
   │   ├── Extract field:value
   │   └── Update state
   └── If message complete:
       └── MessageBuilder.finalize()
           └── Invoke user callback
   ↓
4. Return error code (or success)
```

## Build Order (Implementation Sequence)

Based on dependencies:

**Phase 1: Foundation**
1. Error codes / result types
2. Message struct
3. RingBuffer (tests first)

**Phase 2: Core Parsing**
4. State machine (single line parsing)
5. Field extraction logic
6. Line reading from buffer

**Phase 3: Message Assembly**
7. MessageBuilder (accumulation)
8. Multi-line data handling
9. Message dispatch

**Phase 4: Public API**
10. SseParser facade
11. Callback integration
12. Flush/reset operations

**Phase 5: Polish**
13. Edge case handling
14. Performance optimization
15. Documentation

## Component Boundaries

| Component | Input | Output | Invariants |
|-----------|-------|--------|------------|
| RingBuffer | Raw bytes | Complete lines | FIFO, bounded size |
| StateMachine | Single line | Field events | Valid SSE syntax |
| MessageBuilder | Field events | Complete Message | Accumulates data |
| SseParser | Byte chunks | Callbacks | Orchestrates above |

## Memory Layout

```
SseParser<4096> instance (~4KB total):
├── RingBuffer: 4096 bytes (data storage)
├── StateMachine: ~32 bytes (state vars)
├── MessageBuilder: ~256 bytes (current message)
└── Vtable/callback: ~16 bytes
```

## Threading Model

**Single-threaded by design:**
- User serializes all `parse()` calls
- Callback executes synchronously in parse context
- No internal locks (user's responsibility)

**Usage pattern:**
```cpp
// Network thread
parser.parse(chunk);  // callback fires here
```

Or with explicit dispatch:
```cpp
// Network thread collects messages
std::vector<Message> queue;
parser.parse(chunk, [&](const Message& m) {
    queue.push_back(m);  // just queue
});

// Main thread processes
for (auto& m : queue) { process(m); }
```
