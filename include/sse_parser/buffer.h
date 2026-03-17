#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "error_codes.h"

namespace sse_parser {

class Buffer {
public:
    Buffer() = default;
    explicit Buffer(size_t max_size) : max_size_(max_size) {}

    SseError append(const char* data, size_t len);
    SseError append(std::string_view data);

    std::optional<std::string_view> read_line();
    bool has_complete_message() const;

    size_t size() const noexcept { return buffer_.size(); }
    bool empty() const noexcept { return buffer_.empty(); }
    void clear() noexcept;
    std::string_view view() const { return buffer_; }

    size_t max_size() const noexcept { return max_size_; }
    void set_max_size(size_t max_size) noexcept { max_size_ = max_size; }

private:
    std::string buffer_;
    size_t read_pos_ = 0;
    size_t max_size_ = 4096;

    size_t find_line_end(size_t start) const;
    void compact();
};

inline SseError Buffer::append(const char* data, size_t len) {
    if (len == 0) {
        return SseError::success;
    }

    if (buffer_.size() + len > max_size_) {
        compact();
        if (buffer_.size() + len > max_size_) {
            return SseError::buffer_overflow;
        }
    }

    buffer_.append(data, len);
    return SseError::success;
}

inline SseError Buffer::append(std::string_view data) {
    return append(data.data(), data.size());
}

inline std::optional<std::string_view> Buffer::read_line() {
    if (read_pos_ >= buffer_.size()) {
        return std::nullopt;
    }

    size_t line_end = find_line_end(read_pos_);
    if (line_end == std::string::npos) {
        return std::nullopt;
    }

    size_t line_start = read_pos_;
    size_t line_length = line_end - line_start;

    read_pos_ = line_end;

    // Handle line endings: LF, CRLF, or CR
    if (read_pos_ < buffer_.size() && buffer_[read_pos_] == '\r') {
        ++read_pos_;  // Skip the CR
        // If this is CRLF, also skip the LF
        if (read_pos_ < buffer_.size() && buffer_[read_pos_] == '\n') {
            ++read_pos_;
        }
    } else if (read_pos_ < buffer_.size() && buffer_[read_pos_] == '\n') {
        ++read_pos_;  // Skip the LF
    }

    while (line_length > 0 && 
           (buffer_[line_start + line_length - 1] == '\r' ||
            buffer_[line_start + line_length - 1] == '\n')) {
        --line_length;
    }

    return std::string_view(buffer_.data() + line_start, line_length);
}

inline bool Buffer::has_complete_message() const {
    size_t pos = read_pos_;

    while (pos < buffer_.size()) {
        size_t line_end = find_line_end(pos);
        if (line_end == std::string::npos) {
            return false;
        }

        if (line_end == pos ||
            (line_end == pos + 1 && buffer_[pos] == '\r')) {
            return true;
        }

        pos = line_end;
        if (pos < buffer_.size() && buffer_[pos] == '\n') {
            ++pos;
        }
    }

    return false;
}

inline void Buffer::clear() noexcept {
    buffer_.clear();
    read_pos_ = 0;
}

inline size_t Buffer::find_line_end(size_t start) const {
    for (size_t i = start; i < buffer_.size(); ++i) {
        if (buffer_[i] == '\n') {
            return i;
        }
        if (buffer_[i] == '\r') {
            if (i + 1 < buffer_.size() && buffer_[i + 1] == '\n') {
                return i;
            }
            return i;
        }
    }
    return std::string::npos;
}

inline void Buffer::compact() {
    if (read_pos_ > 0) {
        buffer_.erase(0, read_pos_);
        read_pos_ = 0;
    }
}

}  // namespace sse_parser
