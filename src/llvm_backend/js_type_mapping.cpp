#include "../../include/llvm_backend/js_type_mapping.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>

JSTypeMapping::JSTypeMapping(llvm::LLVMContext &context) : context(context) {
    // Initialize basic types
    jsValueType = llvm::Type::getInt64Ty(context);
    stringPtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);

    // Create object type (simplified version)
    objectType = llvm::StructType::create(context, "JSObject");
    objectType->setBody({
            llvm::Type::getInt32Ty(context), // Type tag
            llvm::PointerType::get(jsValueType, 0) // Pointer to properties array
    });

    // Create array type
    arrayType = llvm::StructType::create(context, "JSArray");
    arrayType->setBody({
            llvm::Type::getInt32Ty(context), // Length
            llvm::PointerType::get(jsValueType, 0) // Pointer to elements
    });

    // Create function type (for JS functions)
    functionType = llvm::FunctionType::get(jsValueType, // Return type is a JS value
                                           {
                                                   llvm::PointerType::get(jsValueType, 0), // this pointer
                                                   llvm::PointerType::get(jsValueType, 0) // arguments array
                                           },
                                           false // Not varargs
    );
}

llvm::Type *JSTypeMapping::getLLVMType(Expression::Type type) const {
    switch (type) {
        case Expression::Type::Number:
        case Expression::Type::Boolean:
        case Expression::Type::String:
        case Expression::Type::Object:
        case Expression::Type::Undefined:
        case Expression::Type::Null:
        case Expression::Type::Function:
        case Expression::Type::Any:
            // All JavaScript values use the same type with NaN-boxing
            return jsValueType;
    }

    // Default case, should never happen
    return jsValueType;
}

llvm::Type *JSTypeMapping::getJSValueType() const { return jsValueType; }

llvm::StructType *JSTypeMapping::getObjectType() const { return objectType; }

llvm::StructType *JSTypeMapping::getArrayType() const { return arrayType; }

llvm::FunctionType *JSTypeMapping::getFunctionType() const { return functionType; }

llvm::PointerType *JSTypeMapping::getStringPtrType() const { return stringPtrType; }

llvm::Value *JSTypeMapping::checkTag(llvm::Value *value, js_value::JSValueTag tag, llvm::IRBuilder<> &builder) const {
    // Constants for checking
    auto tagMask = llvm::ConstantInt::get(jsValueType, js_value::TAG_MASK);
    auto tagValue = llvm::ConstantInt::get(jsValueType, static_cast<uint64_t>(tag) << js_value::TAG_SHIFT);
    auto quietNaN = llvm::ConstantInt::get(jsValueType, js_value::QUIET_NAN);

    // For JS_TAG_NUMBER, we need to check if it's NOT a NaN-boxed value
    if (tag == js_value::JS_TAG_NUMBER) {
        // Extract the tag bits: value & TAG_MASK
        auto extractedTag = builder.CreateAnd(value, tagMask);
        // Check that it's not QUIET_NAN (meaning it's a regular double)
        return builder.CreateICmpNE(extractedTag, quietNaN);
    } else {
        // Extract the tag bits: (value & TAG_MASK) == (tag << TAG_SHIFT)
        auto extractedTag = builder.CreateAnd(value, tagMask);
        return builder.CreateICmpEQ(extractedTag, tagValue);
    }
}

llvm::Value *JSTypeMapping::isNumber(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    return checkTag(value, js_value::JS_TAG_NUMBER, builder);
}

llvm::Value *JSTypeMapping::isString(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    return checkTag(value, js_value::JS_TAG_STRING, builder);
}

llvm::Value *JSTypeMapping::isObject(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    return checkTag(value, js_value::JS_TAG_OBJECT, builder);
}

llvm::Value *JSTypeMapping::isFunction(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    return checkTag(value, js_value::JS_TAG_FUNCTION, builder);
}

llvm::Value *JSTypeMapping::isBoolean(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    return checkTag(value, js_value::JS_TAG_BOOLEAN, builder);
}

llvm::Value *JSTypeMapping::isNull(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    return checkTag(value, js_value::JS_TAG_NULL, builder);
}

llvm::Value *JSTypeMapping::isUndefined(llvm::Value *value, llvm::IRBuilder<> &builder) const {
    return checkTag(value, js_value::JS_TAG_UNDEFINED, builder);
}
