#ifndef DATAFLOW_ANALYSES_H
#define DATAFLOW_ANALYSES_H

#include <map>
#include <set>
#include <string>
#include <utility>
#include "ssa_transformer.h"

namespace cfg {

    // Expression representation for available expressions analysis
    struct Expression {
        enum class Type { Binary, Unary, Variable };

        Type type;
        TokenType op; // For binary/unary operations
        std::string left; // For binary operations or variable name
        std::string right; // For binary operations

        // Constructor for binary expressions
        Expression(TokenType op, const std::string &left, const std::string &right) :
            type(Type::Binary), op(op), left(left), right(right) {}

        // Constructor for unary expressions
        Expression(TokenType op, const std::string &operand) : type(Type::Unary), op(op), left(operand), right("") {}

        // Constructor for variables
        explicit Expression(const std::string &var) :
            type(Type::Variable), op(TokenType::IDENTIFIER), left(var), right("") {}

        // For use in sets and maps
        bool operator==(const Expression &other) const {
            if (type != other.type)
                return false;
            if (op != other.op)
                return false;

            if (type == Type::Binary) {
                return (left == other.left && right == other.right) ||
                       // For commutative operations, order doesn't matter
                       (isCommutative() && left == other.right && right == other.left);
            } else {
                return left == other.left;
            }
        }

        bool operator<(const Expression &other) const {
            if (type != other.type)
                return type < other.type;
            if (op != other.op)
                return op < other.op;
            if (left != other.left)
                return left < other.left;
            return right < other.right;
        }

        // Check if this is a commutative operation
        bool isCommutative() const {
            return type == Type::Binary && (op == TokenType::PLUS || // +
                                            op == TokenType::STAR || // *
                                            op == TokenType::EQUAL_EQUAL || // ==
                                            op == TokenType::BANG_EQUAL || // !=
                                            op == TokenType::AND || // &&
                                            op == TokenType::OR // ||
                                           );
        }

        // String representation for debugging
        std::string toString() const {
            if (type == Type::Binary) {
                std::string op_str;
                switch (op) {
                    case TokenType::PLUS:
                        op_str = "+";
                        break;
                    case TokenType::MINUS:
                        op_str = "-";
                        break;
                    case TokenType::STAR:
                        op_str = "*";
                        break;
                    case TokenType::SLASH:
                        op_str = "/";
                        break;
                    case TokenType::EQUAL_EQUAL:
                        op_str = "==";
                        break;
                    case TokenType::BANG_EQUAL:
                        op_str = "!=";
                        break;
                    case TokenType::LESS:
                        op_str = "<";
                        break;
                    case TokenType::LESS_EQUAL:
                        op_str = "<=";
                        break;
                    case TokenType::GREATER:
                        op_str = ">";
                        break;
                    case TokenType::GREATER_EQUAL:
                        op_str = ">=";
                        break;
                    case TokenType::AND:
                        op_str = "&&";
                        break;
                    case TokenType::OR:
                        op_str = "||";
                        break;
                    default:
                        op_str = "?";
                        break;
                }
                return "(" + left + " " + op_str + " " + right + ")";
            } else if (type == Type::Unary) {
                std::string op_str;
                switch (op) {
                    case TokenType::MINUS:
                        op_str = "-";
                        break;
                    case TokenType::BANG:
                        op_str = "!";
                        break;
                    default:
                        op_str = "?";
                        break;
                }
                return op_str + left;
            } else {
                return left;
            }
        }
    };

    // Hash function for Expression
    struct ExpressionHash {
        std::size_t operator()(const Expression &expr) const {
            std::size_t h1 = std::hash<int>{}(static_cast<int>(expr.type));
            std::size_t h2 = std::hash<int>{}(static_cast<int>(expr.op));
            std::size_t h3 = std::hash<std::string>{}(expr.left);
            std::size_t h4 = std::hash<std::string>{}(expr.right);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };

    // Available Expressions Analysis
    // Identifies expressions that have been computed and not later modified
    class AvailableExpressionsAnalysis : public DataFlowAnalysis<std::unordered_set<Expression, ExpressionHash>> {
    public:
        AvailableExpressionsAnalysis() {
            // Initialize all expressions that appear in the CFG
            all_expressions_ = std::unordered_set<Expression, ExpressionHash>();
        }

    protected:
        void initialize(ControlFlowGraph &cfg) override {
            // Find all expressions in the CFG
            all_expressions_.clear();

            // First, identify all expressions in the CFG
            for (auto *block: cfg.getBlocks()) {
                for (const auto &instr: block->instructions) {
                    if (instr->type == Instruction::Type::Binary) {
                        auto *binary = static_cast<const BinaryInstruction *>(instr.get());
                        all_expressions_.emplace(binary->op, binary->left, binary->right);
                    } else if (instr->type == Instruction::Type::Unary) {
                        auto *unary = static_cast<const UnaryInstruction *>(instr.get());
                        all_expressions_.emplace(unary->op, unary->operand);
                    }
                }
            }

            // Initialize dataflow values:
            // Entry block has empty set, other blocks have all expressions
            for (auto *block: cfg.getBlocks()) {
                if (block == cfg.getEntryBlock()) {
                    block_outputs_[block] = std::unordered_set<Expression, ExpressionHash>();
                } else {
                    block_outputs_[block] = all_expressions_;
                }
            }
        }

        std::unordered_set<Expression, ExpressionHash> computeInput(BasicBlock *block) override {
            // Available expressions are those available on all predecessor paths

            // For entry block, no available expressions
            if (block->predecessors.empty()) {
                return std::unordered_set<Expression, ExpressionHash>();
            }

            // Start with all expressions
            std::unordered_set<Expression, ExpressionHash> result = all_expressions_;

            // Intersect available expressions from all predecessors
            for (auto *pred: block->predecessors) {
                const auto &pred_avail = getOutput(pred);

                // Keep only expressions available in this predecessor
                std::unordered_set<Expression, ExpressionHash> intersection;
                for (const auto &expr: result) {
                    if (pred_avail.find(expr) != pred_avail.end()) {
                        intersection.insert(expr);
                    }
                }

                result = std::move(intersection);
            }

            return result;
        }

        std::unordered_set<Expression, ExpressionHash>
        transferFunction(BasicBlock *block, const std::unordered_set<Expression, ExpressionHash> &in) override {

            // Start with available expressions from input
            auto result = in;

            // Process instructions
            for (const auto &instr: block->instructions) {
                // Add newly available expressions
                if (instr->type == Instruction::Type::Binary) {
                    auto *binary = static_cast<const BinaryInstruction *>(instr.get());
                    result.emplace(binary->op, binary->left, binary->right);
                } else if (instr->type == Instruction::Type::Unary) {
                    auto *unary = static_cast<const UnaryInstruction *>(instr.get());
                    result.emplace(unary->op, unary->operand);
                }

                // Remove expressions that are invalidated by this instruction
                // (i.e., expressions containing variables that are redefined)
                const auto &defined_vars = instr->getDefinedVars();
                if (!defined_vars.empty()) {
                    std::unordered_set<Expression, ExpressionHash> valid_exprs;

                    for (const auto &expr: result) {
                        bool is_valid = true;

                        // Check if expression uses any variable defined here
                        for (const auto &var: defined_vars) {
                            if (expr.type == Expression::Type::Binary) {
                                if (expr.left == var || expr.right == var) {
                                    is_valid = false;
                                    break;
                                }
                            } else if (expr.type == Expression::Type::Unary) {
                                if (expr.left == var) {
                                    is_valid = false;
                                    break;
                                }
                            } else if (expr.type == Expression::Type::Variable) {
                                if (expr.left == var) {
                                    is_valid = false;
                                    break;
                                }
                            }
                        }

                        if (is_valid) {
                            valid_exprs.insert(expr);
                        }
                    }

                    result = std::move(valid_exprs);
                }
            }

            return result;
        }

        // Get all expressions in this CFG
        const std::unordered_set<Expression, ExpressionHash> &getAllExpressions() const { return all_expressions_; }

        // Check if an expression is available at the beginning of a block
        bool isExpressionAvailable(const Expression &expr, BasicBlock *block) const {
            // Get input for this block
            std::unordered_set<Expression, ExpressionHash> avail = computeInput(block);
            return avail.find(expr) != avail.end();
        }

        // Get all available expressions at the beginning of a block
        std::unordered_set<Expression, ExpressionHash> computeInput(BasicBlock *block) const {
            return computeInput(block);
        }

    private:
        std::unordered_set<Expression, ExpressionHash> all_expressions_;
    };

    // Constant Propagation Analysis
    // Identifies variables that have constant values at each program point
    class ConstantPropagationAnalysis
        : public DataFlowAnalysis<
                  std::unordered_map<std::string, std::variant<std::monostate, double, std::string, bool>>> {
    public:
        // Constants can be undefined (top), a concrete value, or NAC (bottom)
        using ConstantValue = std::variant<std::monostate, double, std::string, bool>;

        // Special values
        static constexpr auto UNDEFINED = std::monostate{}; // Top value: could be anything (uninitialized)
        // NAC (Not A Constant) is represented by a specific string
        static ConstantValue NAC() { return std::string("NAC"); }

        // Check if a value is NAC
        static bool isNAC(const ConstantValue &value) {
            return std::holds_alternative<std::string>(value) && std::get<std::string>(value) == "NAC";
        }

        // Meet operation for constants
        static ConstantValue meet(const ConstantValue &a, const ConstantValue &b) {
            // If either is NAC, result is NAC
            if (isNAC(a) || isNAC(b)) {
                return NAC();
            }

            // If either is undefined, return the other
            if (std::holds_alternative<std::monostate>(a)) {
                return b;
            }

            if (std::holds_alternative<std::monostate>(b)) {
                return a;
            }

            // If they're the same value, return that
            if (a == b) {
                return a;
            }

            // Different constants, so result is NAC
            return NAC();
        }

        // String representation of constant value
        static std::string valueToString(const ConstantValue &value) {
            if (isNAC(value)) {
                return "NAC";
            }

            if (std::holds_alternative<std::monostate>(value)) {
                return "UNDEFINED";
            }

            if (std::holds_alternative<double>(value)) {
                return std::to_string(std::get<double>(value));
            }

            if (std::holds_alternative<std::string>(value)) {
                return "\"" + std::get<std::string>(value) + "\"";
            }

            if (std::holds_alternative<bool>(value)) {
                return std::get<bool>(value) ? "true" : "false";
            }

            return "UNKNOWN";
        }

    protected:
        void initialize(ControlFlowGraph &cfg) override {
            // Initialize all blocks with empty maps (no constants known initially)
            for (auto *block: cfg.getBlocks()) {
                block_outputs_[block] = std::unordered_map<std::string, ConstantValue>();
            }
        }

        std::unordered_map<std::string, ConstantValue> computeInput(BasicBlock *block) override {
            std::unordered_map<std::string, ConstantValue> result;

            // For entry block, no constants
            if (block->predecessors.empty()) {
                return result;
            }

            // First predecessor defines initial constants
            auto *first_pred = block->predecessors[0];
            result = getOutput(first_pred);

            // Meet with other predecessors
            for (size_t i = 1; i < block->predecessors.size(); i++) {
                auto *pred = block->predecessors[i];
                const auto &pred_constants = getOutput(pred);

                // Add all variables from this predecessor
                for (const auto &[var, value]: pred_constants) {
                    auto it = result.find(var);
                    if (it == result.end()) {
                        result[var] = value;
                    } else {
                        // Meet operation - conservative: different values become NAC
                        result[var] = meet(it->second, value);
                    }
                }
            }

            return result;
        }

        std::unordered_map<std::string, ConstantValue>
        transferFunction(BasicBlock *block, const std::unordered_map<std::string, ConstantValue> &in) override {
            // Start with input constants
            auto result = in;

            // Update based on instructions
            for (const auto &instr: block->instructions) {
                // Handle different instruction types
                if (instr->type == Instruction::Type::Assign) {
                    auto *assign = static_cast<const AssignInstruction *>(instr.get());

                    // Check if source is a constant literal
                    if (isNumericLiteral(assign->source)) {
                        result[assign->target] = std::stod(assign->source);
                    } else if (isStringLiteral(assign->source)) {
                        // Remove the quotes
                        std::string str = assign->source.substr(1, assign->source.length() - 2);
                        result[assign->target] = str;
                    } else if (assign->source == "true") {
                        result[assign->target] = true;
                    } else if (assign->source == "false") {
                        result[assign->target] = false;
                    } else if (assign->source == "null" || assign->source == "undefined") {
                        // Treat null/undefined as not a constant for optimization purposes
                        result[assign->target] = NAC();
                    } else {
                        // Check if source is a variable with known constant value
                        auto it = result.find(assign->source);
                        if (it != result.end()) {
                            result[assign->target] = it->second;
                        } else {
                            // Source is not a known constant
                            result[assign->target] = NAC();
                        }
                    }
                } else if (instr->type == Instruction::Type::Binary) {
                    auto *binary = static_cast<const BinaryInstruction *>(instr.get());

                    // Try to evaluate constant expression
                    auto left_val = getConstantValue(binary->left, result);
                    auto right_val = getConstantValue(binary->right, result);

                    // If both operands are constants, compute the result
                    if (!isNAC(left_val) && !isNAC(right_val) && !std::holds_alternative<std::monostate>(left_val) &&
                        !std::holds_alternative<std::monostate>(right_val)) {

                        // Only handle numeric operations for simplicity
                        if (std::holds_alternative<double>(left_val) && std::holds_alternative<double>(right_val)) {

                            double left = std::get<double>(left_val);
                            double right = std::get<double>(right_val);

                            switch (binary->op) {
                                case TokenType::PLUS:
                                    result[binary->result] = left + right;
                                    break;
                                case TokenType::MINUS:
                                    result[binary->result] = left - right;
                                    break;
                                case TokenType::STAR:
                                    result[binary->result] = left * right;
                                    break;
                                case TokenType::SLASH:
                                    // Avoid division by zero
                                    if (right == 0) {
                                        result[binary->result] = NAC();
                                    } else {
                                        result[binary->result] = left / right;
                                    }
                                    break;
                                case TokenType::EQUAL_EQUAL:
                                    result[binary->result] = (left == right);
                                    break;
                                case TokenType::BANG_EQUAL:
                                    result[binary->result] = (left != right);
                                    break;
                                case TokenType::LESS:
                                    result[binary->result] = (left < right);
                                    break;
                                case TokenType::LESS_EQUAL:
                                    result[binary->result] = (left <= right);
                                    break;
                                case TokenType::GREATER:
                                    result[binary->result] = (left > right);
                                    break;
                                case TokenType::GREATER_EQUAL:
                                    result[binary->result] = (left >= right);
                                    break;
                                default:
                                    result[binary->result] = NAC();
                            }
                        } else {
                            // Non-numeric operation
                            result[binary->result] = NAC();
                        }
                    } else {
                        // Non-constant operands
                        result[binary->result] = NAC();
                    }
                } else if (instr->type == Instruction::Type::Unary) {
                    auto *unary = static_cast<const UnaryInstruction *>(instr.get());

                    // Try to evaluate constant expression
                    auto operand_val = getConstantValue(unary->operand, result);

                    // If operand is constant, compute the result
                    if (!isNAC(operand_val) && !std::holds_alternative<std::monostate>(operand_val)) {

                        // Handle numeric operations
                        if (std::holds_alternative<double>(operand_val)) {
                            double val = std::get<double>(operand_val);

                            switch (unary->op) {
                                case TokenType::MINUS:
                                    result[unary->result] = -val;
                                    break;
                                default:
                                    result[unary->result] = NAC();
                            }
                        }
                        // Handle boolean operations
                        else if (std::holds_alternative<bool>(operand_val)) {
                            bool val = std::get<bool>(operand_val);

                            switch (unary->op) {
                                case TokenType::BANG:
                                    result[unary->result] = !val;
                                    break;
                                default:
                                    result[unary->result] = NAC();
                            }
                        } else {
                            // Non-numeric non-boolean operation
                            result[unary->result] = NAC();
                        }
                    } else {
                        // Non-constant operand
                        result[unary->result] = NAC();
                    }
                } else if (instr->type == Instruction::Type::Phi) {
                    auto *phi = static_cast<const PhiInstruction *>(instr.get());

                    // Compute meet of all incoming values
                    ConstantValue phi_val = UNDEFINED;

                    for (const auto &source_var: phi->source_vars) {
                        ConstantValue incoming_val;

                        // Check if source is a constant
                        if (isNumericLiteral(source_var)) {
                            incoming_val = std::stod(source_var);
                        } else if (isStringLiteral(source_var)) {
                            incoming_val = source_var.substr(1, source_var.length() - 2); // Remove quotes
                        } else if (source_var == "true") {
                            incoming_val = true;
                        } else if (source_var == "false") {
                            incoming_val = false;
                        } else {
                            // Source is a variable, check if it's a constant
                            auto it = result.find(source_var);
                            incoming_val = (it != result.end()) ? it->second : NAC();
                        }

                        // Meet with current result
                        phi_val = (std::holds_alternative<std::monostate>(phi_val)) ? incoming_val
                                                                                    : meet(phi_val, incoming_val);
                    }

                    result[phi->target_var] = phi_val;
                } else if (instr->type == Instruction::Type::Call) {
                    // Function calls make the result non-constant
                    auto *call = static_cast<const CallInstruction *>(instr.get());
                    result[call->result] = NAC();
                } else {
                    // Other instruction types - handle defined variables
                    for (const auto &var: instr->getDefinedVars()) {
                        if (!var.empty()) {
                            result[var] = NAC();
                        }
                    }
                }
            }

            return result;
        }

    private:
        // Helper to check if a string is a numeric literal
        bool isNumericLiteral(const std::string &str) const {
            return !str.empty() && str.find_first_not_of("0123456789.e+-") == std::string::npos;
        }

        // Helper to check if a string is a string literal
        bool isStringLiteral(const std::string &str) const {
            return str.size() >= 2 &&
                   ((str.front() == '"' && str.back() == '"') || (str.front() == '\'' && str.back() == '\''));
        }

        // Get constant value for a variable or literal
        ConstantValue getConstantValue(const std::string &expr,
                                       const std::unordered_map<std::string, ConstantValue> &constants) const {

            if (isNumericLiteral(expr)) {
                return std::stod(expr);
            } else if (isStringLiteral(expr)) {
                return expr.substr(1, expr.length() - 2); // Remove quotes
            } else if (expr == "true") {
                return true;
            } else if (expr == "false") {
                return false;
            } else {
                // Variable lookup
                auto it = constants.find(expr);
                return (it != constants.end()) ? it->second : NAC();
            }
        }
    };

    // Dead Code Elimination Analysis
    // Identifies code that has no effect on the program's output
    class DeadCodeAnalysis : public DataFlowAnalysis<std::unordered_set<std::string>> {
    public:
        // Constructor
        DeadCodeAnalysis() {
            // Start with empty sets
        }

        // Get unused definitions in a basic block
        std::unordered_set<const Instruction *> getUnusedDefinitions(BasicBlock *block) {
            std::unordered_set<const Instruction *> result;

            // Get live variables at the end of this block
            const auto &live_out = getOutput(block);

            // Process instructions in reverse order
            std::unordered_set<std::string> live = live_out;

            for (auto it = block->instructions.rbegin(); it != block->instructions.rend(); ++it) {
                const Instruction *instr = it->get();

                // Check if any defined variable is live
                bool has_live_def = false;
                for (const auto &var: instr->getDefinedVars()) {
                    if (live.find(var) != live.end()) {
                        has_live_def = true;
                        break;
                    }
                }

                // If instruction has no live definitions, mark as unused
                if (!has_live_def && !hasSideEffects(instr)) {
                    result.insert(instr);
                }

                // Update live variables
                // First add used variables
                for (const auto &var: instr->getUsedVars()) {
                    live.insert(var);
                }

                // Then remove defined variables
                for (const auto &var: instr->getDefinedVars()) {
                    live.erase(var);
                }
            }

            return result;
        }

    protected:
        void initialize(ControlFlowGraph &cfg) override {
            // Initialize with empty sets of live variables
            for (auto *block: cfg.getBlocks()) {
                block_outputs_[block] = std::unordered_set<std::string>();
            }
        }

        std::unordered_set<std::string> computeInput(BasicBlock *block) override {
            // Compute live variables at the beginning of the block
            // based on live variables at the end of the block and the block's instructions
            const auto &live_out = block_outputs_[block];

            std::unordered_set<std::string> live = live_out;

            // Process instructions in reverse order
            for (auto it = block->instructions.rbegin(); it != block->instructions.rend(); ++it) {
                const Instruction *instr = it->get();

                // First add used variables
                for (const auto &var: instr->getUsedVars()) {
                    live.insert(var);
                }

                // Then remove defined variables
                for (const auto &var: instr->getDefinedVars()) {
                    live.erase(var);
                }
            }

            return live;
        }

        std::unordered_set<std::string> transferFunction(BasicBlock *block,
                                                         const std::unordered_set<std::string> &in) override {

            // For backward analysis, input is computed from output using computeInput
            // This is used by the framework for the iterative calculation
            return in;
        }

        // Check if an instruction has side effects (like I/O, calls)
        bool hasSideEffects(const Instruction *instr) const {
            // Call instructions may have side effects
            if (instr->type == Instruction::Type::Call) {
                return true;
            }

            // Return, branch, and jump instructions affect control flow
            if (instr->type == Instruction::Type::Return || instr->type == Instruction::Type::Branch ||
                instr->type == Instruction::Type::Jump) {
                return true;
            }

            return false;
        }
    };

} // namespace cfg

#endif // DATAFLOW_ANALYSES_H
