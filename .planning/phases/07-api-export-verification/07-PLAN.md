# Phase 7: API Export Verification

**Phase:** 07 | **Milestone:** v2.0 Single Header Integration

## Overview

Verify that all public API symbols from the merged `sse_parser.hpp` are properly accessible and functional.

## Context

From `.planning/phases/06-header-consolidation/06-CONTEXT.md`:
- Single file `sse_parser.hpp` created with version 2.0.0
- All 7 headers merged using `#pragma region` for organization
- No `SSE_PARSER_API` macro needed (header-only library)

## Requirements

| ID | Requirement | Source |
|----|-------------|--------|
| HDRI-04 | Public API symbols accessible from single header | REQUIREMENTS.md |

## Success Criteria

1. `sse::SseParser` class fully accessible
2. `sse::Message` struct fully accessible
3. `sse::SseError` enum fully accessible
4. `sse::Buffer` class fully accessible
5. `sse::MessageBuilder` class fully accessible
6. All free functions accessible: `parse_field_line`, `error_message`, `has_bom`, `skip_bom`
7. Both `std::function` and function pointer callbacks work
8. All 295+ existing tests pass with single include

## Tasks

### Task 7.1: Verify Symbol Accessibility

Create a verification test file that includes ONLY `sse_parser.hpp` and verifies all public API symbols are accessible:

```cpp
// verify_api_export.cpp
#include "sse_parser/sse_parser.hpp"

// Verify classes
static_assert(sizeof(sse::SseParser) > 0, "SseParser accessible");
static_assert(sizeof(sse::Message) > 0, "Message accessible");
static_assert(sizeof(sse::Buffer) > 0, "Buffer accessible");
static_assert(sizeof(sse::MessageBuilder) > 0, "MessageBuilder accessible");

// Verify enum
static_assert(static_cast<int>(sse::SseError::success) == 0, "SseError accessible");

// Verify free functions
static_assert(sizeof(sse::error_message) > 0, "error_message accessible");
static_assert(sizeof(sse::has_bom) > 0, "has_bom accessible");
static_assert(sizeof(sse::skip_bom) > 0, "skip_bom accessible");
static_assert(sizeof(sse::parse_field_line) > 0, "parse_field_line accessible");
```

### Task 7.2: Verify Callback Mechanisms

Ensure both callback types work:

```cpp
// Test std::function callback
sse::SseParser parser([](const sse::Message& msg) {
    // Callback invoked
});

// Test function pointer callback
void (*fp)(const sse::Message*, void*) = nullptr;
parser.set_callback(fp, nullptr);
```

### Task 7.3: Run Full Test Suite

Run existing test suite with single-header include:
- Compile all test files with `#include "sse_parser/sse_parser.hpp"`
- Verify 295+ tests still pass
- Verify no compilation warnings

### Task 7.4: Update Include Paths in Tests

Update test files to use new single include:
- Change `#include "sse_parser/sse_parser.h"` → `#include "sse_parser/sse_parser.hpp"`
- Change `#include "sse_parser/buffer.h"` → `#include "sse_parser/sse_parser.hpp"`
- etc.

## Execution

```bash
# Build and run tests
cd build
cmake --build . --config Release
ctest -C Release --output-on-failure

# Run specific API verification
./tests/sse_parser_tests
```

## Verification

- [ ] All API symbols accessible via single include
- [ ] All 295+ tests pass
- [ ] No compilation warnings
- [ ] Test files updated to use sse_parser.hpp

## Files Modified

- `tests/test_sse_parser.cpp` — Update include path
- `tests/test_buffer.cpp` — Update include path
- `tests/test_message.cpp` — Update include path
- `tests/test_field_parser.cpp` — Update include path
- `tests/test_integration*.cpp` — Update include paths
- `tests/test_edge_cases.cpp` — Update include path
- `examples/usage_examples.cpp` — Update include path
