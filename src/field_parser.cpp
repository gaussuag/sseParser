#include "sse_parser/field_parser.h"
#include <charconv>
#include <cctype>

namespace sse {

SseError parse_field_line(std::string_view line, Message& msg) {
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

SseError parse_retry_value(std::string_view value, int& result) {
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

// Validate that retry value is non-negative (0 is valid per SSE spec)
SseError validate_retry(int value) {
    if (value < 0) {
        return SseError::invalid_retry;
    }
    return SseError::success;
}

} // namespace sse
