#ifndef CFG_BUILDER_H
#define CFG_BUILDER_H

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include "../ast_visitor.h"
#include "control_flow_graph.h"

namespace cfg {

    // Class for building control flow graphs from AST nodes
    class CFGBuilder : public ASTVisitor {
    public:
        CFGBuilder() = default;
        ~CFGBuilder() override = default;

        // Build a CFG for a function declaration
        std::unique_ptr<ControlFlowGraph> buildCFG(const FunctionDeclStmt &func);

        // Build a CFG for a program (top-level function)
        std::unique_ptr<ControlFlowGraph> buildCFG(const Program &program);

        // ASTVisitor methods - implemented to traverse the AST and build the CFG
        void visitProgram(const Program &program) override;
        void visitBlockStmt(const BlockStmt &stmt) override;
        void visitExpressionStmt(const ExpressionStmt &stmt) override;
        void visitVarDeclStmt(const VarDeclStmt &stmt) override;
        void visitIfStmt(const IfStmt &stmt) override;
        void visitWhileStmt(const WhileStmt &stmt) override;
        void visitForStmt(const ForStmt &stmt) override;
        void visitFunctionDeclStmt(const FunctionDeclStmt &stmt) override;
        void visitReturnStmt(const ReturnStmt &stmt) override;
        void visitBreakStmt(const BreakStmt &stmt) override;
        void visitContinueStmt(const ContinueStmt &stmt) override;
        void visitClassDeclStmt(const ClassDeclStmt &stmt) override;

        void visitLiteralExpr(const LiteralExpr &expr) override;
        void visitVariableExpr(const VariableExpr &expr) override;
        void visitBinaryExpr(const BinaryExpr &expr) override;
        void visitUnaryExpr(const UnaryExpr &expr) override;
        void visitCallExpr(const CallExpr &expr) override;
        void visitGetExpr(const GetExpr &expr) override;
        void visitArrayExpr(const ArrayExpr &expr) override;
        void visitObjectExpr(const ObjectExpr &expr) override;
        void visitArrowFunctionExpr(const ArrowFunctionExpr &expr) override;

    private:
        std::unique_ptr<ControlFlowGraph> current_cfg_;
        BasicBlock *current_block_ = nullptr;

        // For break/continue statements
        struct LoopContext {
            BasicBlock *continue_target = nullptr;
            BasicBlock *break_target = nullptr;
        };
        std::stack<LoopContext> loop_stack_;

        // Counter for temporary variables
        int temp_var_counter_ = 0;

        // Counter for basic block names
        int block_counter_ = 0;

        // Generate a unique temporary variable name
        std::string genTempVar() { return "tmp_" + std::to_string(temp_var_counter_++); }

        // Generate a unique basic block name
        std::string genBlockName(const std::string &prefix = "block") {
            return prefix + "_" + std::to_string(block_counter_++);
        }

        // Generate code for an expression and return the variable holding its value
        std::string processExpression(const Expression &expr);

        // Start a new basic block and set it as current
        BasicBlock *startNewBlock(const std::string &name_prefix = "block");

        // Ensure the current block ends with a terminator instruction
        void ensureBlockTerminator();

        // Create a jump from current block to target block
        void createJump(BasicBlock *target);
    };

} // namespace cfg

#endif // CFG_BUILDER_H
