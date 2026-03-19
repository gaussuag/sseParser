# Technology Stack

## Overview

SSE Parser is a C++17 header-only library for parsing Server-Sent Events (SSE) streams. It has zero external dependencies for the core library.

## Languages & Standards

- **Primary Language**: C++17
- **C++ Standard**: ISO C++17
- **Namespace**: `sse` (primary), `sse_parser` (deprecated alias)

## Build System

- **CMake**: 3.16+ minimum required
- **Configuration Files**:
  - `CMakeLists.txt` - Root configuration
  - `src/CMakeLists.txt` - Source target
  - `tests/CMakeLists.txt` - Test targets
  - `cmake/CompilerOptions.cmake` - Compiler flags
  - `cmake/VcpkgDeps.cmake` - vcpkg integration

## Dependencies

### Core Library
**Zero dependencies** - header-only library using only C++17 standard library.

### Testing
- **GoogleTest**: Built-in from `libs/googletest/` (git submodule or vendored)
- Test executable: `sse_parser_tests`

### Build/Development Tools
- **vcpkg**: Manifest mode for dependency management (`vcpkg.json`)
  - Currently empty dependencies array (no external deps needed)
- **Doxygen**: Documentation generation (`docs/Doxyfile`)

## Compiler Support

### Verified Compilers (via CI)
- **GCC** (latest on Ubuntu)
- **Clang** (latest on Ubuntu)
- **MSVC** (Visual Studio 2022 on Windows)

### Compiler Flags

**MSVC**:
- `/utf-8` - UTF-8 source/execution charset
- `/W4` - High warning level
- `/permissive-` - Strict conformance mode
- `/O2` - Optimization (Release)
- `/Od /Zi` - Debug symbols (Debug)

**GCC/Clang**:
- `-Wall -Wextra -pedantic` - Strict warnings
- `-Werror` - Treat warnings as errors (CI)
- `-O3` - Optimization (Release)
- `-g -O0` - Debug (Debug)

## Project Structure

```
sseParser/
├── CMakeLists.txt          # Root build config
├── vcpkg.json              # vcpkg manifest (empty deps)
├── cmake/                  # CMake modules
│   ├── CompilerOptions.cmake
│   └── VcpkgDeps.cmake
├── include/sse_parser/     # Header-only library
├── src/                    # Example/main application
├── tests/                  # GoogleTest test suite
├── libs/googletest/        # Testing framework
├── scripts/                # Build automation
├── docs/                   # Documentation & Doxyfile
└── .github/workflows/      # CI/CD
```

## Version

- **Current**: 1.0.0
- **Version Macros**: Defined in `include/sse_parser/sse_parser.h`
  - `SSE_PARSER_VERSION_MAJOR 1`
  - `SSE_PARSER_VERSION_MINOR 0`
  - `SSE_PARSER_VERSION_PATCH 0`

## Package Management

- **vcpkg Integration**: Automatic via `VCPKG_ROOT` environment variable
- **Manifest Mode**: Enabled (`VCPKG_MANIFEST_MODE ON`)
- **Auto-linking**: `cmake/VcpkgDeps.cmake` automatically links vcpkg dependencies

## Documentation

- **Doxygen**: Configured in `docs/Doxyfile`
- **Output**: HTML documentation in `docs/html/`
- **Style**: Javadoc-style comments
- **Features**: Source browsing, cross-referencing, call graphs
