#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory>
#include <stdexcept>
#include "token.h"
#include "ast.h"

// Parser error exception
class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& message) : std::runtime_error(message) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    
    // Parse the entire program
    std::shared_ptr<Program> parse();

private:
    const std::vector<Token>& tokens;
    size_t current = 0;
    size_t start = 0;  // Start position for current expression/statement
    int line = 1;      // Current line number for error reporting

    // Utility methods
    bool isAtEnd();
    Token peek();
    Token previous();
    Token advance();
    bool check(TokenType type);
    bool match(TokenType type);
    bool match(const std::vector<TokenType>& types);
    Token consume(TokenType type, const std::string& message);
    ParseError error(Token token, const std::string& message);
    void synchronize();

    // Recursive descent parsing methods
    std::shared_ptr<Statement> declaration();
    std::shared_ptr<Statement> varDeclaration();
    std::shared_ptr<Statement> statement();
    std::shared_ptr<Statement> expressionStatement();
    std::shared_ptr<Statement> ifStatement();
    std::shared_ptr<Statement> block();
    
    std::shared_ptr<Expression> expression();
    std::shared_ptr<Expression> equality();
    std::shared_ptr<Expression> comparison();
    std::shared_ptr<Expression> term();
    std::shared_ptr<Expression> factor();
    std::shared_ptr<Expression> unary();
    std::shared_ptr<Expression> primary();
};

#endif // PARSER_H
