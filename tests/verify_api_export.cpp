/**
 * @file verify_api_export.cpp
 * @brief Verification that all public API symbols are accessible from sse_parser.hpp
 *
 * This file uses static_assert to verify compile-time accessibility of all public API
 * symbols from the single header. If this file compiles, all public API is accessible.
 */

#include "sse_parser/sse_parser.hpp"

// Verify classes are accessible and have valid sizes
static_assert(sizeof(sse::SseParser) > 0, "SseParser must be accessible");
static_assert(sizeof(sse::Message) > 0, "Message must be accessible");
static_assert(sizeof(sse::Buffer) > 0, "Buffer must be accessible");
static_assert(sizeof(sse::MessageBuilder) > 0, "MessageBuilder must be accessible");

// Verify enum is accessible
static_assert(static_cast<int>(sse::SseError::success) == 0, "SseError::success must be 0");
static_assert(static_cast<int>(sse::SseError::incomplete_message) == 1, "SseError incomplete_message");
static_assert(static_cast<int>(sse::SseError::invalid_retry) == 2, "SseError invalid_retry");
static_assert(static_cast<int>(sse::SseError::out_of_memory) == 3, "SseError out_of_memory");
static_assert(static_cast<int>(sse::SseError::buffer_overflow) == 4, "SseError buffer_overflow");

// Verify free functions are accessible (function pointers have non-zero size)
static_assert(sizeof(&sse::error_message) > 0, "error_message function must be accessible");
static_assert(sizeof(&sse::has_bom) > 0, "has_bom function must be accessible");
static_assert(sizeof(&sse::skip_bom) > 0, "skip_bom function must be accessible");
static_assert(sizeof(&sse::parse_field_line) > 0, "parse_field_line function must be accessible");
static_assert(sizeof(&sse::parse_retry_value) > 0, "parse_retry_value function must be accessible");

// Verify namespace alias works
static_assert(sizeof(sse_parser::SseParser) > 0, "sse_parser namespace alias must work");
static_assert(sizeof(sse_parser::Message) > 0, "sse_parser namespace alias must work");
static_assert(sizeof(sse_parser::Buffer) > 0, "sse_parser namespace alias must work");
static_assert(sizeof(sse_parser::MessageBuilder) > 0, "sse_parser namespace alias must work");
static_assert(sizeof(sse_parser::SseError) > 0, "sse_parser namespace alias must work");

// Verify Message struct members are accessible
static_assert(sizeof(&sse::Message::event) > 0, "Message::event member must be accessible");
static_assert(sizeof(&sse::Message::data) > 0, "Message::data member must be accessible");
static_assert(sizeof(&sse::Message::id) > 0, "Message::id member must be accessible");
static_assert(sizeof(&sse::Message::retry) > 0, "Message::retry member must be accessible");
static_assert(sizeof(&sse::Message::clear) > 0, "Message::clear method must be accessible");
static_assert(sizeof(&sse::Message::empty) > 0, "Message::empty method must be accessible");

// Verify Buffer class methods are accessible
static_assert(sizeof(&sse::Buffer::append) > 0, "Buffer::append must be accessible");
static_assert(sizeof(&sse::Buffer::read_line) > 0, "Buffer::read_line must be accessible");
static_assert(sizeof(&sse::Buffer::has_complete_message) > 0, "Buffer::has_complete_message must be accessible");
static_assert(sizeof(&sse::Buffer::size) > 0, "Buffer::size must be accessible");
static_assert(sizeof(&sse::Buffer::empty) > 0, "Buffer::empty must be accessible");
static_assert(sizeof(&sse::Buffer::clear) > 0, "Buffer::clear must be accessible");
static_assert(sizeof(&sse::Buffer::view) > 0, "Buffer::view must be accessible");
static_assert(sizeof(&sse::Buffer::max_size) > 0, "Buffer::max_size must be accessible");
static_assert(sizeof(&sse::Buffer::set_max_size) > 0, "Buffer::set_max_size must be accessible");

// Verify MessageBuilder class methods are accessible
static_assert(sizeof(&sse::MessageBuilder::set_callback) > 0, "MessageBuilder::set_callback must be accessible");
static_assert(sizeof(&sse::MessageBuilder::feed_line) > 0, "MessageBuilder::feed_line must be accessible");
static_assert(sizeof(&sse::MessageBuilder::get_message) > 0, "MessageBuilder::get_message must be accessible");
static_assert(sizeof(&sse::MessageBuilder::has_message) > 0, "MessageBuilder::has_message must be accessible");
static_assert(sizeof(&sse::MessageBuilder::current_message) > 0, "MessageBuilder::current_message must be accessible");
static_assert(sizeof(&sse::MessageBuilder::reset) > 0, "MessageBuilder::reset must be accessible");
static_assert(sizeof(&sse::MessageBuilder::last_event_id) > 0, "MessageBuilder::last_event_id must be accessible");
static_assert(sizeof(&sse::MessageBuilder::clear_last_event_id) > 0, "MessageBuilder::clear_last_event_id must be accessible");

// Verify SseParser class methods are accessible
static_assert(sizeof(&sse::SseParser::parse) > 0, "SseParser::parse must be accessible");
static_assert(sizeof(&sse::SseParser::set_callback) > 0, "SseParser::set_callback must be accessible");
static_assert(sizeof(&sse::SseParser::flush) > 0, "SseParser::flush must be accessible");
static_assert(sizeof(&sse::SseParser::reset) > 0, "SseParser::reset must be accessible");
static_assert(sizeof(&sse::SseParser::last_event_id) > 0, "SseParser::last_event_id must be accessible");
static_assert(sizeof(&sse::SseParser::has_incomplete_message) > 0, "SseParser::has_incomplete_message must be accessible");

// Verify type aliases are accessible
static_assert(sizeof(sse::SseParser::MessageCallback) > 0, "SseParser::MessageCallback must be accessible");
static_assert(sizeof(sse::SseParser::MessageCallbackFn) > 0, "SseParser::MessageCallbackFn must be accessible");
static_assert(sizeof(sse::MessageBuilder::MessageCallback) > 0, "MessageBuilder::MessageCallback must be accessible");

// Verify version macros
static_assert(SSE_PARSER_VERSION_MAJOR == 2, "Version major must be 2");
static_assert(SSE_PARSER_VERSION_MINOR == 0, "Version minor must be 0");
static_assert(SSE_PARSER_VERSION_PATCH == 0, "Version patch must be 0");

int main() {
    return 0;
}
