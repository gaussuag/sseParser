# Research: C++ SSE Parser Stack

## Overview

Header-only C++17 SSE parser for desktop and embedded systems. Zero dynamic allocation in hot path.

## Recommended Stack

### Core Language Features

| Component | Feature | Rationale | Confidence |
|-----------|---------|-----------|------------|
| Memory Management | `std::array` + `std::string_view` | Stack allocation, zero-copy views | High |
| Callback Interface | `std::function` or function pointers | Flexibility vs performance tradeoff | High |
| Error Handling | `std::error_code` / enum class | Embedded-friendly, no exceptions | High |
| Optional Values | `std::optional` (C++17) | Clean optional field handling | High |
| String View | `std::string_view` (C++17) | Zero-copy input handling | High |

### Buffer Strategy

**Fixed Ring Buffer (Recommended)**
```cpp
static constexpr size_t BUFFER_SIZE = 4096;  // or configurable
template<size_t N>
class RingBuffer { /* ... */ };
```

- **Pros**: Predictable memory, no heap fragmentation
- **Cons**: Maximum message size limit
- **Use when**: Embedded systems, bounded workloads

**Dynamic Vector (Alternative)**
```cpp
std::vector<char> buffer_;
```

- **Pros**: Handles arbitrary message sizes
- **Cons**: Heap allocation, potential fragmentation
- **Use when**: Unknown message sizes, desktop systems

### What NOT to Use

| Approach | Why Not | Alternative |
|----------|---------|-------------|
| `std::stringstream` | High overhead, allocates internally | State machine parser |
| Regular expressions | Massive overhead for simple protocol | State machine |
| Exceptions | Embedded systems often disable exceptions | Error codes |
| Dynamic polymorphism (virtual) | Vtable overhead unnecessary | CRTP or callbacks |
| `std::string` for internal storage | Forces allocation | `RingBuffer<char>` |

## Compiler Compatibility

| Compiler | Minimum Version | Notes |
|----------|----------------|-------|
| GCC | 7.0 | Full C++17 support |
| Clang | 6.0 | Full C++17 support |
| MSVC | 2017 (19.14) | `/std:c++17` flag |
| Embedded GCC | 9.0+ | For ARM Cortex-M |

## Dependencies

**Allowed:**
- STL (containers, algorithms, string_view)
- Standard library error codes

**Not Allowed:**
- External libraries (Boost, etc.)
- Platform-specific APIs
- Threading primitives (user handles concurrency)

## Recommendations

1. **Use `std::string_view` for all input** — zero-copy interface
2. **Fixed-size ring buffer for internal storage** — predictability
3. **State machine parser** — no regex/stream overhead
4. **CRTP for static polymorphism** — if extensibility needed
5. **Enum class for error codes** — type-safe error handling

## Performance Targets

- Parse throughput: >100MB/s on modern hardware
- Memory overhead: <8KB per parser instance
- Zero allocations in parse hot path
- Cache-friendly sequential access
