#ifndef AST_VISITOR_H
#define AST_VISITOR_H

#include "ast.h"

// Forward declarations
class LiteralExpr;
class VariableExpr;
class BinaryExpr;
class UnaryExpr;
class ExpressionStmt;
class VarDeclStmt;
class BlockStmt;
class IfStmt;
class Program;

// Visitor interface for expression nodes
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;
    
    virtual void visitLiteralExpr(const LiteralExpr& expr) = 0;
    virtual void visitVariableExpr(const VariableExpr& expr) = 0;
    virtual void visitBinaryExpr(const BinaryExpr& expr) = 0;
    virtual void visitUnaryExpr(const UnaryExpr& expr) = 0;
};

// Visitor interface for statement nodes
class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;
    
    virtual void visitExpressionStmt(const ExpressionStmt& stmt) = 0;
    virtual void visitVarDeclStmt(const VarDeclStmt& stmt) = 0;
    virtual void visitBlockStmt(const BlockStmt& stmt) = 0;
    virtual void visitIfStmt(const IfStmt& stmt) = 0;
};

// Combined visitor interface for all nodes
class ASTVisitor : public ExprVisitor, public StmtVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    virtual void visitProgram(const Program& program) = 0;
};

#endif // AST_VISITOR_H
