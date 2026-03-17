---
id: 01-10
phase: 1
wave: 3
autonomous: true
objective: Update CMakeLists.txt to include all new test files
req_ids: []
gap_closure: false
---

# Plan 01-10: Build System Update

**Objective:** Update CMakeLists.txt to include all new test files and verify build works.

**Estimated Time:** 20 minutes

## Tasks

### Task 1: Update tests/CMakeLists.txt
Add all test files:
```cmake
add_executable(${PROJECT_NAME}_tests 
    main_test.cpp
    test_error_codes.cpp
    test_message.cpp
    test_buffer.cpp
    test_integration_p1.cpp
)

target_link_libraries(${PROJECT_NAME}_tests 
    PRIVATE
    gtest_main
    gmock
)

target_include_directories(${PROJECT_NAME}_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)
```

### Task 2: Verify Build Configuration
- Check include directories
- Ensure gtest is properly linked
- Verify C++17 standard is set

### Task 3: Test Build
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Task 4: Run Tests
```bash
ctest --output-on-failure
```

## Files Modified

- `tests/CMakeLists.txt`

## Success Criteria

- [ ] All test files added to CMakeLists.txt
- [ ] Project configures with cmake
- [ ] Project builds without errors
- [ ] All tests run with ctest
- [ ] No compiler warnings

## Dependencies

- 01-06: Error Code Tests
- 01-07: Message Tests
- 01-08: Buffer Tests
- 01-09: Integration Test

## Notes

This is the final plan - verifies everything works together
