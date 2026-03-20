# Phase 9: Backward Compatibility — DELETE OLD HEADERS

**Phase:** 09 | **Milestone:** v2.0 Single Header Integration

## Decision

**删除旧头文件，用户使用新单文件 `sse_parser.hpp`**

v2.0 是 API breaking change，用户需要更新 include 路径。

## Files to Delete

| File | Reason |
|------|--------|
| `include/sse_parser/error_codes.h` | 合并到 sse_parser.hpp |
| `include/sse_parser/message.h` | 合并到 sse_parser.hpp |
| `include/sse_parser/buffer.h` | 合并到 sse_parser.hpp |
| `include/sse_parser/field_parser.h` | 合并到 sse_parser.hpp |
| `include/sse_parser/message_builder.h` | 合并到 sse_parser.hpp |
| `include/sse_parser/sse_parser_facade.h` | 合并到 sse_parser.hpp |
| `include/sse_parser/sse_parser.h` | 旧的入口，替换为 sse_parser.hpp |

## Tasks

### Task 9.1: Delete Old Header Files

```bash
rm include/sse_parser/error_codes.h
rm include/sse_parser/message.h
rm include/sse_parser/buffer.h
rm include/sse_parser/field_parser.h
rm include/sse_parser/message_builder.h
rm include/sse_parser/sse_parser_facade.h
rm include/sse_parser/sse_parser.h
```

### Task 9.2: Verify Only New Header Remains

```bash
ls include/sse_parser/
# Should show only: sse_parser.hpp
```

### Task 9.3: Update Test Includes

Update all test and example files to use new header:

```cpp
// Old (v1.0)
#include "sse_parser/sse_parser.h"
#include "sse_parser/buffer.h"

// New (v2.0)
#include "sse_parser/sse_parser.hpp"
```

### Task 9.4: Run Full Test Suite

```bash
cd build
cmake --build . --config Release
ctest -C Release --output-on-failure
```

## v2.0 Migration Guide

Users upgrading from v1.x to v2.0:

**Before:**
```cpp
#include "sse_parser/sse_parser.h"
#include "sse_parser/buffer.h"  // if used directly
```

**After:**
```cpp
#include "sse_parser/sse_parser.hpp"
```

## Files Modified

- `include/sse_parser/*.h` — Deleted (7 files)
- `tests/*.cpp` — Update include paths
- `examples/*.cpp` — Update include paths

## Verification

- [ ] Only `sse_parser.hpp` remains in `include/sse_parser/`
- [ ] All tests compile and pass
- [ ] No references to old header files remain

---

*Phase: 09-backward-compatibility*
*Plan: Delete old headers (2026-03-20)*
