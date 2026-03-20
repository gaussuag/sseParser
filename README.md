# SSE Parser

C++17 Header-Only Server-Sent Events Parser

## Features

- Single header file: `#include "sse_parser/sse_parser.hpp"`
- Zero dependencies (C++17 STL only)
- Handles network fragmentation
- Supports all SSE fields: event, data, id, retry
- Callback-based API
- Zero-copy string_view interface

## Quick Start

```cpp
#include "sse_parser/sse_parser.hpp"

sse::SseParser parser([](const sse::Message& msg) {
    std::cout << "Event: " << msg.event << "\n";
    std::cout << "Data: " << msg.data << "\n";
});

parser.parse(data, length);
```

## Installation

Copy `sse_parser.hpp` to your project or add this repository as a submodule.

## API

See [documentation](docs/html/) for full API reference.

## v2.0 Migration

If upgrading from v1.x:

```cpp
// v1.x
#include "sse_parser/sse_parser.h"
#include "sse_parser/buffer.h"

// v2.0
#include "sse_parser/sse_parser.hpp"
```
