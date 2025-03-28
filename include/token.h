#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <utility>
#include <variant>

enum TokenType {
    IDENTIFIER,
    KEYWORD,
    STRING,
    NUMBER,
    SYMBOL,
    OPERATOR,
    COMMENT,
    ERROR,
    END_OF_FILE
};

class Token {
public:
    TokenType type;
    std::string lexeme;
    int line;
    std::variant<std::monostate, double, bool, std::string> value;

    // Constructor with 3 parameters (no value)
    Token(TokenType type, std::string lexeme, int line)
        : type(type), lexeme(std::move(lexeme)), line(line), value(std::monostate{}) {}
    
    // Constructor with 4 parameters for numeric values
    Token(TokenType type, std::string lexeme, int line, double numValue)
        : type(type), lexeme(std::move(lexeme)), line(line), value(numValue) {}
    
    // Constructor with 4 parameters for boolean values
    Token(TokenType type, std::string lexeme, int line, bool boolValue)
        : type(type), lexeme(std::move(lexeme)), line(line), value(boolValue) {}
    
    // Constructor with 4 parameters for string values
    Token(TokenType type, std::string lexeme, int line, const std::string& strValue)
        : type(type), lexeme(std::move(lexeme)), line(line), value(strValue) {}
    
    // Constructor with 4 parameters for monostate (null) values
    Token(TokenType type, std::string lexeme, int line, std::monostate)
        : type(type), lexeme(std::move(lexeme)), line(line), value(std::monostate{}) {}
};

#endif // TOKEN_H
