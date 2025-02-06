cmake_minimum_required(VERSION 3.30)
project(JS_Compiler_Final_Project)

set(CMAKE_CXX_STANDARD 20)

add_executable(JS_Compiler_Final_Project src/main.cpp
        include/lexer.h
        src/lexer.cpp)

#Find LLVM
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Compiler Source Files
#set(SOURCES
#        src
#)