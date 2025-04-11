#ifndef COMPILER_DRIVER_H
#define COMPILER_DRIVER_H

#include <memory>
#include <string>
#include "cfg/cfg_builder.h"
#include "cfg/ssa_transformer.h"
#include "error_reporter.h"
#include "lexer.h"
#include "llvm_backend/llvm_backend.h"
#include "parser.h"
#include "rust_memory/include/js_memory_manager.hpp"

/**
 * CompilerDriver - Main class that coordinates the compilation process
 *
 * This class manages the entire compilation pipeline from source code
 * to executable code, integrating all compiler components.
 */
class CompilerDriver {
public:
    /**
     * Constructor initializes the compiler components
     * @param optimizationLevel Level of optimization to apply
     */
    CompilerDriver(int optimizationLevel = 2);

    /**
     * Destructor cleans up resources
     */
    ~CompilerDriver();

    /**
     * Compile JavaScript source code to executable code
     * @param source JavaScript source code
     * @param filename Source filename for error messages
     * @return True if compilation succeeds
     */
    bool compile(const std::string &source, const std::string &filename = "source.js");

    /**
     * Execute the compiled code
     * @return Result of execution
     */
    Value execute();

    /**
     * Get any error messages generated during compilation
     * @return Error messages
     */
    std::string getErrors() const;

    /**
     * Get the generated LLVM IR
     * @return LLVM IR as string
     */
    std::string getIR() const;

    /**
     * Get performance statistics about the compilation
     * @return Statistics object
     */
    CompilationStats getStats() const;

private:
    int optimizationLevel;
    ErrorReporter errorReporter;
    std::unique_ptr<LLVMBackend> llvmBackend;
    js_memory::MemoryManager &memoryManager;

    // Compilation statistics
    CompilationStats stats;

    // Helper methods
    void initializeMemoryManager();
    void runOptimizationPasses(std::unique_ptr<cfg::ControlFlowGraph> &cfg);
};

#endif // COMPILER_DRIVER_H
