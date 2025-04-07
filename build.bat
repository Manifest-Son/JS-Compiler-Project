@echo off
echo Building dataflow analysis example...

set SRC_DIR=src
set INCLUDE_DIR=include
set EXAMPLES_DIR=examples
set CXX=g++
set CXXFLAGS=-std=c++17 -I%INCLUDE_DIR%

:: Compile core files
%CXX% %CXXFLAGS% ^
  %SRC_DIR%\lexer.cpp ^
  %SRC_DIR%\token.cpp ^
  %SRC_DIR%\parser.cpp ^
  %SRC_DIR%\ast.cpp ^
  %SRC_DIR%\ast_printer.cpp ^
  %SRC_DIR%\cfg\control_flow_graph.cpp ^
  %SRC_DIR%\cfg\cfg_builder.cpp ^
  %SRC_DIR%\cfg\ssa_transformer.cpp ^
  %SRC_DIR%\cfg\dataflow_analyses.cpp ^
  %EXAMPLES_DIR%\dataflow_analysis_example.cpp ^
  -o dataflow_analysis_example

if %ERRORLEVEL% NEQ 0 (
  echo Build failed!
) else (
  echo Build successful!
)
