#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "token.h"

class Lexer {
  public:
    Lexer(const std::string& source);
    std::vector<Token> tokenize(); //Main function to tokenize input

    private:
      std::string source;
      size_t position = 0;
      int line = 1;

      char peek(); //Look at the current character
      char advance(); //Move to next character
      void skipWhitespace();
      Token identifier();
      Token string();
      Token symbol();
      Token number();
      // Token skipComment();
      // Token Operator();
      Token errorToken(const std::string& message);
};

#endif //LEXER_H