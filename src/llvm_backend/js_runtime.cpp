#include "../../include/llvm_backend/js_runtime.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <llvm/IR/DerivedTypes.h>
#include <string>
#include <unordered_map>
#include <vector>

// String cache for string interning
static std::unordered_map<std::string, char *> stringCache;

// Object structure
struct JSObject {
    enum class Type { Object, Array, Function };

    Type type;
    std::unordered_map<std::string, uint64_t> properties;
    std::vector<uint64_t> elements; // For arrays
    void *functionPtr; // For functions

    JSObject(Type t = Type::Object) : type(t), functionPtr(nullptr) {}
    ~JSObject() {
        properties.clear();
        elements.clear();
    }
};

// Clean up cached strings
void cleanupStringCache() {
    for (auto &pair: stringCache) {
        free(pair.second);
    }
    stringCache.clear();
}

// Type operations
extern "C" int32_t js_get_type(uint64_t value) { return static_cast<int32_t>(JS_GetTag(value)); }

extern "C" uint64_t js_typeof(uint64_t value) {
    const char *typeStr;

    switch (JS_GetTag(value)) {
        case JS_TAG_NUMBER:
            typeStr = "number";
            break;
        case JS_TAG_STRING:
            typeStr = "string";
            break;
        case JS_TAG_BOOLEAN:
            typeStr = "boolean";
            break;
        case JS_TAG_FUNCTION:
            typeStr = "function";
            break;
        case JS_TAG_OBJECT:
            if (value == JS_MakeNull()) {
                typeStr = "object"; // null is considered an object in JavaScript
            } else {
                JSObject *obj = reinterpret_cast<JSObject *>(JS_GetPayload(value));
                typeStr = (obj && obj->type == JSObject::Type::Array) ? "object" : "object";
            }
            break;
        case JS_TAG_UNDEFINED:
            typeStr = "undefined";
            break;
        default:
            typeStr = "object";
    }

    return js_make_string(typeStr);
}

extern "C" double js_value_to_number(uint64_t value) {
    // If it's already a number, extract it
    if (JS_GetTag(value) == JS_TAG_NUMBER) {
        union {
            uint64_t u;
            double d;
        } u;
        u.u = value;
        return u.d;
    }

    // Convert other types to number according to JavaScript rules
    switch (JS_GetTag(value)) {
        case JS_TAG_BOOLEAN:
            return JS_GetPayload(value) ? 1.0 : 0.0;

        case JS_TAG_STRING: {
            const char *str = reinterpret_cast<const char *>(JS_GetPayload(value));
            if (!str || !*str)
                return 0.0; // Empty string is 0

            char *endptr;
            double result = strtod(str, &endptr);
            if (*endptr != '\0') {
                // Not a valid number string
                return std::numeric_limits<double>::quiet_NaN();
            }
            return result;
        }

        case JS_TAG_UNDEFINED:
            return std::numeric_limits<double>::quiet_NaN();

        case JS_TAG_NULL:
            return 0.0;

        default:
            return std::numeric_limits<double>::quiet_NaN();
    }
}

extern "C" bool js_value_to_boolean(uint64_t value) {
    switch (JS_GetTag(value)) {
        case JS_TAG_NUMBER: {
            union {
                uint64_t u;
                double d;
            } u;
            u.u = value;
            // Number is false if it's 0 or NaN
            return u.d != 0.0 && !std::isnan(u.d);
        }

        case JS_TAG_BOOLEAN:
            return JS_GetPayload(value) != 0;

        case JS_TAG_STRING: {
            const char *str = reinterpret_cast<const char *>(JS_GetPayload(value));
            // String is false only if it's empty
            return str && *str;
        }

        case JS_TAG_UNDEFINED:
        case JS_TAG_NULL:
            return false;

        default:
            // Objects are always truthy
            return true;
    }
}

extern "C" const char *js_value_to_string(uint64_t value) {
    std::string result;

    switch (JS_GetTag(value)) {
        case JS_TAG_NUMBER: {
            union {
                uint64_t u;
                double d;
            } u;
            u.u = value;

            if (std::isnan(u.d)) {
                result = "NaN";
            } else if (std::isinf(u.d)) {
                result = u.d > 0 ? "Infinity" : "-Infinity";
            } else {
                // Convert number to string
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%.14g", u.d);
                result = buffer;
            }
            break;
        }

        case JS_TAG_STRING:
            return reinterpret_cast<const char *>(JS_GetPayload(value));

        case JS_TAG_BOOLEAN:
            result = JS_GetPayload(value) ? "true" : "false";
            break;

        case JS_TAG_UNDEFINED:
            result = "undefined";
            break;

        case JS_TAG_NULL:
            result = "null";
            break;

        default:
            result = "[object Object]";
    }

    // Allocate memory for the string
    char *str = strdup(result.c_str());

    // Return the allocated string
    return str;
}

// String operations
extern "C" uint64_t js_make_string(const char *str) {
    if (!str)
        return JS_MakeNull();

    // Check if this string is already interned
    std::string key(str);
    auto it = stringCache.find(key);

    if (it != stringCache.end()) {
        return JS_MakeString(it->second);
    }

    // Allocate memory for the string and copy it
    char *newStr = strdup(str);
    stringCache[key] = newStr;

    return JS_MakeString(newStr);
}

extern "C" const char *js_get_string_ptr(uint64_t value) {
    if (JS_GetTag(value) != JS_TAG_STRING) {
        return "";
    }
    return reinterpret_cast<const char *>(JS_GetPayload(value));
}

extern "C" uint64_t js_concat_strings(uint64_t a, uint64_t b) {
    const char *str1 = js_value_to_string(a);
    const char *str2 = js_value_to_string(b);

    std::string result(str1);
    result += str2;

    return js_make_string(result.c_str());
}

// Object operations
extern "C" uint64_t js_create_object() {
    JSObject *obj = new JSObject(JSObject::Type::Object);
    return JS_MakeObject(obj);
}

extern "C" uint64_t js_create_array() {
    JSObject *obj = new JSObject(JSObject::Type::Array);
    return JS_MakeObject(obj);
}

extern "C" void js_set_property(uint64_t obj, const char *key, uint64_t value) {
    if (JS_GetTag(obj) != JS_TAG_OBJECT)
        return;

    JSObject *object = reinterpret_cast<JSObject *>(JS_GetPayload(obj));
    if (object) {
        // Store property
        object->properties[key] = value;

        // Special handling for arrays with numeric indices
        if (object->type == JSObject::Type::Array) {
            char *endptr;
            long index = strtol(key, &endptr, 10);
            if (*key != '\0' && *endptr == '\0' && index >= 0) {
                // Valid numeric index
                if (static_cast<size_t>(index) >= object->elements.size()) {
                    object->elements.resize(index + 1, JS_MakeUndefined());
                }
                object->elements[index] = value;
            }
        }
    }
}

extern "C" uint64_t js_get_property(uint64_t obj, const char *key) {
    if (JS_GetTag(obj) != JS_TAG_OBJECT) {
        return JS_MakeUndefined();
    }

    JSObject *object = reinterpret_cast<JSObject *>(JS_GetPayload(obj));
    if (object) {
        auto it = object->properties.find(key);
        if (it != object->properties.end()) {
            return it->second;
        }
    }

    return JS_MakeUndefined();
}

extern "C" bool js_has_property(uint64_t obj, const char *key) {
    if (JS_GetTag(obj) != JS_TAG_OBJECT)
        return false;

    JSObject *object = reinterpret_cast<JSObject *>(JS_GetPayload(obj));
    if (object) {
        return object->properties.find(key) != object->properties.end();
    }
    return false;
}

extern "C" void js_delete_property(uint64_t obj, const char *key) {
    if (JS_GetTag(obj) != JS_TAG_OBJECT)
        return;

    JSObject *object = reinterpret_cast<JSObject *>(JS_GetPayload(obj));
    if (object) {
        object->properties.erase(key);
    }
}

// Array operations
extern "C" uint64_t js_array_get(uint64_t array, int32_t index) {
    if (JS_GetTag(array) != JS_TAG_OBJECT)
        return JS_MakeUndefined();

    JSObject *obj = reinterpret_cast<JSObject *>(JS_GetPayload(array));
    if (obj && obj->type == JSObject::Type::Array) {
        if (index >= 0 && static_cast<size_t>(index) < obj->elements.size()) {
            return obj->elements[index];
        }
    }

    return JS_MakeUndefined();
}

extern "C" void js_array_set(uint64_t array, int32_t index, uint64_t value) {
    if (JS_GetTag(array) != JS_TAG_OBJECT)
        return;

    JSObject *obj = reinterpret_cast<JSObject *>(JS_GetPayload(array));
    if (obj && obj->type == JSObject::Type::Array && index >= 0) {
        if (static_cast<size_t>(index) >= obj->elements.size()) {
            obj->elements.resize(index + 1, JS_MakeUndefined());

            // Update length property
            char lenStr[16];
            snprintf(lenStr, sizeof(lenStr), "%d", index + 1);
            obj->properties["length"] = js_make_string(lenStr);
        }

        obj->elements[index] = value;
    }
}

extern "C" int32_t js_array_length(uint64_t array) {
    if (JS_GetTag(array) != JS_TAG_OBJECT)
        return 0;

    JSObject *obj = reinterpret_cast<JSObject *>(JS_GetPayload(array));
    if (obj && obj->type == JSObject::Type::Array) {
        return static_cast<int32_t>(obj->elements.size());
    }

    return 0;
}

extern "C" uint64_t js_create_array_with_elements(int32_t count, uint64_t *elements) {
    JSObject *obj = new JSObject(JSObject::Type::Array);

    if (count > 0 && elements) {
        obj->elements.reserve(count);
        for (int32_t i = 0; i < count; i++) {
            obj->elements.push_back(elements[i]);
        }
    }

    // Set length property
    char lenStr[16];
    snprintf(lenStr, sizeof(lenStr), "%d", count);
    obj->properties["length"] = js_make_string(lenStr);

    return JS_MakeObject(obj);
}

// Function operations
extern "C" uint64_t js_create_closure(void *function_ptr) {
    if (!function_ptr)
        return JS_MakeUndefined();

    JSObject *obj = new JSObject(JSObject::Type::Function);
    obj->functionPtr = function_ptr;

    return JS_MakeFunction(obj);
}

// Array of JS values for arguments
static thread_local std::vector<uint64_t> callArguments;

extern "C" uint64_t js_call_function(uint64_t function, int32_t argc, uint64_t *argv) {
    if (JS_GetTag(function) != JS_TAG_FUNCTION && JS_GetTag(function) != JS_TAG_OBJECT) {
        return JS_MakeUndefined();
    }

    // Get the function object
    JSObject *obj = reinterpret_cast<JSObject *>(JS_GetPayload(function));
    if (!obj || obj->type != JSObject::Type::Function || !obj->functionPtr) {
        return JS_MakeUndefined();
    }

    // This is a simplified way to call a function
    // In a real implementation, this would involve more complex handling
    typedef uint64_t (*JSFunction)(int32_t, uint64_t *);
    JSFunction jsFunc = reinterpret_cast<JSFunction>(obj->functionPtr);

    // Call the function with the arguments
    return jsFunc(argc, argv);
}

// Arithmetic operations
extern "C" uint64_t js_add(uint64_t a, uint64_t b) {
    // If either operand is a string, do string concatenation
    if (JS_GetTag(a) == JS_TAG_STRING || JS_GetTag(b) == JS_TAG_STRING) {
        return js_concat_strings(a, b);
    }

    // Otherwise, convert both to numbers and add
    double numA = js_value_to_number(a);
    double numB = js_value_to_number(b);
    return JS_MakeDouble(numA + numB);
}

extern "C" uint64_t js_subtract(uint64_t a, uint64_t b) {
    double numA = js_value_to_number(a);
    double numB = js_value_to_number(b);
    return JS_MakeDouble(numA - numB);
}

extern "C" uint64_t js_multiply(uint64_t a, uint64_t b) {
    double numA = js_value_to_number(a);
    double numB = js_value_to_number(b);
    return JS_MakeDouble(numA * numB);
}

extern "C" uint64_t js_divide(uint64_t a, uint64_t b) {
    double numA = js_value_to_number(a);
    double numB = js_value_to_number(b);

    if (numB == 0.0) {
        // Division by zero results in Infinity or -Infinity
        return JS_MakeDouble(numA < 0 ? -INFINITY : INFINITY);
    }

    return JS_MakeDouble(numA / numB);
}

// Comparison operations
extern "C" uint64_t js_equal(uint64_t a, uint64_t b) {
    // If types are the same, use strict equality
    if (JS_GetTag(a) == JS_GetTag(b)) {
        return js_strict_equal(a, b);
    }

    // If one is null and the other is undefined, they are equal
    if ((JS_GetTag(a) == JS_TAG_NULL && JS_GetTag(b) == JS_TAG_UNDEFINED) ||
        (JS_GetTag(a) == JS_TAG_UNDEFINED && JS_GetTag(b) == JS_TAG_NULL)) {
        return JS_MakeBoolean(true);
    }

    // If one is a number and the other is a string, convert string to number
    if (JS_GetTag(a) == JS_TAG_NUMBER && JS_GetTag(b) == JS_TAG_STRING) {
        return js_equal(a, JS_MakeDouble(js_value_to_number(b)));
    }

    if (JS_GetTag(a) == JS_TAG_STRING && JS_GetTag(b) == JS_TAG_NUMBER) {
        return js_equal(JS_MakeDouble(js_value_to_number(a)), b);
    }

    // If one is a boolean, convert it to a number
    if (JS_GetTag(a) == JS_TAG_BOOLEAN) {
        return js_equal(JS_MakeDouble(js_value_to_boolean(a) ? 1 : 0), b);
    }

    if (JS_GetTag(b) == JS_TAG_BOOLEAN) {
        return js_equal(a, JS_MakeDouble(js_value_to_boolean(b) ? 1 : 0));
    }

    // If comparing object to primitive, convert object to primitive
    // This is simplified; a real implementation would involve ToPrimitive operations

    return JS_MakeBoolean(false);
}

extern "C" uint64_t js_strict_equal(uint64_t a, uint64_t b) {
    // Different types are never strictly equal
    if (JS_GetTag(a) != JS_GetTag(b)) {
        return JS_MakeBoolean(false);
    }

    switch (JS_GetTag(a)) {
        case JS_TAG_UNDEFINED:
        case JS_TAG_NULL:
            return JS_MakeBoolean(true); // All undefined/null values are equal

        case JS_TAG_NUMBER: {
            union {
                uint64_t u;
                double d;
            } ua, ub;
            ua.u = a;
            ub.u = b;
            // NaN is not equal to anything, including NaN
            if (std::isnan(ua.d) || std::isnan(ub.d)) {
                return JS_MakeBoolean(false);
            }
            return JS_MakeBoolean(ua.d == ub.d);
        }

        case JS_TAG_STRING: {
            const char *strA = reinterpret_cast<const char *>(JS_GetPayload(a));
            const char *strB = reinterpret_cast<const char *>(JS_GetPayload(b));
            return JS_MakeBoolean(strcmp(strA, strB) == 0);
        }

        case JS_TAG_BOOLEAN:
            return JS_MakeBoolean(JS_GetPayload(a) == JS_GetPayload(b));

        case JS_TAG_OBJECT:
        case JS_TAG_FUNCTION:
            // For objects and functions, compare pointers (reference equality)
            return JS_MakeBoolean(JS_GetPayload(a) == JS_GetPayload(b));

        default:
            return JS_MakeBoolean(false);
    }
}

extern "C" uint64_t js_less_than(uint64_t a, uint64_t b) {
    double numA = js_value_to_number(a);
    double numB = js_value_to_number(b);

    // Handle NaN comparisons
    if (std::isnan(numA) || std::isnan(numB)) {
        return JS_MakeBoolean(false);
    }

    return JS_MakeBoolean(numA < numB);
}

extern "C" uint64_t js_greater_than(uint64_t a, uint64_t b) {
    double numA = js_value_to_number(a);
    double numB = js_value_to_number(b);

    // Handle NaN comparisons
    if (std::isnan(numA) || std::isnan(numB)) {
        return JS_MakeBoolean(false);
    }

    return JS_MakeBoolean(numA > numB);
}

// I/O operations
extern "C" void js_print(uint64_t value) {
    const char *str = js_value_to_string(value);
    std::cout << str << std::endl;

    // If the string was dynamically allocated by js_value_to_string
    if (JS_GetTag(value) != JS_TAG_STRING) {
        free(const_cast<char *>(str));
    }
}

extern "C" uint64_t js_read_line() {
    std::string line;
    std::getline(std::cin, line);
    return js_make_string(line.c_str());
}

// Memory management functions
extern "C" void *js_alloc(uint64_t size) { return malloc(size); }

extern "C" void js_free(void *ptr) { free(ptr); }

// JSRuntime implementation
JSRuntime::JSRuntime(llvm::LLVMContext &context, llvm::Module *module) : context(context), module(module) {
    // Initialize the runtime - empty to begin with
}

llvm::Function *JSRuntime::getFunction(const std::string &name) const {
    auto it = functions.find(name);
    if (it != functions.end()) {
        return it->second;
    }
    return nullptr;
}

void JSRuntime::declareAll() {
    declareTypeOperations();
    declareObjectOperations();
    declareStringOperations();
    declareArrayOperations();
    declareArithmeticOperations();
    declareComparisonOperations();
    declareControlFlowOperations();
}

const std::unordered_map<std::string, llvm::Function *> &JSRuntime::getAllFunctions() const { return functions; }

llvm::Function *JSRuntime::declareFunction(const std::string &name, llvm::Type *returnType,
                                           const std::vector<llvm::Type *> &paramTypes) {
    // Create function type
    llvm::FunctionType *funcType = llvm::FunctionType::get(returnType, paramTypes, false);

    // Declare function in the module
    llvm::Function *func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, module);

    // Add to our map
    functions[name] = func;

    return func;
}

void JSRuntime::declareTypeOperations() {
    // Common types
    llvm::Type *jsValueTy = llvm::Type::getInt64Ty(context);
    llvm::Type *i32Ty = llvm::Type::getInt32Ty(context);
    llvm::Type *i1Ty = llvm::Type::getInt1Ty(context);
    llvm::Type *doubleTy = llvm::Type::getDoubleTy(context);
    llvm::Type *voidTy = llvm::Type::getVoidTy(context);
    llvm::Type *charPtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);

    // Type operations
    declareFunction("js_get_type", i32Ty, {jsValueTy});
    declareFunction("js_typeof", jsValueTy, {jsValueTy});
    declareFunction("js_value_to_number", doubleTy, {jsValueTy});
    declareFunction("js_value_to_boolean", i1Ty, {jsValueTy});
    declareFunction("js_value_to_string", charPtrTy, {jsValueTy});

    // Type conversion
    declareFunction("js_to_string", jsValueTy, {jsValueTy});
    declareFunction("js_to_number", jsValueTy, {jsValueTy});
}

void JSRuntime::declareObjectOperations() {
    // Common types
    llvm::Type *jsValueTy = llvm::Type::getInt64Ty(context);
    llvm::Type *charPtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);

    // Object operations
    declareFunction("js_create_object", jsValueTy, {});
    declareFunction("js_set_property", jsValueTy, {jsValueTy, charPtrTy, jsValueTy});
    declareFunction("js_get_property", jsValueTy, {jsValueTy, charPtrTy});
    declareFunction("js_has_property", jsValueTy, {jsValueTy, charPtrTy});
    declareFunction("js_delete_property", jsValueTy, {jsValueTy, charPtrTy});
}

void JSRuntime::declareStringOperations() {
    // Common types
    llvm::Type *jsValueTy = llvm::Type::getInt64Ty(context);
    llvm::Type *charPtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
    llvm::Type *i32Ty = llvm::Type::getInt32Ty(context);

    // String operations
    declareFunction("js_string_new", jsValueTy, {charPtrTy});
    declareFunction("js_string_length", i32Ty, {jsValueTy});
    declareFunction("js_string_concat", jsValueTy, {jsValueTy, jsValueTy});
    declareFunction("js_string_slice", jsValueTy, {jsValueTy, i32Ty, i32Ty});
}

void JSRuntime::declareArrayOperations() {
    // Common types
    llvm::Type *jsValueTy = llvm::Type::getInt64Ty(context);
    llvm::Type *i32Ty = llvm::Type::getInt32Ty(context);

    // Array operations
    declareFunction("js_array_new", jsValueTy, {i32Ty});
    declareFunction("js_array_get", jsValueTy, {jsValueTy, i32Ty});
    declareFunction("js_array_set", jsValueTy, {jsValueTy, i32Ty, jsValueTy});
    declareFunction("js_array_length", i32Ty, {jsValueTy});
    declareFunction("js_array_push", jsValueTy, {jsValueTy, jsValueTy});
    declareFunction("js_array_pop", jsValueTy, {jsValueTy});
}

void JSRuntime::declareArithmeticOperations() {
    // Common types
    llvm::Type *jsValueTy = llvm::Type::getInt64Ty(context);

    // Arithmetic operations
    declareFunction("js_add", jsValueTy, {jsValueTy, jsValueTy});
    declareFunction("js_subtract", jsValueTy, {jsValueTy, jsValueTy});
    declareFunction("js_multiply", jsValueTy, {jsValueTy, jsValueTy});
    declareFunction("js_divide", jsValueTy, {jsValueTy, jsValueTy});
    declareFunction("js_modulo", jsValueTy, {jsValueTy, jsValueTy});
    declareFunction("js_negate", jsValueTy, {jsValueTy});
    declareFunction("js_increment", jsValueTy, {jsValueTy});
    declareFunction("js_decrement", jsValueTy, {jsValueTy});
}

void JSRuntime::declareComparisonOperations() {
    // Common types
    llvm::Type *jsValueTy = llvm::Type::getInt64Ty(context);

    // Comparison operations
    declareFunction("js_equals", jsValueTy, {jsValueTy, jsValueTy});
    declareFunction("js_strict_equals", jsValueTy, {jsValueTy, jsValueTy});
    declareFunction("js_less_than", jsValueTy, {jsValueTy, jsValueTy});
    declareFunction("js_less_equal", jsValueTy, {jsValueTy, jsValueTy});
    declareFunction("js_greater_than", jsValueTy, {jsValueTy, jsValueTy});
    declareFunction("js_greater_equal", jsValueTy, {jsValueTy, jsValueTy});
}

void JSRuntime::declareControlFlowOperations() {
    // Common types
    llvm::Type *jsValueTy = llvm::Type::getInt64Ty(context);
    llvm::Type *i32Ty = llvm::Type::getInt32Ty(context);
    llvm::Type *voidTy = llvm::Type::getVoidTy(context);

    // Function operations
    declareFunction("js_call", jsValueTy, {jsValueTy, jsValueTy, llvm::PointerType::get(jsValueTy, 0), i32Ty});

    // Console output (for debugging)
    declareFunction("js_console_log", voidTy, {jsValueTy});

    // Memory management
    declareFunction("js_gc_collect", voidTy, {});
}

JSRuntime::~JSRuntime() {
    // Clean up the runtime environment
    cleanupStringCache();
}

void JSRuntime::initialize() {
    // Initialize global runtime structures
}

void JSRuntime::shutdown() {
    // Clean up global resources
    cleanupStringCache();
}

JSRuntime &JSRuntime::getInstance() {
    static JSRuntime instance;
    return instance;
}

uint64_t JSRuntime::createString(const std::string &str) { return js_make_string(str.c_str()); }

std::string JSRuntime::toString(uint64_t jsvalue) {
    const char *str = js_value_to_string(jsvalue);
    std::string result(str);

    // If the string was dynamically allocated by js_value_to_string
    if (JS_GetTag(jsvalue) != JS_TAG_STRING) {
        free(const_cast<char *>(str));
    }

    return result;
}

uint64_t JSRuntime::createObject() { return js_create_object(); }

uint64_t JSRuntime::createArray(int size) {
    JSObject *obj = new JSObject(JSObject::Type::Array);

    if (size > 0) {
        obj->elements.resize(size, JS_MakeUndefined());

        // Set length property
        char lenStr[16];
        snprintf(lenStr, sizeof(lenStr), "%d", size);
        obj->properties["length"] = js_make_string(lenStr);
    }

    return JS_MakeObject(obj);
}

void JSRuntime::print(uint64_t value) { js_print(value); }
