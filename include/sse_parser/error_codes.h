/**
 * @file error_codes.h
 * @brief Error codes and utilities for SSE parser
 * @ingroup Core
 *
 * Defines SseError enum and error message conversion.
 * All errors are reported via return codes (no exceptions).
 */

#pragma once

#include <cstdint>

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
