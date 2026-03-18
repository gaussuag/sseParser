/**
 * @mainpage SSE Parser Library
 * @brief C++17 Header-Only Server-Sent Events Parser
 *
 * High-performance, header-only C++17 library for parsing
 * Server-Sent Events (SSE) streams. Zero dependencies.
 *
 * @version 1.0.0
 * @date 2026-03-18
 */

#pragma once

/**
 * @defgroup Versioning Version Information
 * @{
 */

/**
 * @def SSE_PARSER_VERSION_MAJOR
 * @brief Major version number
 */
#define SSE_PARSER_VERSION_MAJOR 1

/**
 * @def SSE_PARSER_VERSION_MINOR
 * @brief Minor version number
 */
#define SSE_PARSER_VERSION_MINOR 0

/**
 * @def SSE_PARSER_VERSION_PATCH
 * @brief Patch version number
 */
#define SSE_PARSER_VERSION_PATCH 0

/** @} */

// Core components
#include "error_codes.h"
#include "message.h"
#include "buffer.h"
#include "field_parser.h"

// Public API facade
#include "sse_parser_facade.h"

/**
 * @brief Convenience namespace alias for backward compatibility
 *
 * @deprecated Use `sse` namespace directly. This alias is provided
 * for backward compatibility with code written before namespace
 * was simplified from `sse_parser` to `sse`.
 */
namespace sse_parser = sse;
