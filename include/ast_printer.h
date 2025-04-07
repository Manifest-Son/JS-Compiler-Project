#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include <iostream>
#include <string>
#include "ast.h"
#include "ast_visitor.h"

// ASTPrinter class - prints the AST in a human-readable format
class ASTPrinter : public ASTVisitor {
private:
    int indent_level = 0;
    // Helper method to print the current indent level
    void printIndent() const {
        for (int i = 0; i < indent_level; i++) {
            std::cout << "  ";
        }
    }

public:
    ASTPrinter() = default;
    ~ASTPrinter() override = default;

    // Program visitor
    void visitProgram(const Program &program) override;

    // Statement visitors
    void visitExpressionStmt(const ExpressionStmt &stmt) override;
    void visitVarDeclStmt(const VarDeclStmt &stmt) override;
    void visitBlockStmt(const BlockStmt &stmt) override;
    void visitIfStmt(const IfStmt &stmt) override;
    void visitWhileStmt(const WhileStmt &stmt) override;
    void visitForStmt(const ForStmt &stmt) override;
    void visitFunctionDeclStmt(const FunctionDeclStmt &stmt) override;
    void visitReturnStmt(const ReturnStmt &stmt) override;
    void visitBreakStmt(const BreakStmt &stmt) override;
    void visitContinueStmt(const ContinueStmt &stmt) override;
    void visitClassDeclStmt(const ClassDeclStmt &stmt) override;

    // Expression visitors
    void visitLiteralExpr(const LiteralExpr &expr) override;
    void visitVariableExpr(const VariableExpr &expr) override;
    void visitBinaryExpr(const BinaryExpr &expr) override;
    void visitUnaryExpr(const UnaryExpr &expr) override;
    void visitCallExpr(const CallExpr &expr) override;
    void visitGetExpr(const GetExpr &expr) override;
    void visitArrayExpr(const ArrayExpr &expr) override;
    void visitObjectExpr(const ObjectExpr &expr) override;
    void visitArrowFunctionExpr(const ArrowFunctionExpr &expr) override;
    std::initializer_list<char> print(const std::shared_ptr<Program> & shared);
};

#endif // AST_PRINTER_H
