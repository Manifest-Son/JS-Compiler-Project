#include "../../include/cfg/control_flow_graph.h"
#include <algorithm>
#include <queue>
#include <sstream>

namespace cfg {

    // PhiInstruction to string representation
    std::string PhiInstruction::toString() const {
        std::stringstream ss;
        ss << result_var << "_" << version << " = phi(";
        for (size_t i = 0; i < source_vars.size(); ++i) {
            if (i > 0)
                ss << ", ";
            ss << source_vars[i];
            if (i < source_blocks.size()) {
                ss << " [" << source_blocks[i]->name << "]";
            }
        }
        ss << ")";
        return ss.str();
    }

    // AssignInstruction to string representation
    std::string AssignInstruction::toString() const { return target + "_" + std::to_string(version) + " = " + source; }

    // BinaryInstruction to string representation
    std::string BinaryInstruction::toString() const {
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
        return result + "_" + std::to_string(version) + " = " + left + " " + op_str + " " + right;
    }

    // UnaryInstruction to string representation
    std::string UnaryInstruction::toString() const {
        std::string op_str;
        switch (op) {
            case TokenType::MINUS:
                op_str = "-";
                break;
            case TokenType::BANG:
                op_str = "!";
                break;
            case TokenType::PLUS_PLUS:
                op_str = "++";
                break;
            case TokenType::MINUS_MINUS:
                op_str = "--";
                break;
            default:
                op_str = "?";
                break;
        }
        return result + "_" + std::to_string(version) + " = " + op_str + operand;
    }

    // CallInstruction to string representation
    std::string CallInstruction::toString() const {
        std::stringstream ss;
        ss << result << "_" << version << " = " << callee << "(";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0)
                ss << ", ";
            ss << args[i];
        }
        ss << ")";
        return ss.str();
    }

    // ReturnInstruction to string representation
    std::string ReturnInstruction::toString() const {
        if (is_implicit) {
            return "return; // implicit";
        }
        return value.empty() ? "return;" : "return " + value + ";";
    }

    // BranchInstruction to string representation
    std::string BranchInstruction::toString() const {
        std::stringstream ss;
        ss << "if (" << condition << ") goto ";
        if (true_target) {
            ss << true_target->name;
        } else {
            ss << "null";
        }
        ss << "; else goto ";
        if (false_target) {
            ss << false_target->name;
        } else {
            ss << "null";
        }
        return ss.str();
    }

    // JumpInstruction to string representation
    std::string JumpInstruction::toString() const { return "goto " + (target ? target->name : "null"); }

    // Check if this block dominates another block
    bool BasicBlock::dominates(BasicBlock *other) const {
        if (this == other) {
            return true; // A block always dominates itself
        }

        // Walk up the immediate dominator tree from 'other'
        BasicBlock *runner = other->idom;
        while (runner && runner != this) {
            runner = runner->idom;
        }
        return runner == this;
    }

    // BasicBlock to string representation
    std::string BasicBlock::toString() const {
        std::stringstream ss;
        ss << name << ":\n";

        // List predecessors
        ss << "  // Predecessors: ";
        for (size_t i = 0; i < predecessors.size(); ++i) {
            if (i > 0)
                ss << ", ";
            ss << predecessors[i]->name;
        }
        ss << "\n";

        // List instructions
        for (const auto &instr: instructions) {
            ss << "  " << instr->toString() << "\n";
        }

        // List successors
        ss << "  // Successors: ";
        for (size_t i = 0; i < successors.size(); ++i) {
            if (i > 0)
                ss << ", ";
            ss << successors[i]->name;
        }
        ss << "\n";

        // List dominance frontier if computed
        if (!dominance_frontier.empty()) {
            ss << "  // Dominance frontier: ";
            bool first = true;
            for (BasicBlock *df: dominance_frontier) {
                if (!first)
                    ss << ", ";
                ss << df->name;
                first = false;
            }
            ss << "\n";
        }

        return ss.str();
    }

    // Compute dominance information for the CFG
    void ControlFlowGraph::computeDominance() {
        std::vector<BasicBlock *> block_list = getBlocks();
        if (block_list.empty())
            return;

        BasicBlock *entry = block_list[0];

        // Initialize dominators
        std::unordered_map<BasicBlock *, std::unordered_set<BasicBlock *>> dominators;
        for (auto *block: block_list) {
            dominators[block] = std::unordered_set<BasicBlock *>(block_list.begin(), block_list.end());
        }

        // Entry block is only dominated by itself
        dominators[entry] = {entry};

        // Iterative algorithm to find dominators
        bool changed = true;
        while (changed) {
            changed = false;

            // Skip entry block (already processed)
            for (size_t i = 1; i < block_list.size(); ++i) {
                BasicBlock *block = block_list[i];

                // Start with all blocks as potential dominators
                std::unordered_set<BasicBlock *> new_doms = {block}; // Self-domination

                // If block has predecessors, intersect their dominators
                if (!block->predecessors.empty()) {
                    std::unordered_set<BasicBlock *> pred_doms = dominators[block->predecessors[0]];

                    for (size_t j = 1; j < block->predecessors.size(); ++j) {
                        std::unordered_set<BasicBlock *> intersection;
                        const auto &pred_set = dominators[block->predecessors[j]];

                        for (auto *dom: pred_doms) {
                            if (pred_set.find(dom) != pred_set.end()) {
                                intersection.insert(dom);
                            }
                        }

                        pred_doms = std::move(intersection);
                    }

                    // Add the block itself to the intersection
                    new_doms.insert(pred_doms.begin(), pred_doms.end());
                }

                // Check if the dominators changed
                if (new_doms != dominators[block]) {
                    dominators[block] = std::move(new_doms);
                    changed = true;
                }
            }
        }

        // Compute immediate dominators and dominated sets
        for (auto *block: block_list) {
            // Skip entry block
            if (block == entry)
                continue;

            // Find the immediate dominator
            BasicBlock *idom = nullptr;
            for (auto *potential_idom: dominators[block]) {
                // Skip self
                if (potential_idom == block)
                    continue;

                // Check if it's an immediate dominator
                bool is_immediate = true;
                for (auto *other_dom: dominators[block]) {
                    if (other_dom != block && other_dom != potential_idom &&
                        dominators[other_dom].find(potential_idom) != dominators[other_dom].end()) {
                        is_immediate = false;
                        break;
                    }
                }

                if (is_immediate) {
                    idom = potential_idom;
                    break;
                }
            }

            // Set immediate dominator
            block->idom = idom;

            // Add to dominated set of immediate dominator
            if (idom) {
                idom->dominated.push_back(block);
            }
        }
    }

    // Compute dominance frontiers for each block in the CFG
    void ControlFlowGraph::computeDominanceFrontiers() {
        // Ensure dominance information is computed
        if (getBlocks()[0]->dominated.empty()) {
            computeDominance();
        }

        for (auto *block: getBlocks()) {
            // Skip blocks with fewer than two predecessors
            if (block->predecessors.size() < 2)
                continue;

            // For each predecessor
            for (auto *pred: block->predecessors) {
                // Walk up the dominator tree until we hit the immediate dominator of block
                BasicBlock *runner = pred;
                while (runner && runner != block->idom) {
                    runner->dominance_frontier.insert(block);
                    runner = runner->idom;
                }
            }
        }
    }

    // ControlFlowGraph to string representation
    std::string ControlFlowGraph::toString() const {
        std::stringstream ss;
        ss << "Control Flow Graph:\n";
        ss << "=================\n\n";

        for (const auto &block: blocks) {
            ss << block->toString() << "\n";
        }

        return ss.str();
    }

} // namespace cfg
