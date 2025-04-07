#ifndef AST_VISITOR_H
#define AST_VISITOR_H

// Forward declarations of all AST node types
class Expression;
class LiteralExpr;
class VariableExpr;
class BinaryExpr;
class UnaryExpr;
class CallExpr;
class GetExpr;
class ArrayExpr;
class ObjectExpr;
class ArrowFunctionExpr;

class Statement;
class ExpressionStmt;
class VarDeclStmt;
class BlockStmt;
class IfStmt;
class WhileStmt;
class ForStmt;
class FunctionDeclStmt;
class ReturnStmt;
class BreakStmt;
class ContinueStmt;
class ClassDeclStmt;

class Program;

// Visitor for expression nodes
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;

    virtual void visitLiteralExpr(const LiteralExpr &expr) = 0;
    virtual void visitVariableExpr(const VariableExpr &expr) = 0;
    virtual void visitBinaryExpr(const BinaryExpr &expr) = 0;
    virtual void visitUnaryExpr(const UnaryExpr &expr) = 0;
    virtual void visitCallExpr(const CallExpr &expr) = 0;
    virtual void visitGetExpr(const GetExpr &expr) = 0;
    virtual void visitArrayExpr(const ArrayExpr &expr) = 0;
    virtual void visitObjectExpr(const ObjectExpr &expr) = 0;
    virtual void visitArrowFunctionExpr(const ArrowFunctionExpr &expr) = 0;
};

// Visitor for statement nodes
class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;

    virtual void visitExpressionStmt(const ExpressionStmt &stmt) = 0;
    virtual void visitVarDeclStmt(const VarDeclStmt &stmt) = 0;
    virtual void visitBlockStmt(const BlockStmt &stmt) = 0;
    virtual void visitIfStmt(const IfStmt &stmt) = 0;
    virtual void visitWhileStmt(const WhileStmt &stmt) = 0;
    virtual void visitForStmt(const ForStmt &stmt) = 0;
    virtual void visitFunctionDeclStmt(const FunctionDeclStmt &stmt) = 0;
    virtual void visitReturnStmt(const ReturnStmt &stmt) = 0;
    virtual void visitBreakStmt(const BreakStmt &stmt) = 0;
    virtual void visitContinueStmt(const ContinueStmt &stmt) = 0;
    virtual void visitClassDeclStmt(const ClassDeclStmt &stmt) = 0;
};

// Combined visitor for the entire AST
class ASTVisitor : public ExprVisitor, public StmtVisitor {
public:
    ~ASTVisitor() override = default;

    virtual void visitProgram(const Program &program) = 0;
};

#endif // AST_VISITOR_H
