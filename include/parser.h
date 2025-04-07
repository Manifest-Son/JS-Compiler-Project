#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include "token.h"
#include "ast.h"
#include "lexer.h"

// Enhanced parser error with line, column, and suggestion
class ParserError : public std::runtime_error {
public:
    int line;
    int column;
    std::string suggestion;

    ParserError(const std::string& message, int line = 0, int column = 0, 
                const std::string& suggestion = "")
        : std::runtime_error(message), line(line), column(column), suggestion(suggestion) {}
};

class Parser {
public:
    Parser(Lexer&); //Used
    explicit Parser(const std::vector<Token>& tokens);
    std::shared_ptr<Program> parse();
    bool isAtEnd();
    Token peek();
    Token previous();
    void parseProgram();
    static void fromString(const std::string & string);

private:
    std::vector<Token> tokens;
    size_t current = 0;
    int start;int line;

    // Helper methods
    Token peek() const;
    Token previous() const;
    bool isAtEnd() const;
    Token advance();
    bool check(TokenType type);
    bool match(TokenType type);
    bool match(const std::vector<TokenType> &types);

    Token consume(TokenType type, const std::string& message);
    
    // Error handling
    static ParserError error(Token token, const std::string &message);
    void synchronize();
    std::shared_ptr<Statement> declaration();
    std::shared_ptr<Statement> varDeclaration();
    std::string getErrorSuggestion(const std::string& message, const Token& token);

    // Grammar rules
    std::shared_ptr<Program> program();
    std::shared_ptr<Statement> statement();
    std::shared_ptr<Statement> expressionStatement();
    std::shared_ptr<Statement> functionDeclaration();
    std::shared_ptr<Statement> variableDeclaration();
    std::shared_ptr<Statement> blockStatement();
    std::shared_ptr<Statement> ifStatement();
    std::shared_ptr<Statement> block();

    std::shared_ptr<Expression> expression();
    std::shared_ptr<Expression> assignment();
    std::shared_ptr<Expression> logicalOr();
    std::shared_ptr<Expression> logicalAnd();
    std::shared_ptr<Expression> equality();
    std::shared_ptr<Expression> comparison();
    std::shared_ptr<Expression> term();
    std::shared_ptr<Expression> factor();
    std::shared_ptr<Expression> unary();
    std::shared_ptr<Expression> call();
    std::shared_ptr<Expression> primary();
};

#endif // PARSER_H
