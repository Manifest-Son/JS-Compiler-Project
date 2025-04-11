#ifndef JS_RUNTIME_H
#define JS_RUNTIME_H

#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <string>
#include <unordered_map>

/**
 * JSRuntime - Provides JavaScript runtime functions for the LLVM backend
 *
 * This class manages the declaration and integration of runtime functions
 * that implement JavaScript operations that can't be directly expressed in LLVM IR.
 */
class JSRuntime {
public:
    /**
     * Constructor initializes runtime functions
     * @param context LLVM context for creating declarations
     * @param module LLVM module where functions should be declared
     */
    JSRuntime(llvm::LLVMContext &context, llvm::Module *module);

    /**
     * Get a runtime function by name
     * @param name The function name
     * @return Pointer to the LLVM Function, or nullptr if not found
     */
    llvm::Function *getFunction(const std::string &name) const;

    /**
     * Declare all runtime functions in the module
     */
    void declareAll();

    /**
     * Get a list of all functions that need to be linked
     * @return Map of function names to LLVM functions
     */
    const std::unordered_map<std::string, llvm::Function *> &getAllFunctions() const;

private:
    llvm::LLVMContext &context;
    llvm::Module *module;
    std::unordered_map<std::string, llvm::Function *> functions;

    // Helper methods for declaring functions
    void declareTypeOperations();
    void declareObjectOperations();
    void declareStringOperations();
    void declareArrayOperations();
    void declareArithmeticOperations();
    void declareComparisonOperations();
    void declareControlFlowOperations();

    // Helper method to declare a function
    llvm::Function *declareFunction(const std::string &name, llvm::Type *returnType,
                                    const std::vector<llvm::Type *> &paramTypes);
};

#endif // JS_RUNTIME_H
