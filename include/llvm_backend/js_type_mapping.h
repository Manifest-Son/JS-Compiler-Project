#ifndef JS_TYPE_MAPPING_H
#define JS_TYPE_MAPPING_H

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <memory>
#include <unordered_map>
#include "../ast.h"
#include "js_value_type.h"

/**
 * JSTypeMapping - Maps JavaScript AST types to LLVM IR types
 *
 * This class is responsible for translating JavaScript types in the AST
 * into corresponding LLVM types. All JavaScript values are represented as
 * 64-bit integers (pointers) following the NaN-boxing scheme.
 */
class JSTypeMapping {
public:
    /**
     * Constructor initializes types based on the LLVM context
     * @param context The LLVM context to use for creating types
     */
    explicit JSTypeMapping(llvm::LLVMContext &context);

    /**
     * Get the LLVM type for a JavaScript value
     * @param type The JavaScript expression type from the AST
     * @return The corresponding LLVM type
     */
    llvm::Type *getLLVMType(Expression::Type type) const;

    /**
     * Get the LLVM type for a JavaScript value (always i64 for NaN-boxing)
     * @return The LLVM type for JavaScript values
     */
    llvm::Type *getJSValueType() const;

    /**
     * Get the LLVM struct type for JavaScript objects
     * @return The LLVM struct type for JavaScript objects
     */
    llvm::StructType *getObjectType() const;

    /**
     * Get the LLVM struct type for JavaScript arrays
     * @return The LLVM struct type for JavaScript arrays
     */
    llvm::StructType *getArrayType() const;

    /**
     * Get the LLVM type for JavaScript function pointers
     * @return The LLVM function pointer type
     */
    llvm::FunctionType *getFunctionType() const;

    /**
     * Get the LLVM pointer type for JavaScript strings
     * @return The LLVM pointer type for JavaScript strings
     */
    llvm::PointerType *getStringPtrType() const;

    /**
     * Check if a JavaScript value is a number
     * @param value The JavaScript value (as LLVM Value*)
     * @param builder The IRBuilder to use for creating instructions
     * @return LLVM Value* representing a boolean result
     */
    llvm::Value *isNumber(llvm::Value *value, llvm::IRBuilder<> &builder) const;

    /**
     * Check if a JavaScript value is a string
     * @param value The JavaScript value (as LLVM Value*)
     * @param builder The IRBuilder to use for creating instructions
     * @return LLVM Value* representing a boolean result
     */
    llvm::Value *isString(llvm::Value *value, llvm::IRBuilder<> &builder) const;

    /**
     * Check if a JavaScript value is a boolean
     * @param value The JavaScript value (as LLVM Value*)
     * @param builder The IRBuilder to use for creating instructions
     * @return LLVM Value* representing a boolean result
     */
    llvm::Value *isBoolean(llvm::Value *value, llvm::IRBuilder<> &builder) const;

    /**
     * Check if a JavaScript value is an object
     * @param value The JavaScript value (as LLVM Value*)
     * @param builder The IRBuilder to use for creating instructions
     * @return LLVM Value* representing a boolean result
     */
    llvm::Value *isObject(llvm::Value *value, llvm::IRBuilder<> &builder) const;

    /**
     * Check if a JavaScript value is undefined
     * @param value The JavaScript value (as LLVM Value*)
     * @param builder The IRBuilder to use for creating instructions
     * @return LLVM Value* representing a boolean result
     */
    llvm::Value *isUndefined(llvm::Value *value, llvm::IRBuilder<> &builder) const;

    /**
     * Check if a JavaScript value is null
     * @param value The JavaScript value (as LLVM Value*)
     * @param builder The IRBuilder to use for creating instructions
     * @return LLVM Value* representing a boolean result
     */
    llvm::Value *isNull(llvm::Value *value, llvm::IRBuilder<> &builder) const;

private:
    llvm::LLVMContext &context;

    // Basic types
    llvm::Type *jsValueType; // 64-bit integer for NaN-boxed values
    llvm::Type *doubleType; // double for numbers
    llvm::PointerType *stringPtrType; // char* for strings

    // Complex types
    llvm::StructType *objectType; // Struct for JS objects
    llvm::StructType *arrayType; // Struct for JS arrays
    llvm::FunctionType *functionType; // Type for JS functions

    // Type constants for bit operations
    llvm::Constant *tagMask; // Mask for extracting tag bits
    llvm::Constant *payloadMask; // Mask for extracting payload bits
    llvm::Constant *quietNaN; // Base value for NaN-boxing

    // Tag constants
    std::unordered_map<JSValueTag, llvm::Constant *> tagConstants;
};

#endif // JS_TYPE_MAPPING_H
