#ifndef SCOPE_ANALYZER_H
#define SCOPE_ANALYZER_H

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
#include "ast_visitor.h"
#include "scope.h"

// ScopeAnalyzer is a visitor that analyzes variable declarations and usages,
// tracking scope chains and detecting closures
class ScopeAnalyzer : public ASTVisitor {
public:
    ScopeAnalyzer();
    ~ScopeAnalyzer() override = default;

    // Analyze the entire program
    void analyze(const Program &program);

    // AST Visitor interface implementation
    void visitProgram(const Program &program) override;

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

    // Get analysis results
    std::vector<std::string> getUnreferencedVariables() const;
    std::vector<std::string> getUninitializedVariables() const;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> getClosureInfo() const;

private:
    // Current function being analyzed for closure detection
    struct FunctionContext {
        std::string name;
        std::unordered_map<std::string, int> capturedVars;
    };

    // Scope management
    ScopeManager scopeManager;

    // Track function contexts for nested functions and closures
    std::stack<FunctionContext> functionStack;

    // Map of function name to captured variables
    std::unordered_map<std::string, std::unordered_map<std::string, int>> closureInfo;

    // Helper methods
    void enterFunction(const std::string &name);
    void exitFunction();
    void processExpression(const Expression &expr);

    // Error reporting
    std::vector<std::string> unreferencedVars;
    std::vector<std::string> uninitializedVars;
};

#endif // SCOPE_ANALYZER_H
