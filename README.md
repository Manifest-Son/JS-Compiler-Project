# JavaScript Compiler Project

A modern JavaScript compiler implemented with LLVM backend and Rust memory management.

## Project Overview

This JavaScript compiler transforms JavaScript source code into optimized machine code using LLVM IR as an intermediate representation. The compiler incorporates advanced techniques such as:

- NaN-boxing for efficient JavaScript value representation
- Shape-based object property optimization
- Generational garbage collection
- Static Single Assignment (SSA) form for advanced optimizations
- Control flow graph transformation for data flow analysis

## Architecture

The compiler consists of several key components:

1. **Frontend**: Lexer and parser written in C++
2. **AST**: Abstract Syntax Tree representation
3. **CFG**: Control Flow Graph construction and optimization
4. **LLVM Backend**: Code generation using LLVM infrastructure
5. **Memory Management**: Rust-based memory management with garbage collection
6. **Runtime Library**: Support functions for JavaScript semantics

## Building the Project

### Prerequisites

- CMake 3.15+
- LLVM 14.0+
- Rust 1.60+
- C++17 compatible compiler

### Build Steps

```bash
# Clone the repository
git clone https://github.com/Manifest-Son/JS-Compiler-Project.git
cd JS-Compiler-Project

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build .
```

## Usage

```bash
# Compile a JavaScript file
./jscompiler input.js -o output

# Run benchmarks
./js_benchmark_runner [-o results.csv] [-i 10] [test_files...]

# Compile with different optimization levels
./jscompiler input.js -o output -O2
```

## Performance Benchmarking

The project includes a comprehensive benchmarking framework to evaluate the compiler's performance across different stages:

### Running Benchmarks

```bash
cd build
./js_benchmark_runner -i 10 -o benchmark_results.csv ../examples/dataflow_test.js
```

### Benchmark Categories

1. **Lexical Analysis**: Measures token generation performance
2. **Parsing**: Evaluates AST construction speed
3. **Code Generation**: Tests LLVM IR generation performance
4. **Optimization**: Measures the effect of different optimization levels
5. **End-to-End**: Full compilation pipeline from source to machine code
6. **Memory Management**: Evaluates GC performance and memory usage
7. **String Interning**: Tests string deduplication efficiency

### Sample Results

The following are sample benchmark results running on typical hardware:

| Benchmark           | Time (ms) | Memory (KB) | Improvement over Baseline |
|---------------------|-----------|-------------|---------------------------|
| Lexer               | 1.2       | 145         | -                         |
| Parser              | 3.5       | 620         | -                         |
| LLVM CodeGen (O0)   | 12.8      | 1450        | 1.0x (baseline)           |
| LLVM CodeGen (O1)   | 14.5      | 1550        | 1.3x faster execution     |
| LLVM CodeGen (O2)   | 18.2      | 1680        | 2.1x faster execution     |
| LLVM CodeGen (O3)   | 25.4      | 1720        | 2.4x faster execution     |
| End-to-End (O2)     | 35.6      | 2100        | -                         |
| String Interning    | 0.8       | 280         | 65% memory reduction      |

### Custom Benchmarks

You can create custom benchmarks by:

1. Creating JavaScript test files
2. Running the benchmark tool with your files
3. Analyzing the results in the generated CSV

## Documentation

Comprehensive documentation is available in the `docs/` directory:

- Architecture overview
- Memory management design
- NaN-boxing implementation
- Control flow graph transformation
- Optimization passes
- Benchmarking methodology

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
