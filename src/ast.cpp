#include "../include/ast.h"
#include "../include/ast_visitor.h"

// Expression accept methods
void LiteralExpr::accept(ExprVisitor &visitor) const { visitor.visitLiteralExpr(*this); }

void VariableExpr::accept(ExprVisitor &visitor) const { visitor.visitVariableExpr(*this); }

void BinaryExpr::accept(ExprVisitor &visitor) const { visitor.visitBinaryExpr(*this); }

void UnaryExpr::accept(ExprVisitor &visitor) const { visitor.visitUnaryExpr(*this); }

void CallExpr::accept(ExprVisitor &visitor) const { visitor.visitCallExpr(*this); }

void GetExpr::accept(ExprVisitor &visitor) const { visitor.visitGetExpr(*this); }

void ArrayExpr::accept(ExprVisitor &visitor) const { visitor.visitArrayExpr(*this); }

void ObjectExpr::accept(ExprVisitor &visitor) const { visitor.visitObjectExpr(*this); }

void ArrowFunctionExpr::accept(ExprVisitor &visitor) const { visitor.visitArrowFunctionExpr(*this); }

// Statement accept methods
void ExpressionStmt::accept(StmtVisitor &visitor) const { visitor.visitExpressionStmt(*this); }

void VarDeclStmt::accept(StmtVisitor &visitor) const { visitor.visitVarDeclStmt(*this); }

void BlockStmt::accept(StmtVisitor &visitor) const { visitor.visitBlockStmt(*this); }

void IfStmt::accept(StmtVisitor &visitor) const { visitor.visitIfStmt(*this); }

void WhileStmt::accept(StmtVisitor &visitor) const { visitor.visitWhileStmt(*this); }

void ForStmt::accept(StmtVisitor &visitor) const { visitor.visitForStmt(*this); }

void FunctionDeclStmt::accept(StmtVisitor &visitor) const { visitor.visitFunctionDeclStmt(*this); }

void ReturnStmt::accept(StmtVisitor &visitor) const { visitor.visitReturnStmt(*this); }

void BreakStmt::accept(StmtVisitor &visitor) const { visitor.visitBreakStmt(*this); }

void ContinueStmt::accept(StmtVisitor &visitor) const { visitor.visitContinueStmt(*this); }

void ClassDeclStmt::accept(StmtVisitor &visitor) const { visitor.visitClassDeclStmt(*this); }

// Program accept method
void Program::accept(ASTVisitor &visitor) const { visitor.visitProgram(*this); }
