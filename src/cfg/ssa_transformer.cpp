#include "../../include/cfg/ssa_transformer.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <queue>

namespace cfg {

    // Transform the CFG into SSA form
    void SSATransformer::transformToSSA(ControlFlowGraph &cfg) {
        // Compute dominance information if not already computed
        cfg.computeDominance();

        // Compute dominance frontiers
        cfg.computeDominanceFrontiers();

        // Reset state
        variable_counter_.clear();
        variables_needing_phis_.clear();
        ssa_variables_.clear();
        reaching_definitions_.clear();

        // Find all variables defined in the CFG
        for (auto *block: cfg.getBlocks()) {
            for (const auto &instr: block->instructions) {
                for (const auto &var: instr->getDefinedVars()) {
                    if (!var.empty()) {
                        variables_needing_phis_.insert(var);
                    }
                }
            }
        }

        // Insert phi nodes at appropriate points
        insertPhiNodes(cfg);

        // Rename variables
        renameVariables(cfg);
    }

    // Insert phi nodes at appropriate points in the CFG
    void SSATransformer::insertPhiNodes(ControlFlowGraph &cfg) {
        // For each variable that might need phi nodes
        for (const auto &var: variables_needing_phis_) {
            // Keep track of blocks that have definitions of this variable
            std::unordered_set<BasicBlock *> blocks_with_defs;

            // Find all blocks that define this variable
            for (auto *block: cfg.getBlocks()) {
                for (const auto &instr: block->instructions) {
                    for (const auto &defined_var: instr->getDefinedVars()) {
                        if (defined_var == var) {
                            blocks_with_defs.insert(block);
                            break;
                        }
                    }
                }
            }

            // Work list for the iterative algorithm
            std::queue<BasicBlock *> work_list;
            for (auto *block: blocks_with_defs) {
                work_list.push(block);
            }

            // Keep track of blocks that already have phi nodes for this variable
            std::unordered_set<BasicBlock *> blocks_with_phi;

            // Iteratively place phi nodes in dominance frontiers
            while (!work_list.empty()) {
                BasicBlock *block = work_list.front();
                work_list.pop();

                // For each block in the dominance frontier
                for (BasicBlock *df: block->dominance_frontier) {
                    // If we haven't already placed a phi node for this variable in this block
                    if (blocks_with_phi.find(df) == blocks_with_phi.end()) {
                        // Create phi node
                        std::vector<std::string> source_vars;
                        std::vector<BasicBlock *> source_blocks;

                        // Add a placeholder for each predecessor (will be filled in during renaming)
                        for (BasicBlock *pred: df->predecessors) {
                            source_vars.push_back(var);
                            source_blocks.push_back(pred);
                        }

                        // Add the phi node to the beginning of the block
                        auto phi = std::make_unique<PhiInstruction>(var, source_vars, source_blocks);
                        df->instructions.insert(df->instructions.begin(), std::move(phi));

                        // Mark this block as having a phi node for this variable
                        blocks_with_phi.insert(df);

                        // This block now defines the variable, so add it to the work list
                        if (blocks_with_defs.find(df) == blocks_with_defs.end()) {
                            blocks_with_defs.insert(df);
                            work_list.push(df);
                        }
                    }
                }
            }
        }
    }

    // Rename variables with incremental versions
    void SSATransformer::renameVariables(ControlFlowGraph &cfg) {
        // Initialize variable counter
        for (const auto &var: variables_needing_phis_) {
            variable_counter_[var] = 0;
        }

        // Start the recursive renaming from the entry block
        std::map<std::string, std::stack<int>> var_versions;
        renameVariablesRecursive(cfg.getEntryBlock(), var_versions);
    }

    // Recursive helper for variable renaming
    void SSATransformer::renameVariablesRecursive(BasicBlock *block,
                                                  std::map<std::string, std::stack<int>> &var_versions) {
        // Process each instruction in this block
        for (auto &instr: block->instructions) {
            // For phi instructions, the LHS is renamed but the RHS is handled differently
            if (instr->type == Instruction::Type::Phi) {
                auto phi = static_cast<PhiInstruction *>(instr.get());

                // Give this phi node a new version number
                phi->version = getNextVersion(phi->target_var, variable_counter_);

                // Push this version onto the stack
                var_versions[phi->target_var].push(phi->version);
            } else {
                // For non-phi instructions, rename variable uses
                std::vector<std::string> used_vars = instr->getUsedVars();

                // Rename each used variable to its current version
                for (auto &used_var: used_vars) {
                    if (!used_var.empty() && var_versions.find(used_var) != var_versions.end() &&
                        !var_versions[used_var].empty()) {
                        // Append version to the variable (in a real implementation, we'd update the actual operand)
                        used_var += "_" + std::to_string(var_versions[used_var].top());
                    }
                }

                // Now rename variable definitions
                std::vector<std::string> defined_vars = instr->getDefinedVars();
                for (auto &defined_var: defined_vars) {
                    if (!defined_var.empty()) {
                        // Give this definition a new version number
                        instr->version = getNextVersion(defined_var, variable_counter_);

                        // Push this version onto the stack
                        var_versions[defined_var].push(instr->version);
                    }
                }
            }
        }

        // Handle phi nodes in successor blocks
        for (BasicBlock *succ: block->successors) {
            // Find the index of this block in the successor's predecessors
            size_t pred_index = 0;
            for (size_t i = 0; i < succ->predecessors.size(); i++) {
                if (succ->predecessors[i] == block) {
                    pred_index = i;
                    break;
                }
            }

            // For each phi node in the successor
            for (auto &instr: succ->instructions) {
                if (instr->type == Instruction::Type::Phi) {
                    auto phi = static_cast<PhiInstruction *>(instr.get());

                    // If this phi has fewer source blocks than expected, continue
                    if (pred_index >= phi->source_blocks.size())
                        continue;

                    // If this phi is for a variable we have a version for, use it
                    if (var_versions.find(phi->target_var) != var_versions.end() &&
                        !var_versions[phi->target_var].empty()) {
                        // Update the source variable with the current version
                        phi->source_vars[pred_index] =
                                phi->target_var + "_" + std::to_string(var_versions[phi->target_var].top());
                    }
                }
            }
        }

        // Recursively process dominated blocks
        for (BasicBlock *dominated: block->dominated) {
            // Make a copy of the current variable versions
            std::map<std::string, std::stack<int>> dominated_vars = var_versions;
            renameVariablesRecursive(dominated, dominated_vars);
        }

        // Pop variable versions defined in this block
        for (auto &instr: block->instructions) {
            for (const auto &var: instr->getDefinedVars()) {
                if (!var.empty() && var_versions.find(var) != var_versions.end() && !var_versions[var].empty()) {
                    var_versions[var].pop();
                }
            }
        }
    }

    // Get a new version number for a variable
    int SSATransformer::getNextVersion(const std::string &var, std::map<std::string, int> &current_version) {
        if (current_version.find(var) == current_version.end()) {
            current_version[var] = 0;
        }
        return current_version[var]++;
    }

} // namespace cfg
