# Original project tests (require curl, etc.)
add_executable(${PROJECT_NAME}_tests 
    main_test.cpp
)

target_link_vcpkg_deps(${PROJECT_NAME}_tests)

target_link_libraries(${PROJECT_NAME}_tests 
    PRIVATE
    gtest_main
    gmock
)

target_include_directories(${PROJECT_NAME}_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

file(GLOB LIB_INCLUDE_DIRS LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/libs/*/include")
foreach(inc_dir ${LIB_INCLUDE_DIRS})
    if(IS_DIRECTORY ${inc_dir})
        target_include_directories(${PROJECT_NAME}_tests PRIVATE ${inc_dir})
    endif()
endforeach()

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests)

# SSE Parser tests (header-only, no external deps)
add_executable(sse_parser_tests
    test_error_codes.cpp
    test_message.cpp
    test_buffer.cpp
    test_integration_p1.cpp
    test_line_endings.cpp
    test_field_parser.cpp
    test_integration_p2.cpp
    test_integration_p3.cpp
    test_sse_parser.cpp
    test_integration.cpp
    test_edge_cases.cpp
    test_fuzzing.cpp
    test_performance.cpp
)

target_link_libraries(sse_parser_tests
    PRIVATE
    gtest_main
)

target_include_directories(sse_parser_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/sse_parser
)

# Set C++17 standard for SSE parser tests
target_compile_features(sse_parser_tests PRIVATE cxx_std_17)

# Add tests to CTest
add_test(NAME sse_parser_unit_tests COMMAND sse_parser_tests)
gtest_discover_tests(sse_parser_tests)
