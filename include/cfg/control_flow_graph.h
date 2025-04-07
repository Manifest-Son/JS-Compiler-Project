#ifndef CONTROL_FLOW_GRAPH_H
#define CONTROL_FLOW_GRAPH_H

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../ast.h"

namespace cfg {

    // Forward declarations
    class BasicBlock;
    class Instruction;
    class PhiInstruction;
    class ControlFlowGraph;

    // Instruction base class
    class Instruction {
    public:
        enum class Type {
            Assign, // Variable assignment
            Binary, // Binary operation
            Unary, // Unary operation
            Call, // Function call
            Return, // Return statement
            Branch, // Conditional branch
            Jump, // Unconditional jump
            Phi // Phi node (SSA)
        };

        explicit Instruction(Type type) : type(type) {}
        virtual ~Instruction() = default;

        Type type;
        BasicBlock *parent = nullptr;
        SourcePosition position;

        // For SSA form
        std::string result_var;
        int version = 0; // Assigned during SSA transformation

        // Clone this instruction
        virtual std::unique_ptr<Instruction> clone() const = 0;

        // Get variables defined by this instruction
        virtual std::vector<std::string> getDefinedVars() const {
            return result_var.empty() ? std::vector<std::string>{} : std::vector<std::string>{result_var};
        }

        // Get variables used by this instruction
        virtual std::vector<std::string> getUsedVars() const = 0;

        // For debugging
        virtual std::string toString() const = 0;
    };

    // Phi instruction for SSA form
    class PhiInstruction : public Instruction {
    public:
        PhiInstruction(std::string target_var, std::vector<std::string> source_vars,
                       std::vector<BasicBlock *> source_blocks) :
            Instruction(Type::Phi), target_var(std::move(target_var)), source_vars(std::move(source_vars)),
            source_blocks(std::move(source_blocks)) {
            result_var = this->target_var;
        }

        std::string target_var;
        std::vector<std::string> source_vars;
        std::vector<BasicBlock *> source_blocks;

        std::unique_ptr<Instruction> clone() const override {
            return std::make_unique<PhiInstruction>(target_var, source_vars, source_blocks);
        }

        std::vector<std::string> getUsedVars() const override { return source_vars; }

        std::string toString() const override;
    };

    // Assignment instruction
    class AssignInstruction : public Instruction {
    public:
        AssignInstruction(std::string target, std::string source) :
            Instruction(Type::Assign), target(std::move(target)), source(std::move(source)) {
            result_var = this->target;
        }

        std::string target;
        std::string source;

        std::unique_ptr<Instruction> clone() const override {
            return std::make_unique<AssignInstruction>(target, source);
        }

        std::vector<std::string> getUsedVars() const override { return {source}; }

        std::string toString() const override;
    };

    // Binary operation instruction
    class BinaryInstruction : public Instruction {
    public:
        BinaryInstruction(std::string result, std::string left, TokenType op, std::string right) :
            Instruction(Type::Binary), result(std::move(result)), left(std::move(left)), op(op),
            right(std::move(right)) {
            result_var = this->result;
        }

        std::string result;
        std::string left;
        TokenType op;
        std::string right;

        std::unique_ptr<Instruction> clone() const override {
            return std::make_unique<BinaryInstruction>(result, left, op, right);
        }

        std::vector<std::string> getUsedVars() const override { return {left, right}; }

        std::string toString() const override;
    };

    // Unary operation instruction
    class UnaryInstruction : public Instruction {
    public:
        UnaryInstruction(std::string result, TokenType op, std::string operand) :
            Instruction(Type::Unary), result(std::move(result)), op(op), operand(std::move(operand)) {
            result_var = this->result;
        }

        std::string result;
        TokenType op;
        std::string operand;

        std::unique_ptr<Instruction> clone() const override {
            return std::make_unique<UnaryInstruction>(result, op, operand);
        }

        std::vector<std::string> getUsedVars() const override { return {operand}; }

        std::string toString() const override;
    };

    // Function call instruction
    class CallInstruction : public Instruction {
    public:
        CallInstruction(std::string result, std::string callee, std::vector<std::string> args) :
            Instruction(Type::Call), result(std::move(result)), callee(std::move(callee)), args(std::move(args)) {
            result_var = this->result;
        }

        std::string result;
        std::string callee;
        std::vector<std::string> args;

        std::unique_ptr<Instruction> clone() const override {
            return std::make_unique<CallInstruction>(result, callee, args);
        }

        std::vector<std::string> getUsedVars() const override {
            std::vector<std::string> used = {callee};
            used.insert(used.end(), args.begin(), args.end());
            return used;
        }

        std::string toString() const override;
    };

    // Return instruction
    class ReturnInstruction : public Instruction {
    public:
        explicit ReturnInstruction(std::string value = "") :
            Instruction(Type::Return), value(std::move(value)), is_implicit(false) {}

        static std::unique_ptr<ReturnInstruction> createImplicit() {
            auto ret = std::make_unique<ReturnInstruction>();
            ret->is_implicit = true;
            return ret;
        }

        std::string value;
        bool is_implicit;

        std::unique_ptr<Instruction> clone() const override {
            auto ret = std::make_unique<ReturnInstruction>(value);
            ret->is_implicit = is_implicit;
            return ret;
        }

        std::vector<std::string> getUsedVars() const override {
            return value.empty() ? std::vector<std::string>{} : std::vector<std::string>{value};
        }

        std::string toString() const override;
    };

    // Conditional branch instruction
    class BranchInstruction : public Instruction {
    public:
        BranchInstruction(std::string condition, BasicBlock *true_target, BasicBlock *false_target) :
            Instruction(Type::Branch), condition(std::move(condition)), true_target(true_target),
            false_target(false_target) {}

        std::string condition;
        BasicBlock *true_target;
        BasicBlock *false_target;

        std::unique_ptr<Instruction> clone() const override {
            return std::make_unique<BranchInstruction>(condition, true_target, false_target);
        }

        std::vector<std::string> getUsedVars() const override { return {condition}; }

        std::string toString() const override;
    };

    // Unconditional jump instruction
    class JumpInstruction : public Instruction {
    public:
        explicit JumpInstruction(BasicBlock *target) : Instruction(Type::Jump), target(target) {}

        BasicBlock *target;

        std::unique_ptr<Instruction> clone() const override { return std::make_unique<JumpInstruction>(target); }

        std::vector<std::string> getUsedVars() const override { return {}; }

        std::string toString() const override;
    };

    // Basic block in CFG
    class BasicBlock {
    public:
        explicit BasicBlock(std::string name) : name(std::move(name)) {}
        ~BasicBlock() = default;

        std::string name;
        std::vector<std::unique_ptr<Instruction>> instructions;
        std::vector<BasicBlock *> predecessors;
        std::vector<BasicBlock *> successors;

        // For dominance calculation
        BasicBlock *idom = nullptr; // Immediate dominator
        std::vector<BasicBlock *> dominated; // Blocks dominated by this block
        std::unordered_set<BasicBlock *> dominance_frontier; // Dominance frontier

        void addInstruction(std::unique_ptr<Instruction> instr) {
            instr->parent = this;
            instructions.push_back(std::move(instr));
        }

        bool hasTerminator() const {
            if (instructions.empty())
                return false;
            Instruction::Type last_type = instructions.back()->type;
            return last_type == Instruction::Type::Return || last_type == Instruction::Type::Branch ||
                   last_type == Instruction::Type::Jump;
        }

        void addTerminator(std::unique_ptr<Instruction> terminator) {
            if (hasTerminator()) {
                return; // Block already has a terminator
            }
            addInstruction(std::move(terminator));

            // Update successors based on terminator type
            Instruction *term = instructions.back().get();
            if (term->type == Instruction::Type::Branch) {
                auto branch = static_cast<BranchInstruction *>(term);
                if (branch->true_target) {
                    successors.push_back(branch->true_target);
                    branch->true_target->predecessors.push_back(this);
                }
                if (branch->false_target) {
                    successors.push_back(branch->false_target);
                    branch->false_target->predecessors.push_back(this);
                }
            } else if (term->type == Instruction::Type::Jump) {
                auto jump = static_cast<JumpInstruction *>(term);
                if (jump->target) {
                    successors.push_back(jump->target);
                    jump->target->predecessors.push_back(this);
                }
            }
        }

        // Check if this block dominates another block
        bool dominates(BasicBlock *other) const;

        // For debugging
        std::string toString() const;
    };

    // Control flow graph for a function
    class ControlFlowGraph {
    public:
        ControlFlowGraph() = default;
        ~ControlFlowGraph() = default;

        // Create a new basic block
        BasicBlock *createBlock(const std::string &name) {
            auto block = std::make_unique<BasicBlock>(name);
            BasicBlock *result = block.get();
            blocks.push_back(std::move(block));
            return result;
        }

        // Get the entry block
        BasicBlock *getEntryBlock() const { return blocks.empty() ? nullptr : blocks.front().get(); }

        // Get all blocks in the CFG
        std::vector<BasicBlock *> getBlocks() const {
            std::vector<BasicBlock *> result;
            for (const auto &block: blocks) {
                result.push_back(block.get());
            }
            return result;
        }

        // Compute dominance information
        void computeDominance();

        // Compute dominance frontiers
        void computeDominanceFrontiers();

        // For debugging
        std::string toString() const;

    private:
        std::vector<std::unique_ptr<BasicBlock>> blocks;
    };

} // namespace cfg

#endif // CONTROL_FLOW_GRAPH_H
