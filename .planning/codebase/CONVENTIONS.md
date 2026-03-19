# Coding Conventions

## Naming Conventions

### Types
- **Classes**: PascalCase
  - `SseParser`, `MessageBuilder`, `Buffer`
- **Structs**: PascalCase
  - `Message`
- **Enums**: PascalCase with values in snake_case
  - `enum class SseError { success, invalid_retry }`
- **Type Aliases**: PascalCase with Callback suffix
  - `MessageCallback`, `MessageCallbackFn`

### Functions
- **Public API**: snake_case
  - `parse()`, `flush()`, `reset()`, `set_callback()`
- **Private methods**: snake_case with trailing underscore (not used in headers)
- **Free functions**: snake_case
  - `error_message()`, `parse_field_line()`, `has_bom()`

### Variables
- **Class members**: snake_case with trailing underscore
  - `buffer_`, `callback_`, `read_pos_`
- **Local variables**: snake_case
  - `line_end`, `field_value`
- **Parameters**: snake_case
  - `data`, `len`, `max_size`
- **Static constants**: UPPER_CASE (not used)

### Namespaces
- **Primary**: `sse`
- **Deprecated alias**: `sse_parser` (backward compatibility)

## Code Style

### Formatting
- **Indentation**: 4 spaces (no tabs)
- **Braces**: Allman style (opening brace on new line)
- **Line length**: ~100 characters
- **Comments**: Doxygen Javadoc style (`/** */` and `///`)

Example:
```cpp
/**
 * @brief Parse raw bytes from SSE stream
 * @param data Pointer to byte buffer
 * @param len Number of bytes in buffer
 * @return SseError::success or error code
 */
inline SseError parse(const char* data, size_t len) {
    if (len == 0) {
        return SseError::success;
    }
    // ...
}
```

### Header Guards
- **Pragma once**: Used instead of include guards
  ```cpp
  #pragma once
  ```

### Includes
- **Order**: System headers first, then local headers
- **Style**: Angle brackets for system, quotes for local
  ```cpp
  #include <functional>
  #include <string>
  #include "buffer.h"
  #include "error_codes.h"
  ```

### Templates & Inline
- **Header-only**: All functions inline in headers
- **Inline keyword**: Used for all function definitions in headers
  ```cpp
  inline SseError parse(const char* data, size_t len) { ... }
  ```

## Error Handling

### No Exceptions Policy
- All errors returned via `SseError` enum
- Boolean NOT operator for natural checking:
  ```cpp
  if (!parse(data, len)) { /* handle error */ }
  ```

### Error Codes
```cpp
enum class SseError : uint8_t {
    success = 0,
    incomplete_message,
    invalid_retry,
    out_of_memory,
    buffer_overflow
};
```

### Error Messages
- Human-readable conversion via `error_message()`:
  ```cpp
  inline const char* error_message(SseError error) noexcept {
      switch (error) {
          case SseError::success: return "Success";
          case SseError::invalid_retry: return "Invalid retry value";
          // ...
      }
  }
  ```

## Design Patterns

### RAII
- No manual resource management needed (header-only, no allocations in core)
- Buffer uses `std::string` internally (automatic memory management)

### Zero-Copy Where Possible
- Return `std::string_view` from buffer operations
- Only copy when populating Message fields

### Callback Pattern
- Two callback types for flexibility:
  1. `std::function<void(const Message&)>` - Modern C++
  2. `void(*)(const Message*, void*)` - C-compatible, zero-overhead

### Facade Pattern
- `SseParser` provides simplified interface over complex subsystem
- Hides Buffer, MessageBuilder, field parsing details

## Documentation Standards

### Doxygen Comments
- **File headers**: `@file`, `@brief`, `@ingroup`
- **Classes**: `@class`, `@brief`, detailed description
- **Functions**: `@brief`, `@param`, `@return`, `@code/@endcode` examples
- **Member variables**: Inline `//` or `@var` in class docs

Example:
```cpp
/**
 * @class SseParser
 * @brief Main parser class for Server-Sent Events streams
 *
 * High-level facade combining Buffer, MessageBuilder, and field parsing
 * into a simple, easy-to-use interface.
 */
```

### Code Examples
- All public APIs include `@code/@endcode` examples
- Examples compiled and tested in `examples/usage_examples.cpp`

## C++ Features Used

### C++17 Features
- `std::string_view` - Zero-copy string references
- `std::optional` - Optional fields (id, retry)
- `std::function` - Callback storage
- `if constexpr` (not used, but available)
- `inline` variables (not needed in headers)

### STL Containers
- `std::string` - Buffer storage, message fields
- `std::queue<Message>` - Completed message queue
- `std::vector<Message>` - Test collectors

### Smart Pointers
- Not used (header-only library with simple ownership)

## Testing Conventions

### Test Structure
- **Files**: `tests/test_<component>.cpp`
- **Fixtures**: Structs for test helpers (e.g., `MessageCollector`)
- **Naming**: `TEST(CategoryTest, TestName)`
- **Categories**: Basic, StringView, Callback, Chunked, Flush, Reset, Integration, Edge

### Assertions
- `EXPECT_EQ` for equality
- `ASSERT_TRUE`/`EXPECT_TRUE` for booleans
- `ASSERT_TRUE(msg.id.has_value())` before dereferencing optional

Example:
```cpp
TEST(SseParserBasicTest, ParseSingleCompleteMessage) {
    MessageCollector collector;
    SseParser parser;
    parser.set_callback(std::ref(collector));
    
    SseError err = parser.parse("data: hello\n\n", 14);
    EXPECT_EQ(err, SseError::success);
    EXPECT_EQ(collector.count(), 1);
    EXPECT_EQ(collector.messages[0].data, "hello");
}
```

## Version Control

### Commit Messages
Follow conventional commits:
- `feat:` New features
- `fix:` Bug fixes
- `docs:` Documentation
- `test:` Tests
- `refactor:` Code refactoring
- `chore:` Maintenance

Example: `feat: add string_view interface for zero-copy parsing`
