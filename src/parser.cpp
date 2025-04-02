#include "../include/parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0), start(0), line(1) {}

std::shared_ptr<Program> Parser::parse() {
    std::vector<std::shared_ptr<Statement>> statements;
    
    while (!isAtEnd()) {
        // Skip any comment tokens before parsing
        while (match(TokenType::COMMENT) && !isAtEnd()) {
            // Just skip the comment token
        }
        
        if (isAtEnd()) break;
        
        try {
            start = current; // Mark the start of each declaration
            statements.push_back(declaration());
        } catch (ParserError& error) {
            synchronize();
        }
    }
    
    return std::make_shared<Program>(statements);
}

// Utility methods
bool Parser::isAtEnd() {
    return peek().type == END_OF_FILE;
}

Token Parser::peek() {
    return tokens[current];
}

Token Parser::previous() {
    return tokens[current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) {
        current++;
        line = tokens[current-1].line; // Update line to current token's line
    }
    return previous();
}

bool Parser::check(TokenType type) {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (match(type)) return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw error(peek(), message);
}

ParserError Parser::error(Token token, const std::string& message) {
    std::cerr << "Error at line " << token.line;
    if (token.type == END_OF_FILE) {
        std::cerr << " at end";
    } else {
        std::cerr << " at '" << token.lexeme << "'";
    }
    std::cerr << ": " << message << std::endl;
    
    return ParserError(message);
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::SYMBOL && previous().lexeme == ";") return;
        
        switch (peek().type) {
            case TokenType::KEYWORD:
                if (peek().lexeme == "function" || 
                    peek().lexeme == "let" || 
                    peek().lexeme == "if" || 
                    peek().lexeme == "return" ||
                    peek().lexeme == "for" ||
                    peek().lexeme == "while") {
                    return;
                }
                break;
            default:
                break;
        }
        
        advance();
    }
}

// Parsing methods
std::shared_ptr<Statement> Parser::declaration() {
    start = current; // Mark the start of the declaration
    
    if (match(TokenType::KEYWORD) && previous().lexeme == "let") {
        return varDeclaration();
    }
    
    return statement();
}

std::shared_ptr<Statement> Parser::varDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
    
    std::shared_ptr<Expression> initializer = nullptr;
    if (match(TokenType::OPERATOR) && previous().lexeme == "=") {
        initializer = expression();
    }
    
    consume(TokenType::SYMBOL, "Expect ';' after variable declaration.");
    return std::make_shared<VarDeclStmt>(name, initializer);
}

std::shared_ptr<Statement> Parser::statement() {
    start = current; // Mark the start of the statement
    
    if (match(TokenType::KEYWORD) && previous().lexeme == "if") {
        return ifStatement();
    }
    
    if (match(TokenType::SYMBOL) && previous().lexeme == "{") {
        return block();
    }
    
    return expressionStatement();
}

std::shared_ptr<Statement> Parser::expressionStatement() {
    std::shared_ptr<Expression> expr = expression();
    consume(TokenType::SYMBOL, "Expect ';' after expression.");
    return std::make_shared<ExpressionStmt>(expr);
}

std::shared_ptr<Statement> Parser::ifStatement() {
    consume(TokenType::SYMBOL, "Expect '(' after 'if'.");
    std::shared_ptr<Expression> condition = expression();
    consume(TokenType::SYMBOL, "Expect ')' after if condition.");
    
    std::shared_ptr<Statement> thenBranch = statement();
    std::shared_ptr<Statement> elseBranch = nullptr;
    
    if (match(TokenType::KEYWORD) && previous().lexeme == "else") {
        elseBranch = statement();
    }
    
    return std::make_shared<IfStmt>(condition, thenBranch, elseBranch);
}

std::shared_ptr<Statement> Parser::block() {
    std::vector<std::shared_ptr<Statement>> statements;
    
    while (!check(TokenType::SYMBOL) && peek().lexeme != "}" && !isAtEnd()) {
        statements.push_back(declaration());
    }
    
    consume(TokenType::SYMBOL, "Expect '}' after block.");
    return std::make_shared<BlockStmt>(statements);
}

std::shared_ptr<Expression> Parser::expression() {
    start = current; // Mark the start of the expression
    return equality();
}

std::shared_ptr<Expression> Parser::equality() {
    std::shared_ptr<Expression> expr = comparison();
    
    while (match(TokenType::OPERATOR) && 
          (previous().lexeme == "==" || previous().lexeme == "!=")) {
        Token op = previous();
        std::shared_ptr<Expression> right = comparison();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::comparison() {
    std::shared_ptr<Expression> expr = term();
    
    while (match(TokenType::OPERATOR) && 
          (previous().lexeme == ">" || previous().lexeme == ">=" ||
           previous().lexeme == "<" || previous().lexeme == "<=")) {
        Token op = previous();
        std::shared_ptr<Expression> right = term();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::term() {
    std::shared_ptr<Expression> expr = factor();
    
    while (match(TokenType::OPERATOR) && 
          (previous().lexeme == "+" || previous().lexeme == "-")) {
        Token op = previous();
        std::shared_ptr<Expression> right = factor();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::factor() {
    std::shared_ptr<Expression> expr = unary();
    
    while (match(TokenType::OPERATOR) && 
          (previous().lexeme == "*" || previous().lexeme == "/")) {
        Token op = previous();
        std::shared_ptr<Expression> right = unary();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::unary() {
    if (match(TokenType::OPERATOR) && 
       (previous().lexeme == "!" || previous().lexeme == "-")) {
        Token op = previous();
        std::shared_ptr<Expression> right = unary();
        return std::make_shared<UnaryExpr>(op, right);
    }
    
    return primary();
}

std::shared_ptr<Expression> Parser::primary() {
    if (match(TokenType::KEYWORD)) {
        if (previous().lexeme == "true" || previous().lexeme == "false" || 
            previous().lexeme == "null") {
            return std::make_shared<LiteralExpr>(previous());
        }
    }
    
    if (match(TokenType::NUMBER) || match(TokenType::STRING)) {
        return std::make_shared<LiteralExpr>(previous());
    }
    
    if (match(TokenType::IDENTIFIER)) {
        return std::make_shared<VariableExpr>(previous());
    }
    
    if (match(TokenType::SYMBOL) && previous().lexeme == "(") {
        std::shared_ptr<Expression> expr = expression();
        consume(TokenType::SYMBOL, "Expect ')' after expression.");
        return expr;
    }
    
    throw error(peek(), "Expect expression.");
}

