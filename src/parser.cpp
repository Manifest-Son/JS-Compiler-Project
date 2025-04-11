#include "../include/parser.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include "../include/error_reporter.h"

Parser::Parser(std::vector<Token> tokens, ErrorReporter &errorReporter) :
    tokens(std::move(tokens)), errorReporter(errorReporter), current(0) {}

std::unique_ptr<Program> Parser::parse() {
    // Create a new Program node to hold all the statements
    auto program = std::make_unique<Program>(std::vector<std::shared_ptr<Statement>>{});

    while (!isAtEnd()) {
        // Skip any comment tokens before parsing
        while (match({TokenType::COMMENT}) && !isAtEnd()) {
        }
        if (isAtEnd())
            break;
        try {
            // Parse each top-level declaration and add it to the Program
            program->statements.push_back(declaration());
        } catch (const std::runtime_error& e) {
            errorReporter.error(peek().line, e.what());
            synchronize();
        }
    }

    return program;
}

// Parsing methods
std::shared_ptr<Statement> Parser::declaration() {
    if (match({TokenType::KEYWORD}) && previous().lexeme == "let") {
        return varDeclaration();
    }

    if (match({TokenType::KEYWORD}) && previous().lexeme == "function") {
        return functionDeclaration();
    }

    return statement();
}

std::shared_ptr<Statement> Parser::varDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");

    std::shared_ptr<Expression> initializer = nullptr;
    if (match({TokenType::OPERATOR}) && previous().lexeme == "=") {
        initializer = expression();
    }

    consume(TokenType::SYMBOL, "Expect ';' after variable declaration.");
    return std::make_shared<VarDeclStmt>(name, initializer);
}

std::shared_ptr<Statement> Parser::functionDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect function name.");
    consume(TokenType::SYMBOL, "Expect '(' after function name.");

    std::vector<Token> parameters;
    if (!check(TokenType::SYMBOL) || peek().lexeme != ")") {
        do {
            parameters.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name."));
        } while (match({TokenType::SYMBOL}) && previous().lexeme == ",");
    }

    consume(TokenType::SYMBOL, "Expect ')' after parameters.");
    consume(TokenType::SYMBOL, "Expect '{' before function body.");

    std::vector<std::shared_ptr<Statement>> bodyStmts;

    while (!check(TokenType::SYMBOL) || peek().lexeme != "}" && !isAtEnd()) {
        bodyStmts.push_back(declaration());
    }

    consume(TokenType::SYMBOL, "Expect '}' after function body.");

    return std::make_shared<FunctionDeclStmt>(name, std::move(parameters), std::move(bodyStmts));
}

std::shared_ptr<Statement> Parser::statement() {
    if (match({TokenType::KEYWORD}) && previous().lexeme == "if") {
        return ifStatement();
    }

    if (match({TokenType::KEYWORD}) && previous().lexeme == "while") {
        return whileStatement();
    }

    if (match({TokenType::KEYWORD}) && previous().lexeme == "for") {
        return forStatement();
    }

    if (match({TokenType::KEYWORD}) && previous().lexeme == "return") {
        return returnStatement();
    }

    if (match({TokenType::SYMBOL}) && previous().lexeme == "{") {
        return blockStatement();
    }

    return expressionStatement();
}

std::shared_ptr<Statement> Parser::expressionStatement() {
    std::shared_ptr<Expression> expr = expression();
    consume(TokenType::SYMBOL, "Expect ';' after expression.");
    return std::make_shared<ExpressionStmt>(std::move(expr));
}

std::shared_ptr<Statement> Parser::ifStatement() {
    consume(TokenType::SYMBOL, "Expect '(' after 'if'.");
    std::shared_ptr<Expression> condition = expression();
    consume(TokenType::SYMBOL, "Expect ')' after if condition.");

    std::shared_ptr<Statement> thenBranch = statement();
    std::shared_ptr<Statement> elseBranch = nullptr;

    if (match({TokenType::KEYWORD}) && previous().lexeme == "else") {
        elseBranch = statement();
    }

    return std::make_shared<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::shared_ptr<Statement> Parser::whileStatement() {
    consume(TokenType::SYMBOL, "Expect '(' after 'while'.");
    std::shared_ptr<Expression> condition = expression();
    consume(TokenType::SYMBOL, "Expect ')' after condition.");

    std::shared_ptr<Statement> body = statement();

    return std::make_shared<WhileStmt>(std::move(condition), std::move(body));
}

std::shared_ptr<Statement> Parser::forStatement() {
    consume(TokenType::SYMBOL, "Expect '(' after 'for'.");

    // Initializer
    std::shared_ptr<Statement> initializer;
    if (match({TokenType::SYMBOL}) && previous().lexeme == ";") {
        initializer = nullptr;
    } else if (match({TokenType::KEYWORD}) && previous().lexeme == "let") {
        initializer = varDeclaration();
    } else {
        initializer = expressionStatement();
    }

    // Condition
    std::shared_ptr<Expression> condition = nullptr;
    if (!check(TokenType::SYMBOL) || peek().lexeme != ";") {
        condition = expression();
    }
    consume(TokenType::SYMBOL, "Expect ';' after loop condition.");

    // Increment
    std::shared_ptr<Expression> increment = nullptr;
    if (!check(TokenType::SYMBOL) || peek().lexeme != ")") {
        increment = expression();
    }
    consume(TokenType::SYMBOL, "Expect ')' after for clauses.");

    // Body
    std::shared_ptr<Statement> body = statement();

    // Desugar for loop into while loop with block
    if (increment != nullptr) {
        std::vector<std::shared_ptr<Statement>> bodyStmts;
        bodyStmts.push_back(std::move(body));
        bodyStmts.push_back(std::make_shared<ExpressionStmt>(std::move(increment)));
        body = std::make_shared<BlockStmt>(std::move(bodyStmts));
    }

    // Add the condition
    if (condition == nullptr) {
        // Create a "true" literal for infinite loops
        Token trueToken(TokenType::KEYWORD, "true", 0, "true");
        trueToken.type = TokenType::KEYWORD;
        trueToken.lexeme = "true";
        condition = std::make_shared<LiteralExpr>(trueToken);
    }
    body = std::make_shared<WhileStmt>(std::move(condition), std::move(body));

    // Add the initializer if present
    if (initializer != nullptr) {
        std::vector<std::shared_ptr<Statement>> blockStmts;
        blockStmts.push_back(std::move(initializer));
        blockStmts.push_back(std::move(body));
        body = std::make_shared<BlockStmt>(std::move(blockStmts));
    }

    return body;
}

std::shared_ptr<Statement> Parser::returnStatement() {
    Token keyword = previous();
    std::shared_ptr<Expression> value = nullptr;

    if (!check(TokenType::SYMBOL) || peek().lexeme != ";") {
        value = expression();
    }

    consume(TokenType::SYMBOL, "Expect ';' after return value.");
    return std::make_shared<ReturnStmt>(keyword, std::move(value));
}

std::shared_ptr<Statement> Parser::blockStatement() {
    std::vector<std::shared_ptr<Statement>> statements;

    while (!check(TokenType::SYMBOL) || peek().lexeme != "}" && !isAtEnd()) {
        statements.push_back(declaration());
    }

    consume(TokenType::SYMBOL, "Expect '}' after block.");
    return std::make_shared<BlockStmt>(std::move(statements));
}

// Expression parsing methods
std::shared_ptr<Expression> Parser::expression() { return assignment(); }

std::shared_ptr<Expression> Parser::assignment() {
    std::shared_ptr<Expression> expr = logicalOr();

    if (match({TokenType::OPERATOR}) && previous().lexeme == "=") {
        Token equals = previous();
        std::shared_ptr<Expression> value = assignment();

        if (auto *varExpr = dynamic_cast<VariableExpr *>(expr.get())) {
            Token name = varExpr->name;
            return std::make_shared<AssignExpr>(name, std::move(value));
        }

        errorReporter.error(equals.line, "Invalid assignment target.");
    }

    return expr;
}

std::shared_ptr<Expression> Parser::logicalOr() {
    std::shared_ptr<Expression> expr = logicalAnd();

    while (match({TokenType::OPERATOR}) && previous().lexeme == "||") {
        Token op = previous();
        std::shared_ptr<Expression> right = logicalAnd();
        expr = std::make_shared<LogicalExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::shared_ptr<Expression> Parser::logicalAnd() {
    std::shared_ptr<Expression> expr = equality();

    while (match({TokenType::OPERATOR}) && previous().lexeme == "&&") {
        Token op = previous();
        std::shared_ptr<Expression> right = equality();
        expr = std::make_shared<LogicalExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::shared_ptr<Expression> Parser::equality() {
    std::shared_ptr<Expression> expr = comparison();

    while (match({TokenType::OPERATOR}) && (previous().lexeme == "==" || previous().lexeme == "!=")) {
        Token op = previous();
        std::shared_ptr<Expression> right = comparison();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::shared_ptr<Expression> Parser::comparison() {
    std::shared_ptr<Expression> expr = term();

    while (match({TokenType::OPERATOR}) && (previous().lexeme == ">" || previous().lexeme == ">=" ||
                                            previous().lexeme == "<" || previous().lexeme == "<=")) {
        Token op = previous();
        std::shared_ptr<Expression> right = term();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::shared_ptr<Expression> Parser::term() {
    std::shared_ptr<Expression> expr = factor();

    while (match({TokenType::OPERATOR}) && (previous().lexeme == "+" || previous().lexeme == "-")) {
        Token op = previous();
        std::shared_ptr<Expression> right = factor();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::shared_ptr<Expression> Parser::factor() {
    std::shared_ptr<Expression> expr = unary();

    while (match({TokenType::OPERATOR}) && (previous().lexeme == "*" || previous().lexeme == "/")) {
        Token op = previous();
        std::shared_ptr<Expression> right = unary();
        expr = std::make_shared<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::shared_ptr<Expression> Parser::unary() {
    if (match({TokenType::OPERATOR}) && (previous().lexeme == "!" || previous().lexeme == "-")) {
        Token op = previous();
        std::shared_ptr<Expression> right = unary();
        return std::make_shared<UnaryExpr>(op, std::move(right));
    }

    return call();
}

std::shared_ptr<Expression> Parser::call() {
    std::shared_ptr<Expression> expr = primary();

    while (true) {
        if (match({TokenType::SYMBOL}) && previous().lexeme == "(") {
            expr = finishCall(std::move(expr));
        } else if (match({TokenType::OPERATOR}) && previous().lexeme == ".") {
            Token name = consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
            expr = std::make_shared<GetExpr>(std::move(expr), name);
        } else {
            break;
        }
    }

    return expr;
}

std::shared_ptr<Expression> Parser::finishCall(std::shared_ptr<Expression> callee) {
    std::vector<std::shared_ptr<Expression>> arguments;

    if (!check(TokenType::SYMBOL) || peek().lexeme != ")") {
        do {
            arguments.push_back(expression());
        } while (match({TokenType::SYMBOL}) && previous().lexeme == ",");
    }

    Token paren = consume(TokenType::SYMBOL, "Expect ')' after arguments.");

    return std::make_shared<CallExpr>(std::move(callee), paren, std::move(arguments));
}

std::shared_ptr<Expression> Parser::primary() {
    if (match({TokenType::KEYWORD}) && previous().lexeme == "true") {
        return std::make_shared<LiteralExpr>(previous());
    }

    if (match({TokenType::KEYWORD}) && previous().lexeme == "false") {
        return std::make_shared<LiteralExpr>(previous());
    }

    if (match({TokenType::KEYWORD}) && previous().lexeme == "null") {
        return std::make_shared<LiteralExpr>(previous());
    }

    if (match({TokenType::NUMBER})) {
        return std::make_shared<LiteralExpr>(previous());
    }

    if (match({TokenType::STRING})) {
        return std::make_shared<LiteralExpr>(previous());
    }

    if (match({TokenType::IDENTIFIER})) {
        return std::make_shared<VariableExpr>(previous());
    }

    if (match({TokenType::SYMBOL}) && previous().lexeme == "[") {
        std::vector<std::shared_ptr<Expression>> elements;

        if (!check(TokenType::SYMBOL) || peek().lexeme != "]") {
            do {
                elements.push_back(expression());
            } while (match({TokenType::SYMBOL}) && previous().lexeme == ",");
        }

        consume(TokenType::SYMBOL, "Expect ']' after array elements.");
        return std::make_shared<ArrayExpr>(std::move(elements));
    }

    if (match({TokenType::SYMBOL}) && previous().lexeme == "{") {
        std::vector<ObjectExpr::Property> properties;

        if (!check(TokenType::SYMBOL) || peek().lexeme != "}") {
            do {
                Token key = consume(TokenType::IDENTIFIER, "Expect property name.");
                consume(TokenType::OPERATOR, "Expect ':' after property name.");
                std::shared_ptr<Expression> value = expression();
                properties.push_back({key, std::move(value)});
            } while (match({TokenType::SYMBOL}) && previous().lexeme == ",");
        }

        consume(TokenType::SYMBOL, "Expect '}' after object properties.");
        return std::make_shared<ObjectExpr>(std::move(properties));
    }

    if (match({TokenType::SYMBOL}) && previous().lexeme == "(") {
        std::shared_ptr<Expression> expr = expression();
        consume(TokenType::SYMBOL, "Expect ')' after expression.");

        // Check if this is the start of an arrow function
        if (match({TokenType::OPERATOR}) && previous().lexeme == "=>") {
            // This is an arrow function with a single parameter
            std::vector<Token> parameters;

            // Extract the parameter from the grouping expression
            if (auto *varExpr = dynamic_cast<VariableExpr *>(expr.get())) {
                parameters.push_back(varExpr->name);

                // Parse the arrow function body
                if (match({TokenType::SYMBOL}) && previous().lexeme == "{") {
                    std::shared_ptr<Statement> body = blockStatement();
                    return std::make_shared<ArrowFunctionExpr>(std::move(parameters), std::move(body));
                } else {
                    std::shared_ptr<Expression> body = expression();
                    return std::make_shared<ArrowFunctionExpr>(std::move(parameters), std::move(body));
                }
            } else {
                errorReporter.error(peek().line, "Invalid arrow function parameter.");
            }
        }

        return std::make_shared<GroupingExpr>(std::move(expr));
    }

    // Handle arrow functions with multiple parameters
    if (match({TokenType::SYMBOL}) && previous().lexeme == "(") {
        std::vector<Token> parameters;

        if (!check(TokenType::SYMBOL) || peek().lexeme != ")") {
            do {
                parameters.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name."));
            } while (match({TokenType::SYMBOL}) && previous().lexeme == ",");
        }

        consume(TokenType::SYMBOL, "Expect ')' after parameters.");
        consume(TokenType::OPERATOR, "Expect '=>' for arrow function.");

        // Parse the arrow function body
        if (match({TokenType::SYMBOL}) && previous().lexeme == "{") {
            std::shared_ptr<Statement> body = blockStatement();
            return std::make_shared<ArrowFunctionExpr>(std::move(parameters), std::move(body));
        } else {
            std::shared_ptr<Expression> body = expression();
            return std::make_shared<ArrowFunctionExpr>(std::move(parameters), std::move(body));
        }
    }

    errorReporter.error(peek().line, "Expect expression.");
    throw std::runtime_error("Parser error: Expect expression.");
}

// Helper methods
bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type: types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd())
        return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!isAtEnd())
        current++;
    return previous();
}

bool Parser::isAtEnd() const { return peek().type == TokenType::END_OF_FILE; }

Token Parser::peek() const { return tokens[current]; }

Token Parser::previous() const { return tokens[current - 1]; }

Token Parser::consume(TokenType type, const std::string &message) {
    if (check(type))
        return advance();

    errorReporter.error(peek().line, message);
    throw std::runtime_error("Parser error: " + message);
}

void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        if (previous().type == TokenType::SYMBOL && previous().lexeme == ";")
            return;

        switch (peek().type) {
            case TokenType::KEYWORD:
                if (peek().lexeme == "function" || peek().lexeme == "let" || peek().lexeme == "if" ||
                    peek().lexeme == "return" || peek().lexeme == "for" || peek().lexeme == "while") {
                    return;
                }
                break;
            default:
                break;
        }

        advance();
    }
}

