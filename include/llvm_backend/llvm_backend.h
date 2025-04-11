#ifndef LLVM_BACKEND_H
#define LLVM_BACKEND_H

#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <unordered_map>
#include "../ast.h"
#include "../ast_visitor.h"
#include "js_runtime.h"
#include "js_type_mapping.h"

/**
 * LLVMBackend - Converts JavaScript AST to LLVM IR
 *
 * This class implements the AST visitor pattern to generate LLVM IR
 * for JavaScript code. It handles type conversion, control flow,
 * and calling conventions for the JavaScript runtime.
 */
class LLVMBackend : public ASTVisitor {
public:
    /**
     * Constructor initializes LLVM context, module, and builder
     * @param moduleName The name for the LLVM module
     */
    LLVMBackend(const std::string &moduleName = "js_module");

    /**
     * Destructor cleans up LLVM resources
     */
    ~LLVMBackend();

    /**
     * Compile an entire program into LLVM IR
     * @param program The root of the AST
     * @return True if compilation succeeds
     */
    bool compile(const Program &program);

    /**
     * Get the generated LLVM IR as a string
     * @return LLVM IR text representation
     */
    std::string getIR() const;

    /**
     * Apply LLVM optimizations to the generated code
     * @param level Optimization level (0=none, 3=aggressive)
     * @return True if optimization succeeds
     */
    bool optimize(int level = 2);

    /**
     * JIT compile and execute the module
     * @return Result of execution as a double value
     */
    double executeJIT();

    /**
     * Create an executable file from the module
     * @param filename Output file name
     * @return True if successful
     */
    bool createExecutable(const std::string &filename);

    // AST visitor methods for expressions
    llvm::Value *visitLiteralExpr(const LiteralExpr &expr) override;
    llvm::Value *visitVariableExpr(const VariableExpr &expr) override;
    llvm::Value *visitBinaryExpr(const BinaryExpr &expr) override;
    llvm::Value *visitUnaryExpr(const UnaryExpr &expr) override;
    llvm::Value *visitCallExpr(const CallExpr &expr) override;
    llvm::Value *visitGetExpr(const GetExpr &expr) override;
    llvm::Value *visitAssignExpr(const AssignExpr &expr) override;
    llvm::Value *visitArrayExpr(const ArrayExpr &expr) override;
    llvm::Value *visitObjectExpr(const ObjectExpr &expr) override;

    // AST visitor methods for statements
    void visitExpressionStmt(const ExpressionStmt &stmt) override;
    void visitVarDeclStmt(const VarDeclStmt &stmt) override;
    void visitIfStmt(const IfStmt &stmt) override;
    void visitWhileStmt(const WhileStmt &stmt) override;
    void visitForStmt(const ForStmt &stmt) override;
    void visitReturnStmt(const ReturnStmt &stmt) override;
    void visitBlockStmt(const BlockStmt &stmt) override;
    void visitFunctionDeclStmt(const FunctionDeclStmt &stmt) override;
    void visitProgram(const Program &program) override;

private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    JSTypeMapping typeMapping;
    std::unique_ptr<JSRuntime> runtime;
    std::unique_ptr<llvm::orc::LLJIT> jit;

    // Current function being compiled
    llvm::Function *currentFunction = nullptr;

    // Symbol table for variable lookups
    std::unordered_map<std::string, llvm::AllocaInst *> namedValues;

    // Runtime function declarations
    std::unordered_map<std::string, llvm::Function *> runtimeFunctions;

    // Helper methods for code generation
    void declareRuntimeFunctions();
    llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *func, const std::string &name);

    // Value creation helpers
    llvm::Value *createJSUndefined();
    llvm::Value *createJSNull();
    llvm::Value *createJSBoolean(bool value);
    llvm::Value *createJSNumber(double value);
    llvm::Value *createJSString(const std::string &value);

    // Type conversion helpers
    llvm::Value *createJSValueToBoolean(llvm::Value *value);
    llvm::Value *createJSValueToDouble(llvm::Value *value);
    llvm::Value *createDoubleToJSValue(llvm::Value *value);

    // JIT compilation helpers
    bool initializeJIT();
};

#endif // LLVM_BACKEND_H
