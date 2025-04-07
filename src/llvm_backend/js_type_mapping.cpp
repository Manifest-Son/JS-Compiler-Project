#include "../../include/llvm_backend/js_type_mapping.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>

JSTypeMapping::JSTypeMapping(llvm::LLVMContext &context) : context(context) {
    // Initialize basic types
    jsValueType = llvm::Type::getInt64Ty(context);
    doubleType = llvm::Type::getDoubleTy(context);
    stringPtrType = llvm::Type::getInt8PtrTy(context);

    // Create complex types
    // Object type with type tag and properties
    objectType = llvm::StructType::create(context, "js_object_t");

    // Array type with elements vector
    arrayType = llvm::StructType::create(context, "js_array_t");

    // Function type: jsValue(*)(int argc, jsValue* argv)
    std::vector<llvm::Type *> functionParams = {llvm::Type::getInt32Ty(context),
                                                llvm::PointerType::getUnqual(jsValueType)};
    functionType = llvm::FunctionType::get(jsValueType, functionParams, false);

    // Initialize constants for bit operations
    tagMask = llvm::ConstantInt::get(jsValueType, TAG_MASK);
    payloadMask = llvm::ConstantInt::get(jsValueType, PAYLOAD_MASK);
    quietNaN = llvm::ConstantInt::get(jsValueType, QUIET_NAN);

    // Initialize tag constants
    tagConstants[JS_TAG_NUMBER] = llvm::ConstantInt::get(jsValueType, JS_TAG_NUMBER << TAG_SHIFT);
    tagConstants[JS_TAG_UNDEFINED] = llvm::ConstantInt::get(jsValueType, JS_TAG_UNDEFINED << TAG_SHIFT);
    tagConstants[JS_TAG_NULL] = llvm::ConstantInt::get(jsValueType, JS_TAG_NULL << TAG_SHIFT);
    tagConstants[JS_TAG_BOOLEAN] = llvm::ConstantInt::get(jsValueType, JS_TAG_BOOLEAN << TAG_SHIFT);
    tagConstants[JS_TAG_STRING] = llvm::ConstantInt::get(jsValueType, JS_TAG_STRING << TAG_SHIFT);
    tagConstants[JS_TAG_OBJECT] = llvm::ConstantInt::get(jsValueType, JS_TAG_OBJECT << TAG_SHIFT);
    tagConstants[JS_TAG_FUNCTION] = llvm::ConstantInt::get(jsValueType, JS_TAG_FUNCTION << TAG_SHIFT);
}

llvm::Type *JSTypeMapping::getLLVMType(Expression::Type type) const {
    // In our NaN-boxing scheme, all JS values are represented as i64
    return jsValueType;
}

llvm::Type *JSTypeMapping::getJSValueType() const { return jsValueType; }

llvm::StructType *JSTypeMapping::getObjectType() const { return objectType; }

llvm::StructType *JSTypeMapping::getArrayType() const { return arrayType; }

llvm::FunctionType *JSTypeMapping::getFunctionType() const { return functionType; }

llvm::PointerType *JSTypeMapping::getStringPtrType() const { return stringPtrType; }

llvm::Value *JSTypeMapping::isNumber(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    // Extract the tag from the value
    llvm::Value *tag = builder.CreateAnd(value, tagMask, "tag");

    // Check if it's a number (either a double value or has a number tag)
    // For doubles, we need to check the exponent bits pattern
    llvm::Value *exponentMask = llvm::ConstantInt::get(jsValueType, 0x7FF0000000000000ULL);
    llvm::Value *exponentBits = builder.CreateAnd(value, exponentMask, "exponent_bits");
    llvm::Value *isNotNaN = builder.CreateICmpNE(exponentBits, exponentMask, "is_not_nan");

    // Either it's not a NaN or it has the number tag
    llvm::Value *hasNumberTag = builder.CreateICmpEQ(tag, tagConstants[JS_TAG_NUMBER], "has_number_tag");
    return builder.CreateOr(isNotNaN, hasNumberTag, "is_number");
}

llvm::Value *JSTypeMapping::isString(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    // Extract the tag from the value
    llvm::Value *tag = builder.CreateAnd(value, tagMask, "tag");

    // Check if it has a string tag
    return builder.CreateICmpEQ(tag, tagConstants[JS_TAG_STRING], "is_string");
}

llvm::Value *JSTypeMapping::isBoolean(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    // Extract the tag from the value
    llvm::Value *tag = builder.CreateAnd(value, tagMask, "tag");

    // Check if it has a boolean tag
    return builder.CreateICmpEQ(tag, tagConstants[JS_TAG_BOOLEAN], "is_boolean");
}

llvm::Value *JSTypeMapping::isObject(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    // Extract the tag from the value
    llvm::Value *tag = builder.CreateAnd(value, tagMask, "tag");

    // Check if it has an object tag
    return builder.CreateICmpEQ(tag, tagConstants[JS_TAG_OBJECT], "is_object");
}

llvm::Value *JSTypeMapping::isUndefined(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    // Extract the tag from the value
    llvm::Value *tag = builder.CreateAnd(value, tagMask, "tag");

    // Check if it has an undefined tag
    return builder.CreateICmpEQ(tag, tagConstants[JS_TAG_UNDEFINED], "is_undefined");
}

llvm::Value *JSTypeMapping::isNull(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    // Extract the tag from the value
    llvm::Value *tag = builder.CreateAnd(value, tagMask, "tag");

    // Check if it has a null tag
    return builder.CreateICmpEQ(tag, tagConstants[JS_TAG_NULL], "is_null");
}
