#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "token.h"
#include "error_reporter.h"

// Utility functions for checking valid identifier characters
bool isIdentifierStart(char c);
bool isIdentifierPart(char c);

class Lexer {
  public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize(); // Main function to tokenize input
    ErrorReporter& getErrorReporter() { return errorReporter; }

  private:
    std::string source;
    size_t position = 0;
    int line = 1;
    int column = 1;  // Add column tracking for better error locations
    ErrorReporter errorReporter;

    char peek(); // Look at the current character
    char peekNext(); // Look at the next character
    char advance(); // Move to next character
    void skipWhitespace();
    Token identifier();
    Token string();
    Token symbol();
    Token number();
    Token handleComment(); // New method for comments
    Token errorToken(const std::string& message);
    bool match(char expected); // New helper method for matching characters
    
    // Track token start position for error reporting
    int startLine;
    int startColumn;
    void startToken();
};

#endif // LEXER_H
