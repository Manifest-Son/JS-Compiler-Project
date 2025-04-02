# Add Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Add test executable
add_executable(lexer_test lexer/lexer_test.cc
        ../src/ast.cpp)
target_link_libraries(lexer_test gtest_main)

# Discover tests
include(GoogleTest)
gtest_discover_tests(lexer_test)
