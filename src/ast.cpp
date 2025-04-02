#include "../include/ast.h"
#include "../include/ast_visitor.h"

// Expression accept methods
void LiteralExpr::accept(ExprVisitor& visitor) const {
    visitor.visitLiteralExpr(*this);
}

void VariableExpr::accept(ExprVisitor& visitor) const {
    visitor.visitVariableExpr(*this);
}

void BinaryExpr::accept(ExprVisitor& visitor) const {
    visitor.visitBinaryExpr(*this);
}

void UnaryExpr::accept(ExprVisitor& visitor) const {
    visitor.visitUnaryExpr(*this);
}

// Statement accept methods
void ExpressionStmt::accept(StmtVisitor& visitor) const {
    visitor.visitExpressionStmt(*this);
}

void VarDeclStmt::accept(StmtVisitor& visitor) const {
    visitor.visitVarDeclStmt(*this);
}

void BlockStmt::accept(StmtVisitor& visitor) const {
    visitor.visitBlockStmt(*this);
}

void IfStmt::accept(StmtVisitor& visitor) const {
    visitor.visitIfStmt(*this);
}

// Program accept method
void Program::accept(ASTVisitor& visitor) const {
    visitor.visitProgram(*this);
}
