#include "../../include/cfg/cfg_builder.h"
#include <cassert>
#include <sstream>

namespace cfg {

    // Build a CFG from a function declaration
    std::unique_ptr<ControlFlowGraph> CFGBuilder::buildCFG(const FunctionDeclStmt &func) {
        // Reset state
        current_cfg_ = std::make_unique<ControlFlowGraph>();
        current_block_ = nullptr;
        temp_var_counter_ = 0;
        block_counter_ = 0;

        // Create an entry block
        current_block_ = startNewBlock("entry");

        // Add parameter assignments to entry block
        for (const auto &param: func.params) {
            auto assign = std::make_unique<AssignInstruction>(param.lexeme, "param_" + param.lexeme);
            current_block_->addInstruction(std::move(assign));
        }

        // Process the function body
        for (const auto &stmt: func.body) {
            stmt->accept(*this);
        }

        // Ensure the function has a return at the end
        if (!current_block_->hasTerminator()) {
            current_block_->addTerminator(ReturnInstruction::createImplicit());
        }

        return std::move(current_cfg_);
    }

    // Build a CFG from a program (top-level function)
    std::unique_ptr<ControlFlowGraph> CFGBuilder::buildCFG(const Program &program) {
        // Reset state
        current_cfg_ = std::make_unique<ControlFlowGraph>();
        current_block_ = nullptr;
        temp_var_counter_ = 0;
        block_counter_ = 0;

        // Create an entry block
        current_block_ = startNewBlock("entry");

        // Process each statement
        visit(program);

        // Ensure the program has a return at the end
        if (!current_block_->hasTerminator()) {
            current_block_->addTerminator(ReturnInstruction::createImplicit());
        }

        return std::move(current_cfg_);
    }

    // Visit methods for all AST node types

    void CFGBuilder::visit(const Program &program) {
        for (const auto &stmt: program.statements) {
            stmt->accept(*this);
        }
    }

    void CFGBuilder::visit(const BlockStmt &stmt) {
        for (const auto &s: stmt.statements) {
            s->accept(*this);
        }
    }

    void CFGBuilder::visit(const ExpressionStmt &stmt) {
        // Evaluate the expression for side effects
        processExpression(*stmt.expression);
    }

    void CFGBuilder::visit(const VarDeclStmt &stmt) {
        if (stmt.initializer) {
            // Initialize the variable
            std::string init_var = processExpression(*stmt.initializer);
            current_block_->addInstruction(std::make_unique<AssignInstruction>(stmt.name.lexeme, init_var));
        } else {
            // Default initialization
            current_block_->addInstruction(std::make_unique<AssignInstruction>(stmt.name.lexeme, "undefined"));
        }
    }

    void CFGBuilder::visit(const IfStmt &stmt) {
        // Create blocks for then branch, else branch, and merge
        BasicBlock *then_block = startNewBlock("then");
        BasicBlock *else_block = startNewBlock("else");
        BasicBlock *merge_block = startNewBlock("if_merge");

        // Process condition
        std::string condition = processExpression(*stmt.condition);

        // Add branch instruction to current block
        current_block_->addTerminator(std::make_unique<BranchInstruction>(condition, then_block, else_block));

        // Process 'then' branch
        current_block_ = then_block;
        stmt.thenBranch->accept(*this);
        // Jump to merge block if not already terminated
        if (!current_block_->hasTerminator()) {
            createJump(merge_block);
        }

        // Process 'else' branch if it exists
        current_block_ = else_block;
        if (stmt.elseBranch) {
            stmt.elseBranch->accept(*this);
        }
        // Jump to merge block if not already terminated
        if (!current_block_->hasTerminator()) {
            createJump(merge_block);
        }

        // Continue from merge block
        current_block_ = merge_block;
    }

    void CFGBuilder::visit(const WhileStmt &stmt) {
        // Create blocks for condition, body, and exit
        BasicBlock *cond_block = startNewBlock("while_cond");
        BasicBlock *body_block = startNewBlock("while_body");
        BasicBlock *exit_block = startNewBlock("while_exit");

        // Jump to condition block
        createJump(cond_block);

        // Process condition
        current_block_ = cond_block;
        std::string condition = processExpression(*stmt.condition);

        // Add branch instruction to condition block
        current_block_->addTerminator(std::make_unique<BranchInstruction>(condition, body_block, exit_block));

        // Push loop context for break/continue
        loop_stack_.push({cond_block, exit_block});

        // Process loop body
        current_block_ = body_block;
        stmt.body->accept(*this);

        // Jump back to condition if not already terminated
        if (!current_block_->hasTerminator()) {
            createJump(cond_block);
        }

        // Pop loop context
        loop_stack_.pop();

        // Continue from exit block
        current_block_ = exit_block;
    }

    void CFGBuilder::visit(const ForStmt &stmt) {
        // Create blocks for initialization, condition, increment, body, and exit
        BasicBlock *init_block = current_block_; // Use current block for initialization
        BasicBlock *cond_block = startNewBlock("for_cond");
        BasicBlock *body_block = startNewBlock("for_body");
        BasicBlock *incr_block = startNewBlock("for_incr");
        BasicBlock *exit_block = startNewBlock("for_exit");

        // Process initializer if it exists
        if (stmt.initializer) {
            stmt.initializer->accept(*this);
        }

        // Jump to condition block
        createJump(cond_block);

        // Process condition if it exists
        current_block_ = cond_block;
        if (stmt.condition) {
            std::string condition = processExpression(*stmt.condition);
            current_block_->addTerminator(std::make_unique<BranchInstruction>(condition, body_block, exit_block));
        } else {
            // No condition means always true
            createJump(body_block);
        }

        // Push loop context for break/continue
        loop_stack_.push({incr_block, exit_block});

        // Process loop body
        current_block_ = body_block;
        if (stmt.body) {
            stmt.body->accept(*this);
        }

        // Jump to increment block if not already terminated
        if (!current_block_->hasTerminator()) {
            createJump(incr_block);
        }

        // Process increment if it exists
        current_block_ = incr_block;
        if (stmt.increment) {
            processExpression(*stmt.increment);
        }

        // Jump back to condition
        createJump(cond_block);

        // Pop loop context
        loop_stack_.pop();

        // Continue from exit block
        current_block_ = exit_block;
    }

    void CFGBuilder::visit(const FunctionDeclStmt &stmt) {
        // Function declarations are not directly converted to CFG instructions
        // They're handled at a higher level by buildCFG()

        // But we do add a placeholder instruction to represent the declaration
        std::string func_name = stmt.name.lexeme;
        auto instr = std::make_unique<AssignInstruction>(func_name, "function_object");
        current_block_->addInstruction(std::move(instr));
    }

    void CFGBuilder::visit(const ReturnStmt &stmt) {
        if (stmt.value) {
            std::string return_val = processExpression(*stmt.value);
            current_block_->addTerminator(std::make_unique<ReturnInstruction>(return_val));
        } else {
            current_block_->addTerminator(std::make_unique<ReturnInstruction>());
        }
    }

    void CFGBuilder::visit(const BreakStmt &stmt) {
        if (loop_stack_.empty()) {
            // Error: break outside of loop
            return;
        }

        // Jump to the loop exit block
        createJump(loop_stack_.top().break_target);

        // Create a new unreachable block for any code after the break
        current_block_ = startNewBlock("after_break");
    }

    void CFGBuilder::visit(const ContinueStmt &stmt) {
        if (loop_stack_.empty()) {
            // Error: continue outside of loop
            return;
        }

        // Jump to the loop continue target (usually the increment or condition block)
        createJump(loop_stack_.top().continue_target);

        // Create a new unreachable block for any code after the continue
        current_block_ = startNewBlock("after_continue");
    }

    void CFGBuilder::visit(const ClassDeclStmt &stmt) {
        // Class declarations are not directly converted to CFG instructions
        // For now, just add a placeholder instruction
        std::string class_name = stmt.name.lexeme;
        auto instr = std::make_unique<AssignInstruction>(class_name, "class_object");
        current_block_->addInstruction(std::move(instr));
    }

    void CFGBuilder::visit(const LiteralExpr &expr) {
        // Generate temp var for the literal value
        std::string temp = genTempVar();

        // Create appropriate assignment based on literal type
        std::string value;
        switch (expr.token.type) {
            case TokenType::NUMBER:
                value = expr.token.lexeme;
                break;
            case TokenType::STRING:
                value = expr.token.lexeme; // Includes quotes
                break;
            case TokenType::TRUE:
                value = "true";
                break;
            case TokenType::FALSE:
                value = "false";
                break;
            case TokenType::NULL_KEYWORD:
                value = "null";
                break;
            default:
                value = "undefined";
                break;
        }

        current_block_->addInstruction(std::make_unique<AssignInstruction>(temp, value));
        processExpression_ = temp;
    }

    void CFGBuilder::visit(const VariableExpr &expr) {
        // Just return the variable name for use in other expressions
        processExpression_ = expr.name.lexeme;
    }

    void CFGBuilder::visit(const BinaryExpr &expr) {
        // Process left and right operands
        std::string left = processExpression(*expr.left);
        std::string right = processExpression(*expr.right);

        // Generate temp var for the result
        std::string temp = genTempVar();

        // Create binary operation instruction
        current_block_->addInstruction(std::make_unique<BinaryInstruction>(temp, left, expr.op.type, right));

        processExpression_ = temp;
    }

    void CFGBuilder::visit(const UnaryExpr &expr) {
        // Process the operand
        std::string operand = processExpression(*expr.right);

        // Generate temp var for the result
        std::string temp = genTempVar();

        // Create unary operation instruction
        current_block_->addInstruction(std::make_unique<UnaryInstruction>(temp, expr.op.type, operand));

        processExpression_ = temp;
    }

    void CFGBuilder::visit(const CallExpr &expr) {
        // Process the callee
        std::string callee = processExpression(*expr.callee);

        // Process all arguments
        std::vector<std::string> args;
        for (const auto &arg: expr.arguments) {
            args.push_back(processExpression(*arg));
        }

        // Generate temp var for the result
        std::string temp = genTempVar();

        // Create call instruction
        current_block_->addInstruction(std::make_unique<CallInstruction>(temp, callee, args));

        processExpression_ = temp;
    }

    void CFGBuilder::visit(const GetExpr &expr) {
        // Process the object
        std::string object = processExpression(*expr.object);

        // Generate temp var for the property access
        std::string temp = genTempVar();

        // Create property access as a binary operation for now
        // In a real implementation, we'd add a GetPropertyInstruction
        auto instr = std::make_unique<BinaryInstruction>(temp, object, TokenType::DOT, "\"" + expr.name.lexeme + "\"");
        current_block_->addInstruction(std::move(instr));

        processExpression_ = temp;
    }

    void CFGBuilder::visit(const ArrayExpr &expr) {
        // Generate temp var for the array
        std::string temp = genTempVar();

        // Create array initialization instruction (simplified)
        current_block_->addInstruction(std::make_unique<AssignInstruction>(temp, "[]"));

        // Add each element to the array
        for (size_t i = 0; i < expr.elements.size(); ++i) {
            std::string element = processExpression(*expr.elements[i]);

            // Create an instruction to add the element to the array
            // In a real implementation, we'd add an ArraySetInstruction
            auto instr = std::make_unique<BinaryInstruction>(temp, temp, TokenType::LEFT_BRACKET,
                                                             std::to_string(i) + "," + element);
            current_block_->addInstruction(std::move(instr));
        }

        processExpression_ = temp;
    }

    void CFGBuilder::visit(const ObjectExpr &expr) {
        // Generate temp var for the object
        std::string temp = genTempVar();

        // Create object initialization instruction (simplified)
        current_block_->addInstruction(std::make_unique<AssignInstruction>(temp, "{}"));

        // Add each property to the object
        for (const auto &prop: expr.properties) {
            std::string value = processExpression(*prop.value);

            // Create an instruction to add the property to the object
            // In a real implementation, we'd add an ObjectSetPropertyInstruction
            auto instr = std::make_unique<BinaryInstruction>(temp, temp, TokenType::DOT, prop.key.lexeme + "," + value);
            current_block_->addInstruction(std::move(instr));
        }

        processExpression_ = temp;
    }

    void CFGBuilder::visit(const ArrowFunctionExpr &expr) {
        // Generate temp var for the function
        std::string temp = genTempVar();

        // Create function object (simplified)
        std::stringstream params;
        for (size_t i = 0; i < expr.parameters.size(); ++i) {
            if (i > 0)
                params << ",";
            params << expr.parameters[i].lexeme;
        }

        std::string function_repr = "arrow_function(" + params.str() + ")";
        current_block_->addInstruction(std::make_unique<AssignInstruction>(temp, function_repr));

        processExpression_ = temp;

        // Note: Building a CFG for the function body would happen separately
    }

    // Helper methods

    std::string CFGBuilder::processExpression(const Expression &expr) {
        expr.accept(*this);
        return processExpression_;
    }

    BasicBlock *CFGBuilder::startNewBlock(const std::string &name_prefix) {
        ensureBlockTerminator();
        return current_cfg_->createBlock(genBlockName(name_prefix));
    }

    void CFGBuilder::ensureBlockTerminator() {
        if (current_block_ && !current_block_->hasTerminator()) {
            // No terminator instruction yet, add a placeholder jump instruction
            // The target will be set properly when we know where to jump to
            current_block_->addTerminator(std::make_unique<JumpInstruction>(nullptr));
        }
    }

    void CFGBuilder::createJump(BasicBlock *target) {
        if (!current_block_->hasTerminator()) {
            current_block_->addTerminator(std::make_unique<JumpInstruction>(target));
        }
    }

} // namespace cfg
