#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "../include/ast_printer.h"
#include "../include/cfg/cfg_builder.h"
#include "../include/cfg/ssa_transformer.h"
#include "../include/parser.h"
#include "../include/lexer.h"

void identifyOptimizationOpportunities(const cfg::ControlFlowGraph &cfg);

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
        // Create a lexer with the source code
        Lexer lexer(source);

        // Create parser with the lexer
        Parser parser(lexer);

        // Parse the JavaScript source code to build an AST
        auto program = parser.parse();

        // Print the original AST
        ASTPrinter printer;
        std::cout << "=== Original AST ===" << std::endl;
        program->accept(printer);
        std::cout << std::endl;

        // Build control flow graph from the AST
        cfg::CFGBuilder builder;
        auto cfg = builder.buildCFG(*program);

        std::cout << "=== Control Flow Graph Before SSA ===" << std::endl;
        std::cout << cfg->toString() << std::endl;

        // Transform the CFG to SSA form
        cfg::SSATransformer transformer;
        transformer.transformToSSA(*cfg);

        std::cout << "=== Control Flow Graph After SSA ===" << std::endl;
        std::cout << cfg->toString() << std::endl;

        // Here you could add code to apply optimizations to the SSA form
        // For example:
        // ConstantPropagation cp;
        // cp.optimize(*cfg);
        //
        // DeadCodeElimination dce;
        // dce.optimize(*cfg);

        std::cout << "=== Optimization Opportunities ===" << std::endl;
        // Identify optimization opportunities
        identifyOptimizationOpportunities(*cfg);

        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

// Helper function to identify basic optimization opportunities in the CFG
void identifyOptimizationOpportunities(const cfg::ControlFlowGraph &cfg) {
    // Check for constant assignments that could be propagated
    for (auto *block: cfg.getBlocks()) {
        for (const auto &instr: block->instructions) {
            if (instr->type == cfg::Instruction::Type::Assign) {
                auto *assign = static_cast<const cfg::AssignInstruction *>(instr.get());
                // Check if source is a literal (simplified check for example purposes)
                if (assign->source.find_first_not_of("0123456789.") == std::string::npos) {
                    std::cout << "Constant Propagation Opportunity: " << assign->toString() << std::endl;
                }
            } else if (instr->type == cfg::Instruction::Type::Binary) {
                auto *binary = static_cast<const cfg::BinaryInstruction *>(instr.get());
                // Check for common subexpressions (would need more sophisticated analysis in practice)
                std::cout << "Potential CSE Opportunity: " << binary->toString() << std::endl;
            }
        }
    }
}

