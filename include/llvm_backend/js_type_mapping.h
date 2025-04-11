#ifndef JS_TYPE_MAPPING_H
#define JS_TYPE_MAPPING_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>
#include <unordered_map>
#include "js_value_type.h"

/**
 * Expression::Type from AST (assuming it exists in your code)
 * If not, you might need to adjust this enum.
 */
namespace Expression {
    enum class Type { Number, Boolean, String, Object, Undefined, Null, Function, Any };
}

/**
 * JSTypeMapping - Maps JavaScript types to LLVM types
 *
 * This class provides utilities for mapping between JavaScript's dynamic
 * types and LLVM's static type system using NaN-boxing.
 */
class JSTypeMapping {
public:
    /**
     * Constructor initializes type mapping
     * @param context LLVM context for type creation
     */
    explicit JSTypeMapping(llvm::LLVMContext &context);

    /**
     * Get LLVM type for a specific JavaScript type
     * @param type JavaScript type from AST
     * @return Corresponding LLVM type
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
     * @return LLVM Value representing boolean result (i1)
     */
    llvm::Value *isNumber(llvm::Value *value, llvm::IRBuilder<> &builder) const;

    /**
     * Check if a JavaScript value is a string
     * @param value The JavaScript value (as LLVM Value*)
     * @param builder The IRBuilder to use for creating instructions
     * @return LLVM Value representing boolean result (i1)
     */
    llvm::Value *isString(llvm::Value *value, llvm::IRBuilder<> &builder) const;

    /**
     * Check if a JavaScript value is an object
     * @param value The JavaScript value (as LLVM Value*)
     * @param builder The IRBuilder to use for creating instructions
     * @return LLVM Value representing boolean result (i1)
     */
    llvm::Value *isObject(llvm::Value *value, llvm::IRBuilder<> &builder) const;

    /**
     * Check if a JavaScript value is a function
     * @param value The JavaScript value (as LLVM Value*)
     * @param builder The IRBuilder to use for creating instructions
     * @return LLVM Value representing boolean result (i1)
     */
    llvm::Value *isFunction(llvm::Value *value, llvm::IRBuilder<> &builder) const;

    /**
     * Check if a JavaScript value is a boolean
     * @param value The JavaScript value (as LLVM Value*)
     * @param builder The IRBuilder to use for creating instructions
     * @return LLVM Value representing boolean result (i1)
     */
    llvm::Value *isBoolean(llvm::Value *value, llvm::IRBuilder<> &builder) const;

    /**
     * Check if a JavaScript value is null
     * @param value The JavaScript value (as LLVM Value*)
     * @param builder The IRBuilder to use for creating instructions
     * @return LLVM Value representing boolean result (i1)
     */
    llvm::Value *isNull(llvm::Value *value, llvm::IRBuilder<> &builder) const;

    /**
     * Check if a JavaScript value is undefined
     * @param value The JavaScript value (as LLVM Value*)
     * @param builder The IRBuilder to use for creating instructions
     * @return LLVM Value representing boolean result (i1)
     */
    llvm::Value *isUndefined(llvm::Value *value, llvm::IRBuilder<> &builder) const;

private:
    llvm::LLVMContext &context;
    llvm::Type *jsValueType;
    llvm::StructType *objectType;
    llvm::StructType *arrayType;
    llvm::FunctionType *functionType;
    llvm::PointerType *stringPtrType;

    // Helper methods for type checking
    llvm::Value *checkTag(llvm::Value *value, js_value::JSValueTag tag, llvm::IRBuilder<> &builder) const;
};

#endif // JS_TYPE_MAPPING_H
