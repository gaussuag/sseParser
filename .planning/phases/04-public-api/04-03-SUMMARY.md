---
phase: 04-public-api
plan: 03
subsystem: api

requires:
  - phase: 04-public-api
    provides: [SseParser facade, parse methods, flush()]

provides:
  - reset() method for reconnection scenarios
  - Function pointer callback support (IFC-01)
  - Zero-overhead embedded-friendly API

affects:
  - 04-public-api (completes API-05 and IFC-01)
  - 05-polish (testing of new features)

tech-stack:
  added: []
  patterns:
    - Function pointer + user_data for C compatibility
    - Overloaded set_callback for multiple callback types

key-files:
  created: []
  modified:
    - include/sse_parser/sse_parser_facade.h

key-decisions:
  - Function pointer callback can coexist with std::function (last set wins)
  - reset() keeps callback registered for reconnection use case
  - MessageBuilder receives std::function wrapper for function pointer

requirements-completed: [API-05, IFC-01]

duration: 3 min
completed: 2026-03-18
---

# Phase 04 Plan 03: Reset and Callback Variants Summary

**Added reset() method and function pointer callback support for embedded/low-overhead use cases**

## Performance

- **Duration:** 3 min
- **Started:** 2026-03-18T04:53:00Z
- **Completed:** 2026-03-18T04:56:00Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments

- Implemented `reset()` method that clears buffer, MessageBuilder state, and last_event_id while preserving callback registration (API-05)
- Added function pointer callback type `MessageCallbackFn` for zero-overhead embedded scenarios (IFC-01)
- Created overloaded `set_callback()` to support both std::function and function pointer interfaces
- Both callback types can coexist with "last set wins" semantics

## Task Commits

1. **Task 1: Add reset() method** - `cf29654` (feat)
2. **Task 2: Add function pointer callback support** - `09cfe78` (feat)

**Plan metadata:** TBD (docs commit)

## Files Created/Modified

- `include/sse_parser/sse_parser_facade.h` - Added reset() method, MessageCallbackFn type, and set_callback overload

## Decisions Made

- Function pointer callback uses `void*` user_data pattern for C compatibility
- reset() preserves callback registration for typical reconnection use case
- Function pointer is wrapped in std::function for MessageBuilder compatibility (avoids dual path in MessageBuilder)
- Both std::function and function pointer can be set; the last one set takes precedence

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- API-05 (reset) complete ✓
- IFC-01 (function pointer callback) complete ✓
- Phase 4 now 75% complete (3/4 plans)
- Ready for Phase 4 Plan 4 (final Public API plan)

---
*Phase: 04-public-api*
*Completed: 2026-03-18*
