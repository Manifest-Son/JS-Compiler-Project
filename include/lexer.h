#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "token.h"

class Lexer {
  public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize(); // Main function to tokenize input

  private:
    std::string source;
    size_t position = 0;
    int line = 1;

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
};

#endif // LEXER_H
