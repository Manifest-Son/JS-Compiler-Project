#ifndef JS_VALUE_TYPE_H
#define JS_VALUE_TYPE_H

#include <cstdint>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

/**
 * JavaScript value representation using NaN-boxing
 *
 * NaN-boxing takes advantage of the IEEE-754 representation of doubles:
 * - Normal double values use all bits as defined by IEEE-754
 * - NaN values have all exponent bits set (0x7FF) and a non-zero fraction
 * - We use NaNs to encode other JS types by setting specific patterns in the bits
 *
 * 64-bit layout:
 * [63]    [62-52]    [51-48]    [47-0]
 *  Sign   Exponent   Tag        Payload
 *
 * For non-number values, the high 16 bits encode the type tag.
 */

// NaN-boxing constants and utilities
namespace js_value {
    // Constants for NaN-boxing
    constexpr uint64_t QUIET_NAN = 0x7FF8000000000000ULL;
    constexpr uint64_t PAYLOAD_MASK = 0x0000FFFFFFFFFFFFULL;
    constexpr uint64_t TAG_MASK = 0xFFFF000000000000ULL;
    constexpr int TAG_SHIFT = 48;

    // JavaScript value tags
    enum JSValueTag {
        JS_TAG_NUMBER = 0, // Regular double, not NaN-boxed
        JS_TAG_UNDEFINED = 1,
        JS_TAG_NULL = 2,
        JS_TAG_BOOLEAN = 3,
        JS_TAG_STRING = 4, // Pointer to string
        JS_TAG_OBJECT = 5, // Pointer to object
        JS_TAG_FUNCTION = 6 // Pointer to function
    };

    // Create a boxed JS value from a tag and payload
    inline uint64_t makeBoxedValue(JSValueTag tag, uint64_t payload) {
        return QUIET_NAN | ((static_cast<uint64_t>(tag) << TAG_SHIFT) & TAG_MASK) | (payload & PAYLOAD_MASK);
    }

    // Extract the tag from a boxed value
    inline JSValueTag getTag(uint64_t value) {
        // Check if it's a regular double (not NaN)
        if ((value & TAG_MASK) != QUIET_NAN) {
            return JS_TAG_NUMBER;
        }
        return static_cast<JSValueTag>((value >> TAG_SHIFT) & 0xF);
    }

    // Extract the payload from a boxed value
    inline uint64_t getPayload(uint64_t value) { return value & PAYLOAD_MASK; }

    // Create special JS values
    inline uint64_t makeUndefined() { return makeBoxedValue(JS_TAG_UNDEFINED, 0); }

    inline uint64_t makeNull() { return makeBoxedValue(JS_TAG_NULL, 0); }

    inline uint64_t makeBoolean(bool value) { return makeBoxedValue(JS_TAG_BOOLEAN, value ? 1 : 0); }

    // For objects, strings, and functions, the payload is a pointer
    inline uint64_t makeObject(void *ptr) { return makeBoxedValue(JS_TAG_OBJECT, reinterpret_cast<uint64_t>(ptr)); }

    inline uint64_t makeString(void *ptr) { return makeBoxedValue(JS_TAG_STRING, reinterpret_cast<uint64_t>(ptr)); }

    inline uint64_t makeFunction(void *ptr) { return makeBoxedValue(JS_TAG_FUNCTION, reinterpret_cast<uint64_t>(ptr)); }

    // Helper functions for LLVM IR generation
    inline llvm::Type *getJSValueType(llvm::LLVMContext &context) { return llvm::Type::getInt64Ty(context); }

    inline llvm::Constant *getUndefined(llvm::LLVMContext &context) {
        return llvm::ConstantInt::get(getJSValueType(context), makeUndefined());
    }

    inline llvm::Constant *getNull(llvm::LLVMContext &context) {
        return llvm::ConstantInt::get(getJSValueType(context), makeNull());
    }

    inline llvm::Constant *getBoolean(llvm::LLVMContext &context, bool value) {
        return llvm::ConstantInt::get(getJSValueType(context), makeBoolean(value));
    }
} // namespace js_value

#endif // JS_VALUE_TYPE_H
