#pragma once

#include <optional>
#include <string_view>
#include "error_codes.h"
#include "message.h"

namespace sse {

// Parse a single SSE field line
// Format: "field: value" or "field:value" or "field" (empty value)
// Returns: success, or error if validation fails (e.g., invalid retry)
SseError parse_field_line(std::string_view line, Message& msg);

// Internal: Parse retry field value
// Validates: must be valid positive integer, checks overflow
SseError parse_retry_value(std::string_view value, int& result);

} // namespace sse
