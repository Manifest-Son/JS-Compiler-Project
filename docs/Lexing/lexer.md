# Lexer

The lexer (also known as a tokenizer or scanner) is responsible for breaking down source code into tokens. It's the first phase of the compilation process.

## Functionality

The lexer performs the following tasks:

1. Reads the input source code character by character
2. Identifies tokens according to language rules
3. Categorizes each token by type
4. Records position information for error reporting
5. Filters out whitespace and comments (unless configured to preserve them)
6. Produces a linear sequence of tokens for the parser

## Implementation

Our lexer is implemented as a class with the following core methods:

```typescript
class Lexer {
  constructor(source: string) {
    // Initialize with source code and position tracking
  }

  // Get the next token from the source
  nextToken(): Token {
    // Identify and return the next token
  }

  // Check if we've reached the end of the source
  isEOF(): boolean {
    // Return true if no more tokens are available
  }

  // Get all tokens at once
  tokenize(): Token[] {
    // Return array of all tokens in the source
  }
}
```

## Lexical Rules

The lexer follows these rules to identify tokens:

- **Identifiers**: Start with a letter or underscore, followed by letters, digits, or underscores
- **Keywords**: Matched against a predefined list of reserved words
- **String Literals**: Enclosed in single or double quotes
- **Numeric Literals**: Sequences of digits, optionally with a decimal point
- **Operators**: Matched against a list of recognized operator symbols
- **Punctuators**: Special characters like parentheses, braces, and semicolons

## Error Handling

The lexer provides detailed error messages for invalid input, including:

- Unterminated string literals
- Invalid numeric literals
- Unrecognized characters
- Unexpected end of input

Each error includes the line and column position for easy debugging.

## Example Usage

```typescript
const source = `function add(a, b) { return a + b; }`;
const lexer = new Lexer(source);
const tokens = lexer.tokenize();

// Output all tokens
tokens.forEach(token => {
  console.log(`${token.type}: '${token.value}' at line ${token.line}, column ${token.column}`);
});
```
