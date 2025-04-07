#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include "../include/ast_printer.h"
#include "../include/cfg/cfg_builder.h"
#include "../include/cfg/dataflow_analyses.h"
#include "../include/cfg/ssa_transformer.h"
#include "../include/parser.h"
#include "../include/lexer.h"

// Forward declarations
void printAnalysisResults(const cfg::ControlFlowGraph &cfg);
void applyConstantPropagation(cfg::ControlFlowGraph &cfg, const cfg::ConstantPropagationAnalysis &analysis);
void eliminateCommonSubexpressions(cfg::ControlFlowGraph &cfg, const cfg::AvailableExpressionsAnalysis &analysis);
void eliminateDeadCode(cfg::ControlFlowGraph &cfg, cfg::DeadCodeAnalysis &analysis);

// Main function demonstrating dataflow analysis usage
int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <js_file>" << std::endl;
        return 1;
    }

    // Read JavaScript source code from file
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    try {
        std::cout << "=== Original JavaScript Code ===" << std::endl;
        std::cout << source << std::endl << std::endl;

        // Create a lexer with the source code
        Lexer lexer(source);

        // Create parser with the lexer
        Parser parser(lexer);

        // Parse the JavaScript source code to build an AST
        auto program = parser.parse();

        // Build control flow graph from the AST
        std::cout << "Building Control Flow Graph..." << std::endl;
        cfg::CFGBuilder builder;
        auto cfg = builder.buildCFG(*program);

        std::cout << "=== Original Control Flow Graph ===" << std::endl;
        std::cout << cfg->toString() << std::endl;

        // Transform the CFG to SSA form
        std::cout << "Transforming to SSA form..." << std::endl;
        cfg::SSATransformer ssa_transformer;
        ssa_transformer.transformToSSA(*cfg);

        std::cout << "=== Control Flow Graph in SSA Form ===" << std::endl;
        std::cout << cfg->toString() << std::endl;

        // Print analysis results before optimization
        std::cout << "=== Analysis Results Before Optimization ===" << std::endl;
        printAnalysisResults(*cfg);

        // Apply optimizations
        std::cout << "=== Applying Optimizations ===" << std::endl;

        // Run constant propagation
        std::cout << "Running constant propagation..." << std::endl;
        cfg::ConstantPropagationAnalysis cp_analysis;
        cp_analysis.analyze(*cfg);
        applyConstantPropagation(*cfg, cp_analysis);

        // Run common subexpression elimination
        std::cout << "Eliminating common subexpressions..." << std::endl;
        cfg::AvailableExpressionsAnalysis avail_expr_analysis;
        avail_expr_analysis.analyze(*cfg);
        eliminateCommonSubexpressions(*cfg, avail_expr_analysis);

        // Run dead code elimination
        std::cout << "Eliminating dead code..." << std::endl;
        cfg::DeadCodeAnalysis dc_analysis;
        dc_analysis.analyze(*cfg);
        eliminateDeadCode(*cfg, dc_analysis);

        std::cout << "=== Optimized Control Flow Graph ===" << std::endl;
        std::cout << cfg->toString() << std::endl;

        // Print analysis results after optimization
        std::cout << "=== Analysis Results After Optimization ===" << std::endl;
        printAnalysisResults(*cfg);

        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

// Function to print results of various analyses
void printAnalysisResults(const cfg::ControlFlowGraph &cfg) {
    // Run reaching definitions analysis
    cfg::ReachingDefinitionsAnalysis rd_analysis;
    rd_analysis.analyze(const_cast<cfg::ControlFlowGraph &>(cfg));

    // Run live variables analysis
    cfg::LiveVariableAnalysis lv_analysis;
    lv_analysis.analyze(const_cast<cfg::ControlFlowGraph &>(cfg));

    // Run available expressions analysis
    cfg::AvailableExpressionsAnalysis ae_analysis;
    ae_analysis.analyze(const_cast<cfg::ControlFlowGraph &>(cfg));

    // Run constant propagation analysis
    cfg::ConstantPropagationAnalysis cp_analysis;
    cp_analysis.analyze(const_cast<cfg::ControlFlowGraph &>(cfg));

    // For each block in the CFG
    for (auto *block: cfg.getBlocks()) {
        std::cout << "Block: " << block->name << std::endl;

        // Print live variables
        std::cout << "  Live Variables: ";
        const auto &live_vars = lv_analysis.getOutput(block);
        for (const auto &var: live_vars) {
            std::cout << var << " ";
        }
        std::cout << std::endl;

        // Print constant variables
        std::cout << "  Constant Variables: ";
        const auto &constants = cp_analysis.getOutput(block);
        for (const auto &[var, value]: constants) {
            if (!cfg::ConstantPropagationAnalysis::isNAC(value) && !std::holds_alternative<std::monostate>(value)) {
                std::cout << var << "=" << cfg::ConstantPropagationAnalysis::valueToString(value) << " ";
            }
        }
        std::cout << std::endl;

        // Print available expressions
        std::cout << "  Available Expressions at Entry: ";
        auto avail_exprs = ae_analysis.getOutput(block);
        for (const auto &expr: avail_exprs) {
            std::cout << expr.toString() << " ";
        }
        std::cout << std::endl;

        std::cout << std::endl;
    }
}

// Function to apply constant propagation optimization
void applyConstantPropagation(cfg::ControlFlowGraph &cfg, const cfg::ConstantPropagationAnalysis &analysis) {
    // For each block in the CFG
    for (auto *block: cfg.getBlocks()) {
        const auto &constants = analysis.getOutput(block);

        // For each instruction in the block
        for (auto &instr: block->instructions) {
            // Skip phi nodes (they're handled differently in SSA)
            if (instr->type == cfg::Instruction::Type::Phi)
                continue;

            // Handle binary instructions
            if (instr->type == cfg::Instruction::Type::Binary) {
                auto *binary = static_cast<cfg::BinaryInstruction *>(instr.get());

                // Replace left operand if it's a constant
                auto left_it = constants.find(binary->left);
                if (left_it != constants.end() && !cfg::ConstantPropagationAnalysis::isNAC(left_it->second) &&
                    !std::holds_alternative<std::monostate>(left_it->second)) {

                    binary->left = cfg::ConstantPropagationAnalysis::valueToString(left_it->second);
                    std::cout << "  Propagated constant: " << binary->left << " in " << binary->toString() << std::endl;
                }

                // Replace right operand if it's a constant
                auto right_it = constants.find(binary->right);
                if (right_it != constants.end() && !cfg::ConstantPropagationAnalysis::isNAC(right_it->second) &&
                    !std::holds_alternative<std::monostate>(right_it->second)) {

                    binary->right = cfg::ConstantPropagationAnalysis::valueToString(right_it->second);
                    std::cout << "  Propagated constant: " << binary->right << " in " << binary->toString()
                              << std::endl;
                }
            }

            // Handle unary instructions
            else if (instr->type == cfg::Instruction::Type::Unary) {
                auto *unary = static_cast<cfg::UnaryInstruction *>(instr.get());

                // Replace operand if it's a constant
                auto operand_it = constants.find(unary->operand);
                if (operand_it != constants.end() && !cfg::ConstantPropagationAnalysis::isNAC(operand_it->second) &&
                    !std::holds_alternative<std::monostate>(operand_it->second)) {

                    unary->operand = cfg::ConstantPropagationAnalysis::valueToString(operand_it->second);
                    std::cout << "  Propagated constant: " << unary->operand << " in " << unary->toString()
                              << std::endl;
                }
            }

            // Handle branch instructions
            else if (instr->type == cfg::Instruction::Type::Branch) {
                auto *branch = static_cast<cfg::BranchInstruction *>(instr.get());

                // Replace condition if it's a constant
                auto cond_it = constants.find(branch->condition);
                if (cond_it != constants.end() && !cfg::ConstantPropagationAnalysis::isNAC(cond_it->second) &&
                    std::holds_alternative<bool>(cond_it->second)) {

                    bool cond_value = std::get<bool>(cond_it->second);
                    std::cout << "  Constant condition: " << branch->condition << " = "
                              << (cond_value ? "true" : "false") << std::endl;

                    // TODO: Convert branch to unconditional jump
                    // This would require more complex CFG restructuring
                }
            }

            // Handle return instructions
            else if (instr->type == cfg::Instruction::Type::Return) {
                auto *ret = static_cast<cfg::ReturnInstruction *>(instr.get());

                // Replace return value if it's a constant
                if (!ret->value.empty()) {
                    auto value_it = constants.find(ret->value);
                    if (value_it != constants.end() && !cfg::ConstantPropagationAnalysis::isNAC(value_it->second) &&
                        !std::holds_alternative<std::monostate>(value_it->second)) {

                        ret->value = cfg::ConstantPropagationAnalysis::valueToString(value_it->second);
                        std::cout << "  Propagated constant in return: " << ret->toString() << std::endl;
                    }
                }
            }
        }
    }
}

// Function to eliminate common subexpressions
void eliminateCommonSubexpressions(cfg::ControlFlowGraph &cfg, const cfg::AvailableExpressionsAnalysis &analysis) {
    // For each block in the CFG
    for (auto *block: cfg.getBlocks()) {
        // Get available expressions at the beginning of this block
        auto avail_exprs = analysis.getOutput(block);

        // Track expressions computed in this block
        std::unordered_map<cfg::Expression, std::string, cfg::ExpressionHash> expr_to_var;

        // For each instruction in the block
        for (auto it = block->instructions.begin(); it != block->instructions.end();) {
            auto &instr = *it;

            // Check binary operations for CSE
            if (instr->type == cfg::Instruction::Type::Binary) {
                auto *binary = static_cast<cfg::BinaryInstruction *>(instr.get());

                // Create expression object
                cfg::Expression expr(binary->op, binary->left, binary->right);

                // Check if this expression is already available
                auto avail_it = avail_exprs.find(expr);
                if (avail_it != avail_exprs.end()) {
                    // Check if we have already computed this expression in this block
                    auto local_it = expr_to_var.find(expr);
                    if (local_it != expr_to_var.end()) {
                        // Replace with an assignment from the already computed value
                        auto temp_var = local_it->second;
                        std::cout << "  Eliminated common subexpression: " << expr.toString() << " using " << temp_var
                                  << std::endl;

                        // Replace with assignment (would be better to actually replace the instruction)
                        auto assign = std::make_unique<cfg::AssignInstruction>(binary->result, temp_var);
                        assign->version = binary->version;
                        *it = std::move(assign);
                        ++it;
                        continue;
                    }
                }

                // Remember this expression for later use in this block
                expr_to_var[expr] = binary->result;
            }

            // Add the expression to available expressions
            if (instr->type == cfg::Instruction::Type::Binary) {
                auto *binary = static_cast<cfg::BinaryInstruction *>(instr.get());
                avail_exprs.emplace(binary->op, binary->left, binary->right);
            } else if (instr->type == cfg::Instruction::Type::Unary) {
                auto *unary = static_cast<cfg::UnaryInstruction *>(instr.get());
                avail_exprs.emplace(unary->op, unary->operand);
            }

            // Remove expressions invalidated by this instruction
            const auto &defined_vars = instr->getDefinedVars();
            if (!defined_vars.empty()) {
                std::vector<cfg::Expression> to_remove;

                for (const auto &expr: avail_exprs) {
                    if (expr.type == cfg::Expression::Type::Binary) {
                        for (const auto &var: defined_vars) {
                            if (expr.left == var || expr.right == var) {
                                to_remove.push_back(expr);
                                break;
                            }
                        }
                    } else if (expr.type == cfg::Expression::Type::Unary) {
                        for (const auto &var: defined_vars) {
                            if (expr.left == var) {
                                to_remove.push_back(expr);
                                break;
                            }
                        }
                    }
                }

                for (const auto &expr: to_remove) {
                    avail_exprs.erase(expr);
                    expr_to_var.erase(expr);
                }
            }

            ++it;
        }
    }
}

// Function to eliminate dead code
void eliminateDeadCode(cfg::ControlFlowGraph &cfg,  cfg::DeadCodeAnalysis &analysis) {
    // For each block in the CFG
    for (auto *block: cfg.getBlocks()) {
        // Get unused definitions in this block
        auto unused = analysis.getUnusedDefinitions(block);

        if (!unused.empty()) {
            std::cout << "  Found " << unused.size() << " dead instructions in block " << block->name << std::endl;

            // Mark instructions for removal
            // In a real implementation, we would actually remove them
            for (const auto *instr: unused) {
                std::cout << "    Dead instruction: " << instr->toString() << std::endl;

                // Actually removing instructions would require more careful handling
                // especially with SSA form and maintaining CFG integrity
            }
        }
    }
}

