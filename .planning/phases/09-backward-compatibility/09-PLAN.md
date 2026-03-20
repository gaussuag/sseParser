# Phase 9: Backward Compatibility

**Phase:** 09 | **Milestone:** v2.0 Single Header Integration

## Overview

Implement backward compatibility by creating forwarding header files that:
1. Include the new `sse_parser.hpp`
2. Emit deprecation warnings to inform users to migrate

## Requirements

| ID | Requirement |
|----|------------|
| BCMP-01 | Legacy `sse_parser/buffer.h` still works |
| BCMP-02 | Legacy `sse_parser/message.h` still works |
| BCMP-03 | Legacy `sse_parser/field_parser.h` still works |
| BCMP-04 | Legacy `sse_parser/sse_parser_facade.h` still works |
| BCMP-05 | Deprecation warnings added to legacy include paths |

## Implementation Strategy

Each legacy header file will be replaced with a minimal forwarding header:

```cpp
/**
 * @file buffer.h
 * @deprecated Use sse_parser.hpp instead. This header will be removed in v3.0.
 * Include sse_parser/sse_parser.hpp for the complete API.
 */
#pragma once

#ifdef __GNUC__
#pragma GCC warning "buffer.h is deprecated. Include sse_parser/sse_parser.hpp instead."
#elif defined(_MSC_VER)
#pragma message("buffer.h is deprecated. Include sse_parser/sse_parser.hpp instead.")
#endif

#include "sse_parser.hpp"
```

## Tasks

### Task 9.1: Create Forwarding Headers

Create forwarding headers for each legacy include:

1. `include/sse_parser/buffer.h` → forwards to `sse_parser.hpp`
2. `include/sse_parser/message.h` → forwards to `sse_parser.hpp`
3. `include/sse_parser/field_parser.h` → forwards to `sse_parser.hpp`
4. `include/sse_parser/message_builder.h` → forwards to `sse_parser.hpp`
5. `include/sse_parser/sse_parser_facade.h` → forwards to `sse_parser.hpp`
6. `include/sse_parser/error_codes.h` → forwards to `sse_parser.hpp`

### Task 9.2: Verify Compilation

Test that legacy includes still compile:

```cpp
// Test each legacy include
#include "sse_parser/buffer.h"      // Should compile with warning
#include "sse_parser/message.h"     // Should compile with warning
#include "sse_parser/field_parser.h" // Should compile with warning
```

### Task 9.3: Run Test Suite

Ensure all 298+ tests still pass with legacy includes.

## Files to Create/Modify

- `include/sse_parser/buffer.h` — Replace with forwarding header
- `include/sse_parser/message.h` — Replace with forwarding header
- `include/sse_parser/field_parser.h` — Replace with forwarding header
- `include/sse_parser/message_builder.h` — Replace with forwarding header
- `include/sse_parser/sse_parser_facade.h` — Replace with forwarding header
- `include/sse_parser/error_codes.h` — Replace with forwarding header

## Verification

```bash
# Build tests with legacy includes
cd build
cmake --build . --config Release
ctest -C Release --output-on-failure
```

## Notes

- The old header files cannot be completely deleted for backward compatibility
- Deprecation warnings help users migrate to the new single-header approach
- v3.0 can consider removing these forwarding headers entirely

---

*Phase: 09-backward-compatibility*
*Plan: Full implementation (2026-03-20)*
