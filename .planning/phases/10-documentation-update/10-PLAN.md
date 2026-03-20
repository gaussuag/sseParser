# Phase 10: Documentation Update

**Phase:** 10 | **Milestone:** v2.0 Single Header Integration

## Overview

Update all documentation for v2.0 release with single-header library.

## Requirements

| ID | Requirement |
|----|------------|
| DOCS-01 | Updated README with v2.0 include instructions |
| DOCS-02 | Doxygen configuration updated for single header |
| DOCS-03 | Migration guide from v1.x to v2.0 provided |
| DOCS-04 | All usage examples updated to single include |

## Tasks

### Task 10.1: Rewrite README.md

Create new README for SSE Parser v2.0:

```markdown
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
```

### Task 10.2: Create CHANGELOG.md

Create version history:

```markdown
# Changelog

## v2.0.0 (2026-03-20)

### Breaking Changes
- Single header file: `sse_parser.hpp` (was multiple headers)
- Include path changed from `.h` to `.hpp`

### Migration
- Update: `#include "sse_parser/sse_parser.h"` → `#include "sse_parser/sse_parser.hpp"`

## v1.0.0 (2026-03-19)
- Initial release
```

### Task 10.3: Update Doxygen Configuration

Update `docs/Doxyfile`:

```
INPUT = include/sse_parser/sse_parser.hpp
FILE_PATTERNS = *.hpp
```

### Task 10.4: Update Usage Examples

Update `examples/usage_examples.cpp`:

```cpp
// Old
#include "../include/sse_parser/sse_parser.h"
#include "../include/sse_parser/buffer.h"

// New
#include "../include/sse_parser/sse_parser.hpp"
```

## Files to Create

- `CHANGELOG.md` — Version history

## Files to Modify

- `README.md` — Complete rewrite for v2.0
- `docs/Doxyfile` — Update for .hpp file
- `examples/usage_examples.cpp` — Update include path

## Verification

- [ ] README shows `sse_parser.hpp` include
- [ ] CHANGELOG.md exists with v2.0 entry
- [ ] Doxygen configured for .hpp
- [ ] Examples use single include
- [ ] README has migration guide

---

*Phase: 10-documentation-update*
*Plan: (2026-03-20)*
