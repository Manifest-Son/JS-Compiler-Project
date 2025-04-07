#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Forward declarations for Rust types
extern "C" {
typedef void *RustGCHandle;
typedef void *RustObjectHandle;

// These enums match their Rust counterparts
enum JSObjectType { Object = 0, Array = 1, Function = 2, String = 3, Number = 4, Boolean = 5, Null = 6, Undefined = 7 };

// Configuration struct for the GC
struct GCConfiguration {
    size_t young_gen_threshold_kb;
    size_t old_gen_threshold_kb;
    uint64_t max_pause_ms;
    bool incremental;
    bool verbose;
};

// Statistics returned from the GC
struct GCStatistics {
    size_t allocation_count;
    size_t collection_count;
    size_t objects_freed;
    size_t young_generation_size;
    size_t old_generation_size;
};

// FFI functions
RustGCHandle js_memory_init();
void js_memory_shutdown(RustGCHandle gc);
void js_gc_configure(RustGCHandle gc, const GCConfiguration *config);
void js_gc_collect(RustGCHandle gc);
void js_gc_add_root(RustGCHandle gc, RustObjectHandle obj);
void js_gc_remove_root(RustGCHandle gc, RustObjectHandle obj);
GCStatistics js_gc_get_stats(RustGCHandle gc);

RustObjectHandle js_create_object(RustGCHandle gc, int obj_type);
void js_release_object(RustObjectHandle obj);

int js_set_property_string(RustObjectHandle obj, const char *key, const char *value);
int js_set_property_number(RustObjectHandle obj, const char *key, double value);
int js_set_property_boolean(RustObjectHandle obj, const char *key, int value);
int js_set_property_object(RustObjectHandle obj, const char *key, RustObjectHandle value);

int js_get_property_string(RustObjectHandle obj, const char *key, char *buffer, size_t buffer_size);
int js_get_property_number(RustObjectHandle obj, const char *key, double *out_value);
int js_get_property_boolean(RustObjectHandle obj, const char *key, int *out_value);
int js_get_property_object(RustObjectHandle obj, const char *key, RustObjectHandle *out_value);

int js_set_finalizer(RustObjectHandle obj, void (*finalizer)(RustObjectHandle));
int js_get_object_type(RustObjectHandle obj);
}

namespace js_memory {

    // Forward declarations
    class MemoryManager;
    class JSObjectWrapper;

    /**
     * RAII wrapper for JavaScript values to ensure proper memory management
     */
    class JSValue {
    public:
        enum class Type { Undefined, Null, Boolean, Number, String, Object };

        JSValue() : type_(Type::Undefined) {}
        JSValue(std::nullptr_t) : type_(Type::Null) {}
        JSValue(bool value) : type_(Type::Boolean), bool_val_(value) {}
        JSValue(double value) : type_(Type::Number), num_val_(value) {}
        JSValue(int value) : type_(Type::Number), num_val_(static_cast<double>(value)) {}
        JSValue(const std::string &value) : type_(Type::String), str_val_(value) {}
        JSValue(std::shared_ptr<JSObjectWrapper> obj) : type_(Type::Object), obj_val_(obj) {}

        Type type() const { return type_; }
        bool is_undefined() const { return type_ == Type::Undefined; }
        bool is_null() const { return type_ == Type::Null; }
        bool is_boolean() const { return type_ == Type::Boolean; }
        bool is_number() const { return type_ == Type::Number; }
        bool is_string() const { return type_ == Type::String; }
        bool is_object() const { return type_ == Type::Object; }

        bool as_boolean() const;
        double as_number() const;
        std::string as_string() const;
        std::shared_ptr<JSObjectWrapper> as_object() const;

    private:
        Type type_;
        bool bool_val_ = false;
        double num_val_ = 0.0;
        std::string str_val_;
        std::shared_ptr<JSObjectWrapper> obj_val_;
    };

    /**
     * Safe C++ wrapper for Rust JSObject
     */
    class JSObjectWrapper {
    public:
        JSObjectWrapper(RustObjectHandle handle);
        ~JSObjectWrapper();

        // Prevent accidental copies
        JSObjectWrapper(const JSObjectWrapper &) = delete;
        JSObjectWrapper &operator=(const JSObjectWrapper &) = delete;

        // Allow move semantics
        JSObjectWrapper(JSObjectWrapper &&other) noexcept;
        JSObjectWrapper &operator=(JSObjectWrapper &&other) noexcept;

        // Property access
        void set_property(const std::string &key, const JSValue &value);
        JSValue get_property(const std::string &key) const;

        // Type information
        JSObjectType type() const;

        // Set finalizer callback
        void set_finalizer(std::function<void(RustObjectHandle)> finalizer);

        // Access the underlying handle (for advanced usage)
        RustObjectHandle handle() const { return handle_; }

    private:
        RustObjectHandle handle_;
        static void finalize_wrapper(RustObjectHandle handle);
        static std::unordered_map<RustObjectHandle, std::function<void(RustObjectHandle)>> finalizers_;
    };

    /**
     * Garbage collector for JavaScript objects
     */
    class GarbageCollector {
    public:
        GarbageCollector(RustGCHandle handle);
        ~GarbageCollector();

        // Prevent copies
        GarbageCollector(const GarbageCollector &) = delete;
        GarbageCollector &operator=(const GarbageCollector &) = delete;

        // Configure the GC
        void configure(size_t young_gen_threshold_kb, size_t old_gen_threshold_kb, uint64_t max_pause_ms,
                       bool incremental, bool verbose);

        // Force collection
        void collect();

        // Root management
        void add_root(std::shared_ptr<JSObjectWrapper> obj);
        void remove_root(std::shared_ptr<JSObjectWrapper> obj);

        // Get statistics
        GCStatistics statistics() const;

        // Access the underlying handle (for advanced usage)
        RustGCHandle handle() const { return handle_; }

    private:
        RustGCHandle handle_;
    };

    /**
     * Main memory manager for JavaScript compiler
     */
    class MemoryManager {
    public:
        // Get the singleton instance
        static MemoryManager &instance();

        // Initialize and shutdown
        void initialize();
        void shutdown();

        // Object creation
        std::shared_ptr<JSObjectWrapper> create_object(JSObjectType type);

        // Configure the garbage collector
        void configure_gc(size_t young_gen_threshold_kb, size_t old_gen_threshold_kb, uint64_t max_pause_ms,
                          bool incremental, bool verbose);

        // Manually trigger garbage collection
        void collect();

        // Get statistics
        GCStatistics gc_statistics() const;

        // Get the GC
        std::shared_ptr<GarbageCollector> gc() const { return gc_; }

    private:
        MemoryManager() = default;
        ~MemoryManager() = default;

        // Prevent copies
        MemoryManager(const MemoryManager &) = delete;
        MemoryManager &operator=(const MemoryManager &) = delete;

        bool initialized_ = false;
        std::shared_ptr<GarbageCollector> gc_;
    };

} // namespace js_memory
