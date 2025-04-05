# Lexer Optimization Techniques

This document outlines strategies for optimizing the lexical analysis phase of our JavaScript compiler, focusing on both performance and memory efficiency.

## Table of Contents
1. [Introduction](#introduction)
2. [Time Complexity Optimizations](#time-complexity-optimizations)
3. [Space Complexity Optimizations](#space-complexity-optimizations)
4. [Input Handling Strategies](#input-handling-strategies)
5. [Implementation Examples](#implementation-examples)

## Introduction

Lexical analysis is the first phase of compilation and can significantly impact overall compiler performance. As the entry point to the pipeline, any inefficiencies here will affect all subsequent stages.

## Time Complexity Optimizations

### Efficient Character Processing

1. **Fast Character Classification**
   - Use lookup tables for character classification instead of multiple conditions
   - Optimize common character tests with bitsets or lookup arrays

```cpp
// Before optimization
bool isIdentifierStart(char c) {
    return (c >= 'a' && c <= 'z') || 
           (c >= 'A' && c <= 'Z') || 
           c == '_';
}

// After optimization using lookup table
bool isIdentifierStart(char c) {
    static constexpr bool lookup[256] = {
        0, 0, 0, 0, /* ... initialize table ... */
        1, 1, /* 'A' to 'Z' set to 1 */
        /* ... more initialization ... */
        1, 1, /* 'a' to 'z' set to 1 */
        /* ... */
    };
    return lookup[static_cast<unsigned char>(c)];
}
```

2. **Finite State Machine Implementation**
   - Implement a table-driven state machine for token recognition
   - Pre-compute transitions to avoid conditional logic

```cpp
enum class LexerState {
    START, 
    IDENTIFIER,
    NUMBER,
    STRING,
    // Other states...
};

// Table-driven approach
LexerState nextState[NUM_STATES][256]; // Precomputed transitions

// Initialize the transition table
void initTransitionTable() {
    // Set all to error state by default
    for (int i = 0; i < NUM_STATES; i++) {
        for (int j = 0; j < 256; j++) {
            nextState[i][j] = LexerState::ERROR;
        }
    }
    
    // Define valid transitions
    for (char c = 'a'; c <= 'z'; c++) {
        nextState[static_cast<int>(LexerState::START)][c] = LexerState::IDENTIFIER;
        nextState[static_cast<int>(LexerState::IDENTIFIER)][c] = LexerState::IDENTIFIER;
    }
    
    // Additional transitions...
}
```

3. **Specialized Token Handlers**
   - Implement dedicated routines for common token types
   - Optimize the most frequently occurring patterns

### Token Recognition Speedups

1. **Keyword Recognition**
   - Use perfect hash functions or tries for keyword lookup
   - Consider specialized data structures for keyword matching

```cpp
// Trie implementation for keyword recognition
class KeywordTrie {
private:
    struct Node {
        bool isEndOfKeyword = false;
        TokenType keywordType;
        std::array<std::unique_ptr<Node>, 26> children;
    };
    
    std::unique_ptr<Node> root = std::make_unique<Node>();
    
public:
    KeywordTrie() {
        // Insert all keywords
        insert("let", TokenType::KEYWORD_LET);
        insert("if", TokenType::KEYWORD_IF);
        insert("else", TokenType::KEYWORD_ELSE);
        // ...
    }
    
    void insert(const std::string& keyword, TokenType type) {
        Node* current = root.get();
        for (char c : keyword) {
            int index = c - 'a';
            if (index < 0 || index >= 26) continue; // Skip non-lowercase letters
            
            if (!current->children[index]) {
                current->children[index] = std::make_unique<Node>();
            }
            current = current->children[index].get();
        }
        current->isEndOfKeyword = true;
        current->keywordType = type;
    }
    
    std::optional<TokenType> lookup(const std::string& word) {
        Node* current = root.get();
        for (char c : word) {
            int index = c - 'a';
            if (index < 0 || index >= 26 || !current->children[index]) {
                return std::nullopt;
            }
            current = current->children[index].get();
        }
        
        if (current->isEndOfKeyword) {
            return current->keywordType;
        }
        return std::nullopt;
    }
};
```

2. **Parallel Scanning**
   - For very large input files, consider parallel tokenization of different sections
   - Merge results with special attention to section boundaries

## Space Complexity Optimizations

### Token Representation

1. **Compact Token Structure**
   - Minimize the size of token objects
   - Use bit fields and smaller integer types where appropriate

```cpp
// Before optimization
struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    std::variant<std::monostate, double, bool, std::string> value;
};

// After optimization
struct Token {
    TokenType type;            // 4 bytes (could be reduced to 1-2 bytes)
    uint32_t lexeme_index;     // Index into string table instead of storing string
    uint32_t position;         // Packed line/column: high 20 bits = line, low 12 bits = column
    // Value handled separately through a value table or other structure
};
```

2. **String Data Management**
   - Avoid storing duplicate string data
   - Use a string interning system for token lexemes

3. **Value Handling**
   - Separate token structure from literal values
   - Store complex values (strings, numbers) in a separate pool

### Memory Allocation Strategies

1. **Preallocated Token Arrays**
   - Pre-allocate token storage based on input size estimates
   - Grow in larger chunks to reduce allocation overhead

```cpp
class Lexer {
private:
    // Preallocate token storage (estimate 1 token per 8 characters of source)
    void reserveTokenStorage(size_t sourceSize) {
        tokens.reserve(sourceSize / 8);
    }
    
    std::vector<Token> tokens;
};
```

2. **Buffer Management**
   - Process input in chunks for large files
   - Implement a sliding window approach for streaming input

## Input Handling Strategies

### Efficient Input Reading

1. **Memory Mapping**
   - Use memory-mapped files for large inputs
   - Leverage OS-level optimizations for file access

```cpp
// Memory-mapped file example
class MappedFileInput {
private:
#ifdef _WIN32
    HANDLE file;
    HANDLE mapping;
#else
    int fd;
#endif
    const char* data;
    size_t size;
    
public:
    MappedFileInput(const std::string& path) {
#ifdef _WIN32
        file = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        mapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
        data = static_cast<const char*>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0));
        size = GetFileSize(file, NULL);
#else
        fd = open(path.c_str(), O_RDONLY);
        struct stat sb;
        fstat(fd, &sb);
        size = sb.st_size;
        data = static_cast<const char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
#endif
    }
    
    ~MappedFileInput() {
#ifdef _WIN32
        UnmapViewOfFile(data);
        CloseHandle(mapping);
        CloseHandle(file);
#else
        munmap(const_cast<char*>(data), size);
        close(fd);
#endif
    }
    
    // Access methods...
};
```

2. **Buffering Strategies**
   - Implement efficient buffering for sequential reads
   - Consider double-buffering for overlapped I/O

### Character Handling

1. **UTF-8 Processing**
   - Optimize UTF-8 character handling for JavaScript source
   - Use lookup tables for UTF-8 sequence recognition

2. **Line/Column Tracking**
   - Efficient tracking of source positions
   - Optimize newline detection and line counting

## Implementation Examples

### Optimized Tokenize Function

```cpp
std::vector<Token> Lexer::tokenize() {
    // Reserve capacity based on estimated token count
    tokens.reserve(source.length() / 8);
    
    while (position < source.length()) {
        char c = source[position];
        
        // Fast path for common cases
        if (isWhitespace(c)) {
            skipWhitespace();
            continue;
        }
        
        // Use jump table for token start detection
        switch (c) {
            case '(':
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
            case ';':
            case ',':
                tokens.push_back(createSimpleToken(c));
                position++;
                continue;
                
            case '"':
            case '\'':
                tokens.push_back(scanString());
                continue;
                
            // Other cases...
        }
        
        // Handle identifiers and numbers
        if (isDigit(c)) {
            tokens.push_back(scanNumber());
        } else if (isIdentifierStart(c)) {
            tokens.push_back(scanIdentifier());
        } else {
            // Handle error or unknown character
            tokens.push_back(createErrorToken());
            position++;
        }
    }
    
    // Add EOF token
    tokens.push_back(createEOFToken());
    return tokens;
}
```

### Optimized String Scanning

```cpp
Token Lexer::scanString() {
    char quote = source[position++]; // Skip opening quote
    size_t start = position;
    std::string value;
    value.reserve(16); // Preallocate reasonable size
    
    while (position < source.length() && source[position] != quote) {
        if (source[position] == '\\' && position + 1 < source.length()) {
            // Handle escape sequences
            position++;
            switch (source[position]) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                // Other escape sequences...
                default: value += source[position];
            }
        } else {
            value += source[position];
        }
        position++;
    }
    
    if (position >= source.length()) {
        // Unterminated string
        return createErrorToken("Unterminated string literal");
    }
    
    position++; // Skip closing quote
    return createStringToken(value);
}
```

## Conclusion

By implementing these lexer optimization techniques, we can significantly improve the performance and memory efficiency of the initial phase of compilation. These optimizations lay the groundwork for a more efficient overall compilation process, as all subsequent phases depend on the output of the lexer.

The most effective optimization strategy will depend on the specific characteristics of your input, so it's recommended to profile different approaches and focus on the areas that yield the greatest benefits.