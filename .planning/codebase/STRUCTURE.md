# Directory Structure

## Project Layout

```
sseParser/
├── .github/
│   └── workflows/
│       └── multi-compiler.yml     # CI: GCC, Clang, MSVC validation
│
├── .planning/                     # GSD planning documents
│   ├── codebase/                  # This directory
│   ├── milestones/
│   ├── phases/
│   ├── research/
│   ├── STATE.md                   # Current project state
│   ├── PROJECT.md                 # Project overview
│   └── MILESTONES.md              # Milestone tracking
│
├── cmake/
│   ├── CompilerOptions.cmake      # MSVC/GCC/Clang flags
│   └── VcpkgDeps.cmake            # vcpkg auto-linking
│
├── docs/
│   ├── Doxyfile                   # Doxygen configuration
│   ├── vcpkg-dependency-issues.md # Troubleshooting guide
│   └── html/                      # Generated docs (gitignored)
│
├── examples/
│   └── usage_examples.cpp         # 8 usage examples
│
├── include/sse_parser/            # Header-only library
│   ├── sse_parser.h               # Main include (version + aggregates)
│   ├── sse_parser_facade.h        # SseParser public API class
│   ├── message_builder.h          # Message assembly logic
│   ├── field_parser.h             # Field line parsing
│   ├── buffer.h                   # Stream buffering
│   ├── message.h                  # Message data structure
│   └── error_codes.h              # Error codes & utilities
│
├── libs/
│   └── googletest/                # GoogleTest framework
│       ├── googletest/
│       ├── googlemock/
│       └── CMakeLists.txt
│
├── scripts/
│   ├── build.bat                  # Windows build script
│   ├── build.sh                   # Unix build script
│   ├── config.bat                 # Project configuration
│   └── detect_libs.bat            # Auto-detect libs/
│
├── src/
│   ├── CMakeLists.txt             # Source target config
│   └── main.cpp                   # Example application
│
├── tests/
│   ├── CMakeLists.txt             # Test targets config
│   ├── data/                      # Test data files
│   │   ├── binary_data.bin
│   │   ├── edge_cases.sse
│   │   ├── large_stream.sse
│   │   ├── mixed_format.sse
│   │   └── small_messages.sse
│   ├── main_test.cpp              # Original project tests
│   ├── test_buffer.cpp            # Buffer unit tests
│   ├── test_error_codes.cpp       # Error code tests
│   ├── test_edge_cases.cpp        # Edge case tests
│   ├── test_field_parser.cpp      # Field parsing tests
│   ├── test_fuzzing.cpp           # Fuzzing tests
│   ├── test_integration.cpp       # Integration tests
│   ├── test_integration_p1.cpp    # Phase 1 integration
│   ├── test_integration_p2.cpp    # Phase 2 integration
│   ├── test_integration_p3.cpp    # Phase 3 integration
│   ├── test_line_endings.cpp      # Line ending tests
│   ├── test_message.cpp           # Message struct tests
│   ├── test_performance.cpp       # Performance benchmarks
│   └── test_sse_parser.cpp        # Public API tests (25+ cases)
│
├── CMakeLists.txt                 # Root build configuration
├── README.md                      # Project documentation
├── REQUIREMENTS_TRACEABILITY.md   # Requirements matrix
├── vcpkg.json                     # vcpkg manifest
└── .gitignore                     # Git ignore rules
```

## Key Locations

### Source Code
- **Library headers**: `include/sse_parser/*.h`
- **Main header**: `include/sse_parser/sse_parser.h`
- **Public API**: `include/sse_parser/sse_parser_facade.h`
- **Example code**: `src/main.cpp`, `examples/usage_examples.cpp`

### Build Configuration
- **Root CMake**: `CMakeLists.txt`
- **Test CMake**: `tests/CMakeLists.txt`
- **Compiler flags**: `cmake/CompilerOptions.cmake`
- **vcpkg config**: `vcpkg.json`

### Testing
- **Unit tests**: `tests/test_*.cpp`
- **Test data**: `tests/data/`
- **Test runner**: `sse_parser_tests` executable

### Documentation
- **README**: `README.md`
- **Doxygen config**: `docs/Doxyfile`
- **Generated docs**: `docs/html/`

## Naming Conventions

### Files
- **Headers**: `snake_case.h`
- **Implementation**: Headers only (no .cpp for library)
- **Tests**: `test_<component>.cpp`
- **CMake**: `CMakeLists.txt`, `*.cmake`

### Directories
- **All lowercase**: `include/`, `tests/`, `cmake/`
- **Library namespace**: `sse_parser/`

## Generated/Build Artifacts

```
build/
├── CMakeCache.txt
├── CMakeFiles/
├── src/
│   └── sseParser.exe             # Main executable
├── tests/
│   ├── sse_parser_tests.exe      # SSE parser test suite
│   └── sseParser_tests.exe       # Original project tests
└── *.vcxproj                     # Visual Studio projects
```

## Git Exclusions

**Ignored directories** (`.gitignore`):
- `build/` - Build artifacts
- `docs/html/` - Generated documentation
- `.vs/`, `.vscode/` - IDE files
- `*.user`, `*.suo` - Visual Studio user files
- `out/`, `cmake-build-*/` - Alternative build dirs
