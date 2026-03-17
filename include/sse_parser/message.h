#pragma once

#include <optional>
#include <string>

namespace sse_parser {

struct Message {
    std::string event = "message";
    std::string data;
    std::optional<std::string> id;
    std::optional<int> retry;

    void clear() noexcept {
        event = "message";
        data.clear();
        id.reset();
        retry.reset();
    }

    bool empty() const noexcept {
        return data.empty();
    }
};

}
