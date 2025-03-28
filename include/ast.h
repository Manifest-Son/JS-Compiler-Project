#ifndef AST_H
#define AST_H

#include <utility>
#include <vector>
#include <memory>
#include "token.h"

// Forward declarations
class Expression;
class Statement;

// Base AST node class
class ASTNode {
public:
    virtual ~ASTNode() = default;
};

// Expression node base class
class Expression : public ASTNode {
public:
    ~Expression() override = default;
};

// Literal expression (numbers, strings, booleans, null)
class LiteralExpr final : public Expression {
public:
    explicit LiteralExpr(Token token) : token(std::move(token)) {}
    Token token;
};

// Variable reference
class VariableExpr final : public Expression {
public:
    explicit VariableExpr(Token name) : name(std::move(name)) {}
    Token name;
};

// Binary operation (a + b, a * b, etc.)
class BinaryExpr final : public Expression {
public:
    BinaryExpr(std::shared_ptr<Expression> left, Token op, std::shared_ptr<Expression> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
    
    std::shared_ptr<Expression> left;
    Token op;
    std::shared_ptr<Expression> right;
};

// Unary operation (-a, !b, etc.)
class UnaryExpr final : public Expression {
public:
    UnaryExpr(Token op, std::shared_ptr<Expression> right)
        : op(std::move(op)), right(std::move(right)) {}
    
    Token op;
    std::shared_ptr<Expression> right;
};

// Statement node base class
class Statement : public ASTNode {
public:
    ~Statement() override = default;
};

// Expression statement (standalone expression)
class ExpressionStmt final : public Statement {
public:
     explicit ExpressionStmt(std::shared_ptr<Expression> expression)
        : expression(std::move(expression)) {}
    
    std::shared_ptr<Expression> expression;
};

// Variable declaration (let x = 5)
class VarDeclStmt final : public Statement {
public:
    VarDeclStmt(Token name, std::shared_ptr<Expression> initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}
    
    Token name;
    std::shared_ptr<Expression> initializer;
};

// Block statement ({ ... })
class BlockStmt final : public Statement {
public:
    explicit BlockStmt(std::vector<std::shared_ptr<Statement>> statements)
        : statements(std::move(statements)) {}
    
    std::vector<std::shared_ptr<Statement>> statements;
};

// If statement
class IfStmt final : public Statement {
public:
    IfStmt(std::shared_ptr<Expression> condition,
           std::shared_ptr<Statement> thenBranch,
           std::shared_ptr<Statement> elseBranch)
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
    
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> thenBranch;
    std::shared_ptr<Statement> elseBranch;
};

// Program node (root of AST)
class Program final : public ASTNode {
public:
    explicit Program(std::vector<std::shared_ptr<Statement>> statements)
        : statements(std::move(statements)) {}
    
    std::vector<std::shared_ptr<Statement>> statements;
};

#endif // AST_H
