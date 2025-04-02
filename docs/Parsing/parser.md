# Parser

The parser is responsible for analyzing the stream of tokens produced by the lexer and organizing them into a hierarchical structure known as an Abstract Syntax Tree (AST). This tree represents the syntactic structure of the program.

## Parsing Approach

Our parser implements a recursive descent parsing strategy, which is a top-down parsing technique where:

1. The parser starts with the highest-level grammar rule (the program)
2. It recursively processes nested structures according to the language grammar
3. Each grammar rule is implemented as a method in the parser

## Implementation

The parser has the following core structure:

```typescript
class Parser {
  constructor(tokens: Token[] | Lexer) {
    // Initialize with tokens or a lexer instance
  }

  // Parse the entire program
  parse(): Program {
    // Return the AST representing the program
  }

  // Helper methods for parsing specific constructs
  parseStatement(): Statement {
    // Parse a statement
  }

  parseExpression(): Expression {
    // Parse an expression
  }

  // Error handling and utility methods
  // ...
}
```

## AST Node Types

The parser constructs various types of AST nodes, including:

- **Program**: The root node representing the entire program
- **Statement**: Nodes for declarations, control flow, etc.
- **Expression**: Nodes for operations that produce values
- **Literal**: Nodes for constant values
- **Identifier**: Nodes for variable and function names

## Operator Precedence

The parser handles operator precedence to ensure expressions are evaluated correctly:

| Precedence | Operators | Associativity |
|------------|-----------|---------------|
| Highest | `()`, `[]`, `.` | Left-to-right |
| | `++`, `--`, `!`, `-` (unary) | Right-to-left |
| | `*`, `/`, `%` | Left-to-right |
| | `+`, `-` (binary) | Left-to-right |
| | `<`, `>`, `<=`, `>=` | Left-to-right |
| | `==`, `!=`, `===`, `!==` | Left-to-right |
| | `&&` | Left-to-right |
| | `\|\|` | Left-to-right |
| Lowest | `=`, `+=`, `-=`, etc. | Right-to-left |

## Error Recovery

The parser includes error recovery mechanisms to:

1. Report syntax errors with helpful messages
2. Skip problematic tokens to continue parsing
3. Avoid cascading errors by synchronizing at statement boundaries

## Example AST

For the source code:
```javascript
function add(a, b) {
  return a + b;
}
```

The parser would produce an AST structure similar to:
```javascript
{
  type: "Program",
  body: [
    {
      type: "FunctionDeclaration",
      id: {
        type: "Identifier",
        name: "add"
      },
      params: [
        {
          type: "Identifier",
          name: "a"
        },
        {
          type: "Identifier",
          name: "b"
        }
      ],
      body: {
        type: "BlockStatement",
        body: [
          {
            type: "ReturnStatement",
            argument: {
              type: "BinaryExpression",
              operator: "+",
              left: {
                type: "Identifier",
                name: "a"
              },
              right: {
                type: "Identifier",
                name: "b"
              }
            }
          }
        ]
      }
    }
  ]
}
```
