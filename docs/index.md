# JavaScript Compiler Documentation

This documentation covers the core components of our JavaScript compiler implementation.

## Components

- [Lexer](Lexing/lexer.md) - Converts source code into tokens
- [Tokens](Parsing/tokens.md) - Defines the token structures used throughout the compiler
- [Parser](Parsing/parser.md) - Transforms tokens into an abstract syntax tree (AST)

## Compilation Pipeline

1. **Lexical Analysis**: The lexer scans the source code and produces a stream of tokens
2. **Syntax Analysis**: The parser consumes tokens and constructs an AST
3. **Semantic Analysis**: The AST is analyzed for semantic correctness (not covered in these docs)
4. **Code Generation**: The AST is transformed into target code (not covered in these docs)

## Getting Started

To understand the compilation process, start by reading about [tokens](Parsing/tokens.md), then the [lexer](Lexing/lexer.md), and finally the [parser](Parsing/parser.md).
