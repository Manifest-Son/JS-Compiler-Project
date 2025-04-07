#ifndef LLVM_BACKEND_H
#define LLVM_BACKEND_H

#include "../ast.h"
#include "../ast_visitor.h"
#include "js_type_mapping.h"
#include "js_value_type.h"

#include <memory>
#include <string>
#include <unordered_map>

// Forward declarations for LLVM classes
namespace llvm {
    class LLVMContext;
    class Module;
    class IRBuilder;
    class Function;
    class BasicBlock;
    class Type;
    class Value;
    class StructType;
} // namespace llvm

/**
 * LLVMBackend - Generates LLVM IR from JavaScript AST
 *
 * This class is responsible for traversing the JavaScript AST and generating
 * equivalent LLVM IR code. It implements the ASTVisitor interface to visit
 * each node in the AST.
 */
class LLVMBackend : public ASTVisitor {
public:
    /**
     * Constructor initializes the LLVM context, module, and builder.
     * @param moduleName The name to give the generated LLVM module
     */
    LLVMBackend(const std::string &moduleName = "js_module");

    /**
     * Destructor cleans up LLVM resources
     */
    ~LLVMBackend();

    /**
     * Compile a JavaScript program to LLVM IR
     * @param program The parsed JavaScript program
     * @return True if compilation was successful, false otherwise
     */
    bool compile(const Program &program);

    /**
     * Apply optimizations to the generated LLVM IR
     * @param level Optimization level (0-3)
     * @return True if optimization was successful, false otherwise
     */
    bool optimize(int level);

    /**
     * Get the generated LLVM IR as a string
     * @return The LLVM IR as a string
     */
    std::string getIR() const;

    /**
     * Write the generated LLVM IR to a file
     * @param filename The path to write the LLVM IR to
     * @return True if the file was written successfully, false otherwise
     */
    bool writeIR(const std::string &filename) const;

    // Visitor methods for expressions
    llvm::Value *visitAssignExpr(const AssignExpr &expr) override;
    llvm::Value *visitBinaryExpr(const BinaryExpr &expr) override;
    llvm::Value *visitCallExpr(const CallExpr &expr) override;
    llvm::Value *visitGetExpr(const GetExpr &expr) override;
    llvm::Value *visitGroupingExpr(const GroupingExpr &expr) override;
    llvm::Value *visitLiteralExpr(const LiteralExpr &expr) override;
    llvm::Value *visitLogicalExpr(const LogicalExpr &expr) override;
    llvm::Value *visitSetExpr(const SetExpr &expr) override;
    llvm::Value *visitUnaryExpr(const UnaryExpr &expr) override;
    llvm::Value *visitVarExpr(const VarExpr &expr) override;
    llvm::Value *visitArrayExpr(const ArrayExpr &expr) override;
    llvm::Value *visitObjectExpr(const ObjectExpr &expr) override;
    llvm::Value *visitArrowFunctionExpr(const ArrowFunctionExpr &expr) override;

    // Visitor methods for statements
    void visitExpressionStmt(const ExpressionStmt &stmt) override;
    void visitPrintStmt(const PrintStmt &stmt) override;
    void visitVarStmt(const VarStmt &stmt) override;
    void visitBlockStmt(const BlockStmt &stmt) override;
    void visitIfStmt(const IfStmt &stmt) override;
    void visitWhileStmt(const WhileStmt &stmt) override;
    void visitForStmt(const ForStmt &stmt) override;
    void visitReturnStmt(const ReturnStmt &stmt) override;
    void visitFunctionStmt(const FunctionStmt &stmt) override;

private:
    // LLVM context, module, and builder
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    // Currently active function and return value
    llvm::Function *currentFunction;
    llvm::Value *returnValue;

    // JavaScript type mapping
    JSTypeMapping typeMapping;

    // Symbol table for variables
    std::unordered_map<std::string, llvm::Value *> namedValues;

    // Helper methods for initialization
    void initializeModule(const std::string &moduleName);
    void declareRuntimeFunctions();
    void createMainFunction();

    // Helper methods for code generation
    llvm::Value *createJSValueToDouble(llvm::Value *value);
    llvm::Value *createDoubleToJSValue(llvm::Value *value);
    llvm::Value *createJSValueToBoolean(llvm::Value *value);
    llvm::Value *createBooleanToJSValue(llvm::Value *value);

    // Helper methods for creating and converting JS values
    llvm::Value *createJSUndefined();
    llvm::Value *createJSNull();
    llvm::Value *createJSBoolean(bool value);
    llvm::Value *createJSNumber(double value);
    llvm::Value *createJSString(const std::string &value);

    // Helper methods for extracting JS values
    llvm::Value *extractJSTag(llvm::Value *value);
    llvm::Value *extractJSPayload(llvm::Value *value);
};

#endif // LLVM_BACKEND_H
