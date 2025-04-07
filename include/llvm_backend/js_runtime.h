#ifndef JS_RUNTIME_H
#define JS_RUNTIME_H

#include <cstdint>
#include <string>
#include "js_value_type.h"

#ifdef __cplusplus
extern "C" {
#endif

// Type operations
int32_t js_get_type(uint64_t value);
uint64_t js_typeof(uint64_t value);
double js_value_to_number(uint64_t value);
bool js_value_to_boolean(uint64_t value);
const char *js_value_to_string(uint64_t value);

// String operations
uint64_t js_make_string(const char *str);
const char *js_get_string_ptr(uint64_t value);
uint64_t js_concat_strings(uint64_t a, uint64_t b);

// Object operations
uint64_t js_create_object();
uint64_t js_create_array();
void js_set_property(uint64_t obj, const char *key, uint64_t value);
uint64_t js_get_property(uint64_t obj, const char *key);
bool js_has_property(uint64_t obj, const char *key);
void js_delete_property(uint64_t obj, const char *key);

// Array operations
uint64_t js_array_get(uint64_t array, int32_t index);
void js_array_set(uint64_t array, int32_t index, uint64_t value);
int32_t js_array_length(uint64_t array);
uint64_t js_create_array_with_elements(int32_t count, uint64_t *elements);

// Function operations
uint64_t js_create_closure(void *function_ptr);
uint64_t js_call_function(uint64_t function, int32_t argc, uint64_t *argv);

// Arithmetic operations
uint64_t js_add(uint64_t a, uint64_t b);
uint64_t js_subtract(uint64_t a, uint64_t b);
uint64_t js_multiply(uint64_t a, uint64_t b);
uint64_t js_divide(uint64_t a, uint64_t b);

// Comparison operations
uint64_t js_equal(uint64_t a, uint64_t b);
uint64_t js_strict_equal(uint64_t a, uint64_t b);
uint64_t js_less_than(uint64_t a, uint64_t b);
uint64_t js_greater_than(uint64_t a, uint64_t b);

// I/O operations
void js_print(uint64_t value);
uint64_t js_read_line();

// Memory management
void *js_alloc(uint64_t size);
void js_free(void *ptr);

// Type checking helpers
inline bool js_is_number(uint64_t value) { return JS_GetTag(value) == JS_TAG_NUMBER; }

inline bool js_is_string(uint64_t value) { return JS_GetTag(value) == JS_TAG_STRING; }

inline bool js_is_boolean(uint64_t value) { return JS_GetTag(value) == JS_TAG_BOOLEAN; }

inline bool js_is_object(uint64_t value) { return JS_GetTag(value) == JS_TAG_OBJECT; }

inline bool js_is_function(uint64_t value) { return JS_GetTag(value) == JS_TAG_FUNCTION; }

inline bool js_is_undefined(uint64_t value) { return JS_GetTag(value) == JS_TAG_UNDEFINED; }

inline bool js_is_null(uint64_t value) { return JS_GetTag(value) == JS_TAG_NULL; }

#ifdef __cplusplus
}
#endif

class JSRuntime {
public:
    // Initialize the runtime
    static void initialize();

    // Cleanup the runtime
    static void shutdown();

    // Get singleton instance
    static JSRuntime &getInstance();

    // Create a new JavaScript string
    uint64_t createString(const std::string &str);

    // Get a C++ string from a JavaScript string value
    std::string toString(uint64_t jsvalue);

    // Create a new JavaScript object
    uint64_t createObject();

    // Create a new JavaScript array
    uint64_t createArray(int size = 0);

    // Print a JavaScript value
    void print(uint64_t value);

private:
    // Private constructor for singleton pattern
    JSRuntime();

    // Private destructor
    ~JSRuntime();

    // String interning table to avoid duplicate strings
    // This would be implemented using a hash map in the cpp file
};

#endif // JS_RUNTIME_H
