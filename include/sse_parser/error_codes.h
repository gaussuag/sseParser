#pragma once

#include <cstdint>

namespace sse_parser {

enum class SseError : uint8_t {
    success = 0,
    incomplete_message,
    invalid_retry,
    out_of_memory,
    buffer_overflow
};

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

inline bool operator!(SseError e) noexcept {
    return e == SseError::success;
}

}  // namespace sse_parser
