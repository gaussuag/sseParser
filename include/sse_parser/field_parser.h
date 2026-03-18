#pragma once

#include <optional>
#include <string_view>
#include <climits>
#include <cctype>
#include "error_codes.h"
#include "message.h"

namespace sse {

// Internal: Parse retry field value
// Validates: must be valid positive integer, checks overflow
SseError parse_retry_value(std::string_view value, int& result);

// EXT-01: Check and skip UTF-8 BOM
// Returns: true if BOM was found and skipped, false otherwise
// If BOM found, the view is advanced past the BOM bytes
bool skip_bom(std::string_view& data);

// EXT-01: Check if data starts with UTF-8 BOM
bool has_bom(std::string_view data);

} // namespace sse
