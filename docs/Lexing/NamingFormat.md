
# Naming Format for Lexer

## Token Types
- `KEYWORD`: Reserved words in JavaScript (e.g., `let`, `if`, `else`).
- `IDENTIFIER`: Names for variables, functions, etc.
- `NUMBER`: Numeric literals (integers and floats).
- `STRING`: String literals.
- `SYMBOL`: Single-character symbols (e.g., `(`, `)`, `{`, `}`, `[`, `]`).
- `OPERATOR`: Multi-character operators (e.g., `==`, `<=`, `>=`).
- `ERROR`: Tokens representing lexing errors.
- `END_OF_FILE`: End of the input source.

## Functions
- `tokenize()`: Main function to tokenize the input source.
- `peek()`: Look at the current character without advancing.
- `advance()`: Advance to the next character.
- `skipWhitespace()`: Skip whitespace characters and track new lines.
- `string()`: Handle string literals.
- `identifier()`: Handle identifiers and keywords.
- `number()`: Handle numeric literals.
- `symbol()`: Handle symbols and operators.
- `errorToken()`: Handle lexing errors.
