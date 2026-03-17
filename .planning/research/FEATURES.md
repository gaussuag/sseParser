# Research: SSE Parser Features

## Protocol Reference

Based on [WHATWG Server-Sent Events spec](https://html.spec.whatwg.org/multipage/server-sent-events.html).

## Table Stakes (Must Have)

These are required for a minimal usable SSE parser:

| Feature | Description | Complexity |
|---------|-------------|------------|
| Field Parsing | Parse `field: value` format | Low |
| Standard Fields | Support event, data, id, retry | Low |
| Message Delimiting | Detect `\n\n`, `\r\n\r\n`, `\r\r` | Medium |
| Data Accumulation | Merge multiple `data:` lines with `\n` | Medium |
| Comment Handling | Skip lines starting with `:` | Low |
| Empty Line Handling | Handle `\n` within messages correctly | Medium |
| Callback Delivery | Deliver complete messages via callback | Low |
| Input Buffering | Handle fragmented input across calls | Medium |

## Differentiators (Competitive Advantages)

| Feature | Description | Value |
|---------|-------------|-------|
| Zero-Copy Input | Accept `string_view` without copying | Performance |
| Zero-Allocation Path | No heap allocations during parsing | Embedded |
| Configurable Buffer | Template parameter for buffer size | Flexibility |
| Field Validation | Validate retry is numeric | Robustness |
| BOM Handling | Strip UTF-8 BOM at stream start | Compatibility |
| Last-Event-ID Tracking | Auto-track and expose last ID | Convenience |
| Reconnection Timing | Parse and expose retry timing | Full spec |
| Error Recovery | Continue parsing after errors | Resilience |

## Anti-Features (Deliberately NOT Building)

| Feature | Reason |
|---------|--------|
| HTTP client | Scope: parsing only |
| JSON parsing | Out of scope, user handles data content |
| Compression | User handles encoding before parser |
| Thread safety | User serializes access |
| WebSocket support | Different protocol |
| Chunked encoding | HTTP layer concern |
| Server-side generation | Client-only |
| Automatic reconnection | Network layer concern |

## Feature Dependencies

```
Core Parser
├── Field Parsing (required by all)
├── Message Delimiting (requires buffering)
├── Data Accumulation (requires buffer management)
└── Callback Delivery (requires message assembly)

Advanced Features
├── Zero-Copy Input (requires string_view API)
├── Zero-Allocation (requires fixed buffer)
├── Field Validation (adds complexity)
└── Error Recovery (requires error state machine)
```

## Edge Cases to Handle

1. **Empty messages**: `\n\n` alone — should trigger callback with empty data
2. **No data field**: Message without `data:` — should still be valid
3. **Mixed line endings**: `\n` and `\r\n` in same stream — normalize
4. **Colon in value**: `data: hello: world` — value is `hello: world`
5. **Leading space**: `data:  hello` — value is ` hello` (keep space)
6. **No colon**: `event` — field name only, empty value
7. **Very long lines**: Exceed buffer — error or truncate?
8. **CR without LF**: `\r\r` as message separator — spec allows

## Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|-------------------|----------|
| Core field parsing | Critical | Low | P0 |
| Message delimiting | Critical | Medium | P0 |
| Data accumulation | Critical | Medium | P0 |
| Callback interface | Critical | Low | P0 |
| Input buffering | Critical | Medium | P0 |
| Zero-copy input | High | Low | P1 |
| Zero-allocation | High | Medium | P1 |
| Field validation | Medium | Low | P2 |
| BOM handling | Low | Low | P2 |
| Error recovery | Medium | High | P2 |
