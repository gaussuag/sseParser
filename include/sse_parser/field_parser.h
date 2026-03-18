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

#pragma once

#include <optional>
#include <string_view>
#include <climits>
#include <cctype>
#include "error_codes.h"
#include "message.h"

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
