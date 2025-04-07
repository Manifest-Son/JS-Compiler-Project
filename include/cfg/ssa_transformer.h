#ifndef SSA_TRANSFORMER_H
#define SSA_TRANSFORMER_H

#include <map>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "control_flow_graph.h"

namespace cfg {

    // Class for transforming a Control Flow Graph into SSA form
    class SSATransformer {
    public:
        SSATransformer() = default;
        ~SSATransformer() = default;

        // Transform the given CFG into SSA form
        void transformToSSA(ControlFlowGraph &cfg);

    private:
        // Compute the dominance frontiers for each block in the CFG
        void computeDominanceFrontiers(ControlFlowGraph &cfg);

        // Insert phi nodes at appropriate points in the CFG
        void insertPhiNodes(ControlFlowGraph &cfg);

        // Rename variables with incremental versions
        void renameVariables(ControlFlowGraph &cfg);

        // Recursive helper for variable renaming
        void renameVariablesRecursive(BasicBlock *block, std::map<std::string, std::stack<int>> &var_versions);

        // Get a new version number for a variable
        int getNextVersion(const std::string &var, std::map<std::string, int> &current_version);

        // Map to track the next version number for each variable
        std::map<std::string, int> variable_counter_;

        // Set of variables that need phi nodes
        std::unordered_set<std::string> variables_needing_phis_;

        // Original variable to SSA variable mapping
        std::unordered_map<std::string, std::vector<std::string>> ssa_variables_;

        // For each block and variable, store the version number(s) reaching that block
        std::unordered_map<BasicBlock *, std::unordered_map<std::string, std::vector<int>>> reaching_definitions_;
    };

    // Data flow analysis framework for SSA-related analyses
    template<typename ValueType>
    class DataFlowAnalysis {
    public:
        virtual ~DataFlowAnalysis() = default;

        void analyze(ControlFlowGraph &cfg) {
            // Initialize data flow values
            initialize(cfg);

            // Iterative algorithm until fixed point
            bool changed = true;
            while (changed) {
                changed = false;
                for (auto *block: cfg.getBlocks()) {
                    // Compute input state by joining predecessor outputs
                    ValueType in = computeInput(block);

                    // Transfer function for this block
                    ValueType out = transferFunction(block, in);

                    // Check if output changed
                    if (out != block_outputs_[block]) {
                        block_outputs_[block] = out;
                        changed = true;
                    }
                }
            }
        }

        // Get the output value for a block
        const ValueType &getOutput(BasicBlock *block) const {
            auto it = block_outputs_.find(block);
            static ValueType empty_value;
            return it != block_outputs_.end() ? it->second : empty_value;
        }

    protected:
        // Abstract methods to be implemented by concrete analyses
        virtual void initialize(ControlFlowGraph &cfg) = 0;
        virtual ValueType computeInput(BasicBlock *block) = 0;
        virtual ValueType transferFunction(BasicBlock *block, const ValueType &in) = 0;

        std::map<BasicBlock *, ValueType> block_outputs_;
    };

    // Live variable analysis - identifies which variables are live at each point in the program
    class LiveVariableAnalysis : public DataFlowAnalysis<std::unordered_set<std::string>> {
    protected:
        void initialize(ControlFlowGraph &cfg) override {
            // Initialize all blocks with empty sets
            for (auto *block: cfg.getBlocks()) {
                block_outputs_[block] = std::unordered_set<std::string>();
            }
        }

        std::unordered_set<std::string> computeInput(BasicBlock *block) override {
            // Live variables at the beginning of a block =
            // Union of live variables at the end of all successor blocks
            std::unordered_set<std::string> result;
            for (auto *succ: block->successors) {
                const auto &succ_live = getOutput(succ);
                result.insert(succ_live.begin(), succ_live.end());
            }
            return result;
        }

        std::unordered_set<std::string> transferFunction(BasicBlock *block,
                                                         const std::unordered_set<std::string> &out) override {
            // Process instructions in reverse order
            std::unordered_set<std::string> current = out;

            for (auto it = block->instructions.rbegin(); it != block->instructions.rend(); ++it) {
                const Instruction *instr = it->get();

                // First add used variables to the live set
                for (const auto &var: instr->getUsedVars()) {
                    current.insert(var);
                }

                // Then remove defined variables from the live set
                for (const auto &var: instr->getDefinedVars()) {
                    current.erase(var);
                }
            }

            return current;
        }
    };

    // Reaching definitions analysis - identifies which definitions reach each point in the program
    class ReachingDefinitionsAnalysis
        : public DataFlowAnalysis<std::unordered_map<std::string, std::unordered_set<const Instruction *>>> {
    protected:
        void initialize(ControlFlowGraph &cfg) override {
            // Initialize with all definition points
            for (auto *block: cfg.getBlocks()) {
                block_outputs_[block] = std::unordered_map<std::string, std::unordered_set<const Instruction *>>();
            }
        }

        std::unordered_map<std::string, std::unordered_set<const Instruction *>>
        computeInput(BasicBlock *block) override {
            std::unordered_map<std::string, std::unordered_set<const Instruction *>> result;

            // For entry block, no reaching definitions from predecessors
            if (block->predecessors.empty()) {
                return result;
            }

            // For each predecessor, union the reaching definitions
            for (auto *pred: block->predecessors) {
                const auto &pred_defs = getOutput(pred);

                // Union all definitions from the predecessor
                for (const auto &[var, defs]: pred_defs) {
                    auto &target_set = result[var];
                    target_set.insert(defs.begin(), defs.end());
                }
            }

            return result;
        }

        std::unordered_map<std::string, std::unordered_set<const Instruction *>>
        transferFunction(BasicBlock *block,
                         const std::unordered_map<std::string, std::unordered_set<const Instruction *>> &in) override {
            // Start with input definitions
            std::unordered_map<std::string, std::unordered_set<const Instruction *>> current = in;

            // Process instructions in order
            for (const auto &instr: block->instructions) {
                // For each defined variable in this instruction:
                for (const auto &var: instr->getDefinedVars()) {
                    // Remove all previous definitions and add this one
                    current[var] = {instr.get()};
                }
            }

            return current;
        }
    };

} // namespace cfg

#endif // SSA_TRANSFORMER_H
