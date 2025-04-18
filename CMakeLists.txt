cmake_minimum_required(VERSION 3.16)
project(JS_Compiler_Final_Project)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(JS_Compiler_Final_Project
        src/main.cpp
        include/lexer.h
        src/lexer.cpp
        include/token.h
        include/ast.h
        include/parser.h
        include/parser_error.h
        src/parser.cpp
        src/ast.cpp
        src/error_reporter.cpp
        include/error_reporter.h
        include/ast_printer.h
        include/ast_visitor.h
        include/source_position.h
        src/source_position.cpp
)

# Benchmark executable
add_executable(JS_Compiler_Benchmark
        src/benchmark_main.cpp
        include/benchmark.h
        include/lexer.h
        src/lexer.cpp
        include/token.h
        include/ast.h
        include/parser.h
        include/parser_error.h
        src/parser.cpp
        src/ast.cpp
        src/error_reporter.cpp
        include/error_reporter.h
        include/ast_visitor.h
        include/source_position.h
        src/source_position.cpp
)
# Find LLVM package
find_package(LLVM REQUIRED CONFIG)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

include_directories(JS_Compiler_Final_Project)
add_subdirectory(tests)

# Find LLVM
# find_package(LLVM REQUIRED CONFIG)
 include_directories(${LLVM_INCLUDE_DIRS})
 add_definitions(${LLVM_DEFINITIONS})

# Add source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Add include directories
include_directories(include)

# Create the executable
add_executable(js_compiler ${SOURCES})

# Testing
#enable_testing()
#find_package(GTest REQUIRED)
#include(GoogleTest)

# AST tests
add_executable(ast_test 
    tests/ast/ast_test.cc
    src/ast.cpp
)
target_link_libraries(ast_test PRIVATE GTest::gtest GTest::gtest_main)
target_include_directories(ast_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
gtest_discover_tests(ast_test)

# Source position tests
add_executable(source_position_test
    tests/source_position/source_position_test.cc
    src/source_position.cpp
    src/ast.cpp
)
target_link_libraries(source_position_test PRIVATE GTest::gtest GTest::gtest_main)
target_include_directories(source_position_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
gtest_discover_tests(source_position_test)

# Link LLVM libraries
# Find LLVM package
find_package(LLVM REQUIRED CONFIG)
set(LLVM_ROOT "C:/Program Files/LLVM")

# Include LLVM directories
include_directories(${LLVM_ROOT}/include)
link_directories("${LLVM_ROOT}/lib")
#add_definitions(${LLVM_DEFINITIONS})

# Link against LLVM libraries
target_link_libraries(your_target ${LLVM_LIBRARIES})
