#ifndef JS_VALUE_TYPE_H
#define JS_VALUE_TYPE_H

#include <cstdint>

/**
 * JS_VALUE_TYPE_H
 *
 * This header defines the JavaScript value representation using NaN-boxing.
 *
 * NaN-boxing uses the fact that in IEEE-754 double precision floating point,
 * there are many bit patterns that represent NaN. These can be used to store
 * pointers and other non-double values.
 *
 * Specifically, any value with the 11-bit exponent field set to all 1s and
 * a non-zero fraction field is a NaN. We can use the highest bits of the
 * mantissa to encode a tag that identifies the type.
 *
 * Representation:
 * - Double values: Stored as-is
 * - Tag + payload: Stored with specific bit patterns in the exponent and mantissa
 */

/**
 * JavaScript value tags for NaN-boxing
 */
enum JSValueTag : uint8_t {
    JS_TAG_NUMBER = 0, // actual double value (not NaN-boxed)
    JS_TAG_UNDEFINED = 1,
    JS_TAG_NULL = 2,
    JS_TAG_BOOLEAN = 3,
    JS_TAG_STRING = 4,
    JS_TAG_OBJECT = 5,
    JS_TAG_FUNCTION = 6,
    JS_TAG_SYMBOL = 7,
    // Add more tags as needed
};

// NaN-boxing constants
constexpr uint64_t QUIET_NAN = 0x7FF8000000000000ULL; // Quiet NaN base value
constexpr uint64_t PAYLOAD_MASK = 0x0007FFFFFFFFFFFFULL; // Mask for the payload (48 bits)
constexpr uint64_t TAG_MASK = 0x0007000000000000ULL; // Mask for the tag (3 bits)
constexpr uint64_t TAG_SHIFT = 48; // Number of bits to shift the tag

/**
 * Create a NaN-boxed value with the given tag and payload
 */
inline uint64_t JS_MakeValue(JSValueTag tag, uint64_t payload) {
    return QUIET_NAN | ((static_cast<uint64_t>(tag) << TAG_SHIFT) & TAG_MASK) | (payload & PAYLOAD_MASK);
}

/**
 * Get the tag from a NaN-boxed value
 */
inline JSValueTag JS_GetTag(uint64_t value) {
    // Special case for actual double values (not NaN-boxed)
    // Check if the value has the NaN pattern in the exponent
    constexpr uint64_t EXPONENT_MASK = 0x7FF0000000000000ULL;
    constexpr uint64_t EXPONENT_ALL_ONES = 0x7FF0000000000000ULL;

    if ((value & EXPONENT_MASK) != EXPONENT_ALL_ONES || (value & 0x8000000000000ULL) == 0) {
        return JS_TAG_NUMBER; // It's an actual double
    }

    // Extract the tag from the NaN-boxed value
    return static_cast<JSValueTag>((value & TAG_MASK) >> TAG_SHIFT);
}

/**
 * Get the payload from a NaN-boxed value
 */
inline uint64_t JS_GetPayload(uint64_t value) { return value & PAYLOAD_MASK; }

/**
 * Create a NaN-boxed undefined value
 */
inline uint64_t JS_MakeUndefined() { return JS_MakeValue(JS_TAG_UNDEFINED, 0); }

/**
 * Create a NaN-boxed null value
 */
inline uint64_t JS_MakeNull() { return JS_MakeValue(JS_TAG_NULL, 0); }

/**
 * Create a NaN-boxed boolean value
 */
inline uint64_t JS_MakeBoolean(bool value) { return JS_MakeValue(JS_TAG_BOOLEAN, value ? 1 : 0); }

/**
 * Create a double value (not NaN-boxed)
 */
inline uint64_t JS_MakeDouble(double value) {
    union {
        double d;
        uint64_t u;
    } u;
    u.d = value;
    return u.u;
}

/**
 * Create a NaN-boxed string value
 */
inline uint64_t JS_MakeString(const char *ptr) { return JS_MakeValue(JS_TAG_STRING, reinterpret_cast<uint64_t>(ptr)); }

/**
 * Create a NaN-boxed object value
 */
inline uint64_t JS_MakeObject(void *ptr) { return JS_MakeValue(JS_TAG_OBJECT, reinterpret_cast<uint64_t>(ptr)); }

/**
 * Create a NaN-boxed function value
 */
inline uint64_t JS_MakeFunction(void *ptr) { return JS_MakeValue(JS_TAG_FUNCTION, reinterpret_cast<uint64_t>(ptr)); }

#endif // JS_VALUE_TYPE_H
