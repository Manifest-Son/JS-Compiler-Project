# Tokens

Tokens are the smallest units of meaning in a programming language. They represent the building blocks that make up the syntax of the language.

## Token Structure

Each token in our compiler consists of:

- **Type**: Identifies the category of the token (e.g., Keyword, Identifier, Operator)
- **Value**: The actual text from the source code
- **Position**: Line and column information for error reporting

```typescript
interface Token {
  type: TokenType;
  value: string;
  line: number;
  column: number;
}
```

## Token Types

Our compiler recognizes the following token types:

| Token Type | Description | Examples |
|------------|-------------|----------|
| Keyword | Reserved words with special meaning | `if`, `function`, `return` |
| Identifier | Names of variables, functions, etc. | `foo`, `myVariable` |
| StringLiteral | String values | `"hello"`, `'world'` |
| NumericLiteral | Number values | `42`, `3.14` |
| Operator | Arithmetic, comparison, and logical operators | `+`, `===`, `&&` |
| Punctuator | Punctuation marks with syntactic meaning | `;`, `{`, `}` |
| Comment | Code comments | `// comment`, `/* comment */` |
| EOF | End of file marker | N/A |

## Example

Source code:
```javascript
let x = 10;
```

Tokenized as:
```javascript
[
  { type: 'Keyword', value: 'let', line: 1, column: 1 },
  { type: 'Identifier', value: 'x', line: 1, column: 5 },
  { type: 'Operator', value: '=', line: 1, column: 7 },
  { type: 'NumericLiteral', value: '10', line: 1, column: 9 },
  { type: 'Punctuator', value: ';', line: 1, column: 11 },
  { type: 'EOF', value: '', line: 1, column: 12 }
]
```
