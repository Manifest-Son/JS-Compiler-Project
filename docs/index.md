# JavaScript Compiler Documentation

This documentation covers the core components of our JavaScript compiler implementation.

## Components

- [Lexer](Lexing/lexer.md) - Converts source code into tokens
- [Tokens](Parsing/tokens.md) - Defines the token structures used throughout the compiler
- [Parser](Parsing/parser.md) - Transforms tokens into an abstract syntax tree (AST)
- [Optimization](Optimization/optimization.md) - Strategies for optimizing space and time complexity

## Compilation Pipeline

1. **Lexical Analysis**: The lexer scans the source code and produces a stream of tokens
2. **Syntax Analysis**: The parser consumes tokens and constructs an AST
3. **Semantic Analysis**: The AST is analyzed for semantic correctness
4. **Optimization**: The AST is transformed to improve space and time efficiency
5. **Code Generation**: The AST is transformed into target code

## Getting Started

To understand the compilation process, start by reading about [tokens](Parsing/tokens.md), then the [lexer](Lexing/lexer.md), and finally the [parser](Parsing/parser.md).

For performance considerations, check out our [optimization guide](Optimization/optimization.md).
