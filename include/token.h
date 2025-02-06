#ifndef TOKEN_H
#define TOKEN_H

#include <string>

//Enum for token types
enum TokenType {
    KEYWORD,
    IDENTIFIER,
    NUMBER,
    STRING,
    OPERATOR,
    SYMBOL,
    COMMENT,
    END_OF_FILE,
    ERROR
  };

//Token class

struct Token {
  public:
    TokenType type;
    std::string value;
    int line;

    Token(TokenType type, std::string value, int line)
    : type(type), value(value), line(line) {}
};


#endif //TOKEN_H
