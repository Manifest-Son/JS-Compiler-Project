/**
 * Parser class for JavaScript source code
 *
 * This class implements a recursive descent parser for JavaScript,
 * converting a stream of tokens into an Abstract Syntax Tree (AST).
 */

#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "ast.h"
#include "error_reporter.h"
#include "token.h"

// Forward declarations to avoid circular dependencies
class Program;
class Expression;
class Statement;

class Parser {
public:
    Parser(std::vector<Token> tokens, ErrorReporter &errorReporter);
    std::unique_ptr<Program> parse();

private:
    std::vector<Token> tokens;
    ErrorReporter &errorReporter;
    int current = 0;

    // Parsing methods for different syntactic constructs
    std::shared_ptr<Statement> declaration();
    std::shared_ptr<Statement> varDeclaration();
    std::shared_ptr<Statement> functionDeclaration();
    std::shared_ptr<Statement> statement();
    std::shared_ptr<Statement> expressionStatement();
    std::shared_ptr<Statement> ifStatement();
    std::shared_ptr<Statement> whileStatement();
    std::shared_ptr<Statement> forStatement();
    std::shared_ptr<Statement> returnStatement();
    std::shared_ptr<Statement> blockStatement();

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
    std::shared_ptr<Expression> finishCall(std::shared_ptr<Expression> callee);

    // Helper methods for parsing
    bool match(std::initializer_list<TokenType> types);
    bool check(TokenType type) const;
    Token advance();
    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    Token consume(TokenType type, const std::string &message);
    void synchronize();
};

#endif // PARSER_H
