#ifndef AST_H
#define AST_H

#include <utility>
#include <vector>
#include <memory>
#include "token.h"

// Forward declarations
class ExprVisitor;
class StmtVisitor;
class ASTVisitor;

// Base AST node class
class ASTNode {
public:
    virtual ~ASTNode() = default;
};

// Expression node base class
class Expression : public ASTNode {
public:
    ~Expression() override = default;
    virtual void accept(ExprVisitor& visitor) const = 0;
};

// Literal expression (numbers, strings, booleans, null)
class LiteralExpr final : public Expression {
public:
    explicit LiteralExpr(Token token) : token(std::move(token)) {}
    void accept(ExprVisitor& visitor) const override;
    Token token;
};

// Variable reference
class VariableExpr final : public Expression {
public:
    explicit VariableExpr(Token name) : name(std::move(name)) {}
    void accept(ExprVisitor& visitor) const override;
    Token name;
};

// Binary operation (a + b, a * b, etc.)
class BinaryExpr final : public Expression {
public:
    BinaryExpr(std::shared_ptr<Expression> left, Token op, std::shared_ptr<Expression> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
    
    void accept(ExprVisitor& visitor) const override;
    std::shared_ptr<Expression> left;
    Token op;
    std::shared_ptr<Expression> right;
};

// Unary operation (-a, !b, etc.)
class UnaryExpr final : public Expression {
public:
    UnaryExpr(Token op, std::shared_ptr<Expression> right)
        : op(std::move(op)), right(std::move(right)) {}
    
    void accept(ExprVisitor& visitor) const override;
    Token op;
    std::shared_ptr<Expression> right;
};

// Statement node base class
class Statement : public ASTNode {
public:
    ~Statement() override = default;
    virtual void accept(StmtVisitor& visitor) const = 0;
};

// Expression statement (standalone expression)
class ExpressionStmt final : public Statement {
public:
     explicit ExpressionStmt(std::shared_ptr<Expression> expression)
        : expression(std::move(expression)) {}
    
    void accept(StmtVisitor& visitor) const override;
    std::shared_ptr<Expression> expression;
};

// Variable declaration (let x = 5)
class VarDeclStmt final : public Statement {
public:
    VarDeclStmt(Token name, std::shared_ptr<Expression> initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}
    
    void accept(StmtVisitor& visitor) const override;
    Token name;
    std::shared_ptr<Expression> initializer;
};

// Block statement ({ ... })
class BlockStmt final : public Statement {
public:
    explicit BlockStmt(std::vector<std::shared_ptr<Statement>> statements)
        : statements(std::move(statements)) {}
    
    void accept(StmtVisitor& visitor) const override;
    std::vector<std::shared_ptr<Statement>> statements;
};

// If statement
class IfStmt final : public Statement {
public:
    IfStmt(std::shared_ptr<Expression> condition,
           std::shared_ptr<Statement> thenBranch,
           std::shared_ptr<Statement> elseBranch)
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
    
    void accept(StmtVisitor& visitor) const override;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> thenBranch;
    std::shared_ptr<Statement> elseBranch;
};

// Program node (root of AST)
class Program final : public ASTNode {
public:
    explicit Program(std::vector<std::shared_ptr<Statement>> statements)
        : statements(std::move(statements)) {}
    
    void accept(ASTVisitor& visitor) const;
    std::vector<std::shared_ptr<Statement>> statements;
};

#endif // AST_H
