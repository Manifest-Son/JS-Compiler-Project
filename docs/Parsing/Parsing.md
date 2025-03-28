# INTRODUCTION TO PARSING

## What is parsing and why is it important in compiler construction

**Parsing** is the process of analyzing a sequence of tokens to determine its grammatical structure with respect to a formal grammar. In compiler construction, parsing follows lexical analysis (lexing) and produces a parse tree or abstract syntax tree (AST) that represents the structure of the program according to the language's grammar.

The parser is important because it:
1. Verifies that the program follows the syntax rules of the language
2. Constructs a structured representation (AST) that can be used for semantic analysis and code generation
3. Reports syntax errors with meaningful messages

## Recursive Descent Parsing

Our parser uses the **recursive descent** technique, which is a top-down parsing strategy where we have a set of functions that each handle a specific grammar production. The functions call each other recursively to parse nested constructs.

Key features of our recursive descent parser:
- Each grammar rule corresponds to a function in the parser
- Functions call each other according to the grammar structure
- Error handling with synchronization to recover from syntax errors

## Grammar for JavaScript Subset

Our parser handles a subset of JavaScript grammar, including:

```
program     → declaration* EOF ;
declaration → varDecl | statement ;
varDecl     → "let" IDENTIFIER ("=" expression)? ";" ;
statement   → exprStmt | ifStmt | block ;
exprStmt    → expression ";" ;
ifStmt      → "if" "(" expression ")" statement ("else" statement)? ;
block       → "{" declaration* "}" ;
expression  → equality ;
equality    → comparison (("==" | "!=") comparison)* ;
comparison  → term ((">" | ">=" | "<" | "<=") term)* ;
term        → factor (("+" | "-") factor)* ;
factor      → unary (("*" | "/") unary)* ;
unary       → ("!" | "-") unary | primary ;
primary     → NUMBER | STRING | "true" | "false" | "null" | IDENTIFIER | "(" expression ")" ;
```

## Abstract Syntax Tree (AST) Structure

The AST represents the hierarchical structure of the program with nodes for:
- Expressions (literals, variables, binary operations, unary operations)
- Statements (expression statements, variable declarations, blocks, if statements)
- The program as a whole (collection of statements)

## Error Handling

The parser includes robust error handling:
1. **Synchronization**: When an error is detected, the parser can skip ahead to a well-defined synchronization point
2. **Error reporting**: Errors include the line number and context for easier debugging
3. **Error recovery**: The parser can continue after errors to report multiple issues in a single pass

## Conclusions

Parsing is a critical step in the compilation process that bridges the gap between the flat token stream and a structured representation of the program. Our recursive descent parser builds an AST that captures the hierarchical nature of the program, allowing for subsequent analysis and code generation.
