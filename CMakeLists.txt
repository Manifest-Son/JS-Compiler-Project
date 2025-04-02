cmake_minimum_required(VERSION 3.13)
project(JS_Compiler_Final_Project)

set(CMAKE_CXX_STANDARD 20)

add_executable(JS_Compiler_Final_Project src/main.cpp
        include/lexer.h
        src/lexer.cpp
        include/token.h
        include/ast.h
        include/parser.h
        include/parser_error.h
        src/parser.cpp
        src/ast.cpp)

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
# include_directories(${LLVM_INCLUDE_DIRS})
# add_definitions(${LLVM_DEFINITIONS})

# Compiler Source Files
# set(SOURCES
#        src
# )
