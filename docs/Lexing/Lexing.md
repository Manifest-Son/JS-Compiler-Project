# INTRODUCTION TO LEXING
## What is lexing and why is it important in compiler construction
**Lexing** scans through a list of characters and groups them together into the smallest sequences that represent lexemes.
The lexemes are only the raw substrings of the source code. When we take the lexeme and bundle it together with other data, the result is a token.
It includes types such as:
1. **Token Type**: The parser wants to know not just that it has a lexeme for some identifier, but that it has a reserved word, and which keyword it is.
2. **Literal Value**: There are lexemes for literal values: numbers and strings and the like.
3. **Local information**: Such as line number for error reporting.

## Tokens Handled by Our Lexer
1. **Keywords**: Full set of JavaScript reserved words including:
   - Standard keywords (`if`, `else`, `function`, etc.)
   - Future reserved words (`class`, `enum`, etc.)
   - Strict mode reserved words (`let`, `static`, etc.)
   - Special identifiers (`this`, `undefined`, etc.)
2. **Identifiers**: Variable and function names with proper JavaScript rules:
   - Can start with a letter, underscore (_) or dollar sign ($)
   - Can contain letters, digits, underscores, and dollar signs
   - Are case-sensitive
   - Cannot be a reserved word
3. **Numbers**: Integer and floating-point literals
4. **Strings**: Text enclosed in single or double quotes
5. **Operators**: Both single-character (+, -, *, /) and multi-character (==, !=, >=, <=, ++, --, etc.)
6. **Symbols**: Parentheses, braces, brackets, commas, semicolons
7. **Comments**: Both single-line (//) and multi-line (/* */) comments

## Data Structures and Algorithms
1. **Data Structure**: The lexer uses a vector to store tokens and a string to hold the source code.
2. **Algorithm**: The lexer scans through the source code character by character, grouping them into tokens based on predefined rules.
3. **Keyword Lookup**: Uses an unordered_set for O(1) lookup of reserved words

## Abstract Syntax Tree (AST)
After the lexical analysis phase, the parser constructs an Abstract Syntax Tree (AST) that represents the structured form of the program. Our AST implementation includes:

1. **Node Types**:
   - Expressions (literals, variables, binary operations, unary operations)
   - Statements (expression statements, variable declarations, blocks, if statements)
   - Program (root node containing all statements)

2. **Visitor Pattern**:
   - The AST supports a visitor pattern for traversing and performing operations on the tree
   - Allows for clean separation between tree structure and operations performed on it

3. **AST Printer**:
   - Visualizes the structure of the parsed program
   - Uses indentation to show nesting relationships between nodes

4. **Benefits**:
   - Provides a structured representation of the program
   - Makes semantic analysis and code generation more straightforward
   - Enables easy traversal and transformation of the program

## Optimizations
- Skipping whitespace efficiently
- Handling multi-character operators
- Proper line counting for error reporting
- Escaped character handling in strings
- Fast identifier and keyword classification

## Error Handling
- Detection of unterminated strings and comments
- Reporting unexpected characters with line information
- Creating error tokens to allow parsing to continue
- Properly flagging invalid identifiers
- Parser synchronization after errors to continue analysis

## Performance
- The lexer processes the source code in a single pass, making it efficient.
- Uses direct character comparisons rather than regular expressions for better performance.
- Efficient lookup of keywords using hash-based containers
- Memory-efficient AST construction using shared pointers

## Conclusions
Lexing is a crucial first step in the compilation process, transforming raw source code into a structured format that the parser can work with. Our enhanced lexer now handles more JavaScript features including comments, complex operators, and full JavaScript identifier and keyword rules, making it more robust for real-world code. The parser builds an AST that provides a structured representation of the program, enabling further analysis and transformation in subsequent compilation phases.
