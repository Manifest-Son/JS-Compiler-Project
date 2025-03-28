# INTRODUCTION TO LEXING
## What is lexing and why is it important in compiler construction
**Lexing** scans through a list of characters and groups them together into the smallest sequences that represent lexemes.
The lexemes are only the raw substrings of the source code. When we take the lexeme and bundle it together with other data, the result is a token.
It includes types such as:
1. **Token Type**: The parser wants to know not just that it has a lexeme for some identifier, but that it has a reserved word, and which keyword it is.
2. **Literal Value**: There are lexemes for literal values: numbers and strings and the like.
3. **Local information**: Such as line number for error reporting.

## Tokens Handled by Our Lexer
1. **Keywords**: Reserved words in JavaScript (let, if, else, function, etc.)
2. **Identifiers**: Variable and function names
3. **Numbers**: Integer and floating-point literals
4. **Strings**: Text enclosed in single or double quotes
5. **Operators**: Both single-character (+, -, *, /) and multi-character (==, !=, >=, <=, ++, --, etc.)
6. **Symbols**: Parentheses, braces, brackets, commas, semicolons
7. **Comments**: Both single-line (//) and multi-line (/* */) comments

## Data Structures and Algorithms
1. **Data Structure**: The lexer uses a vector to store tokens and a string to hold the source code.
2. **Algorithm**: The lexer scans through the source code character by character, grouping them into tokens based on predefined rules.

## Optimizations
- Skipping whitespace efficiently
- Handling multi-character operators
- Proper line counting for error reporting
- Escaped character handling in strings

## Error Handling
- Detection of unterminated strings and comments
- Reporting unexpected characters with line information
- Creating error tokens to allow parsing to continue

## Performance
- The lexer processes the source code in a single pass, making it efficient.
- Uses direct character comparisons rather than regular expressions for better performance.

## Conclusions
Lexing is a crucial first step in the compilation process, transforming raw source code into a structured format that the parser can work with. Our enhanced lexer now handles more JavaScript features including comments and complex operators, making it more robust for real-world code.
