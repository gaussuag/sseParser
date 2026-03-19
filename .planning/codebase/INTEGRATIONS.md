# Integrations

## External Services & APIs

**None** - SSE Parser is a pure parsing library with no external service integrations.

The library is designed to be a standalone component that receives SSE stream data from any source (HTTP client, file, network socket, etc.) and parses it into structured messages.

## Integration Patterns

### Input Sources

The library accepts raw byte streams from any source:

- **HTTP Clients**: libcurl, Boost.Beast, custom HTTP implementations
- **Network Sockets**: Raw TCP/HTTP streams
- **Files**: Local SSE dump files
- **Memory Buffers**: In-memory data for testing

Example integration with HTTP client:
```cpp
// Pseudo-code showing typical integration
HttpClient client;
sse::SseParser parser([](const sse::Message& msg) {
    process_message(msg);
});

client.onData([&parser](const char* data, size_t len) {
    parser.parse(data, len);  // Feed raw HTTP response body
});
```

### Output Consumers

Parsed messages are delivered via callback:

- **Application Handlers**: Process events in UI or business logic
- **Message Queues**: Forward to async processing pipelines
- **Logging Systems**: Record events for debugging
- **State Machines**: Update application state based on events

## Dependencies Summary

| Component | External Dependencies | Notes |
|-----------|----------------------|-------|
| Core Library | None | Header-only, STL only |
| Tests | GoogleTest | Bundled in `libs/googletest/` |
| Examples | None | Only uses core library |
| Documentation | Doxygen | Optional build tool |
| Build | CMake, vcpkg | Build-time only |

## Standards Compliance

- **SSE Protocol**: W3C Server-Sent Events specification
- **C++ Standard**: ISO C++17
- **Encoding**: UTF-8 support with BOM detection
- **Line Endings**: LF, CRLF, CR support

## CI/CD Integration

### GitHub Actions
- **Workflow**: `.github/workflows/multi-compiler.yml`
- **Triggers**: Push to main/develop, PRs to main
- **Jobs**:
  - GCC build & test (Ubuntu)
  - Clang build & test (Ubuntu)
  - MSVC build & test (Windows)

### Build Scripts
- `scripts/build.bat` - Windows Visual Studio build
- `scripts/build.sh` - Unix build (if available)
- `scripts/config.bat` - Project configuration
- `scripts/detect_libs.bat` - Auto-detect libs/ directory

## Reconnection Support

The library provides reconnection state management:

- **Last-Event-ID Tracking**: `parser.last_event_id()` returns last seen ID
- **Reset Capability**: `parser.reset()` clears state for reconnection
- **Use Case**: HTTP clients can reconnect with `Last-Event-ID` header

```cpp
// Typical reconnection flow
sse::SseParser parser(callback);

// First connection
parser.parse(stream_data);
std::string last_id = parser.last_event_id();

// Connection drops...
parser.reset();

// Reconnect with last ID
http_client.reconnect({
    {"Last-Event-ID", last_id}
});
```
