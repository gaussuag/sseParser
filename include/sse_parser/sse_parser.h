#pragma once

// Version info
#define SSE_PARSER_VERSION_MAJOR 1
#define SSE_PARSER_VERSION_MINOR 0
#define SSE_PARSER_VERSION_PATCH 0

// Core components
#include "error_codes.h"
#include "message.h"
#include "buffer.h"
#include "field_parser.h"

// Convenience namespace alias
namespace sse_parser = sse;
