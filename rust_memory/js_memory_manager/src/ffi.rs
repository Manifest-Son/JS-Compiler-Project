use crate::gc::{GarbageCollector, GCConfiguration, GCStatistics};
use crate::object::{JSObject, JSObjectHandle, JSObjectType, JSValue};
use crate::string_interner::{InternedString, get_interner_stats};
use libc::{c_char, c_double, c_int, c_void, size_t};
use std::ffi::{CStr, CString};
use std::ptr;
use std::sync::Arc;

// Export the GC and object types to C++
pub type RustGCHandle = *mut GarbageCollector;
pub type RustObjectHandle = *mut JSObject;

/// Initialize the memory manager and return a handle to the GC
#[no_mangle]
pub extern "C" fn js_memory_init() -> RustGCHandle {
    let gc = GarbageCollector::new();
    // Convert Arc<GarbageCollector> to raw pointer
    Arc::into_raw(gc) as *mut GarbageCollector
}

/// Clean up and destroy the memory manager
#[no_mangle]
pub extern "C" fn js_memory_shutdown(gc_handle: RustGCHandle) {
    if !gc_handle.is_null() {
        // Safety: Convert back to Arc and drop it
        unsafe {
            let _ = Arc::from_raw(gc_handle);
        }
    }
}

/// Configure the garbage collector
#[no_mangle]
pub extern "C" fn js_gc_configure(gc_handle: RustGCHandle, config: *const GCConfiguration) {
    if gc_handle.is_null() || config.is_null() {
        return;
    }

    // Safety: We trust the C++ side to provide a valid configuration
    let gc = unsafe { &*(gc_handle as *const GarbageCollector) };
    let config = unsafe { &*config };
    
    gc.configure(config.clone());
}

/// Force a garbage collection cycle
#[no_mangle]
pub extern "C" fn js_gc_collect(gc_handle: RustGCHandle) {
    if gc_handle.is_null() {
        return;
    }

    // Safety: We trust the gc_handle to be valid
    let gc = unsafe { &*(gc_handle as *const GarbageCollector) };
    gc.collect();
}

/// Add a root object that shouldn't be collected
#[no_mangle]
pub extern "C" fn js_gc_add_root(gc_handle: RustGCHandle, obj_handle: RustObjectHandle) {
    if gc_handle.is_null() || obj_handle.is_null() {
        return;
    }

    // Safety: We trust both handles to be valid
    let gc = unsafe { &*(gc_handle as *const GarbageCollector) };
    gc.add_root(obj_handle);
}

/// Remove a root object
#[no_mangle]
pub extern "C" fn js_gc_remove_root(gc_handle: RustGCHandle, obj_handle: RustObjectHandle) {
    if gc_handle.is_null() || obj_handle.is_null() {
        return;
    }

    // Safety: We trust both handles to be valid
    let gc = unsafe { &*(gc_handle as *const GarbageCollector) };
    gc.remove_root(obj_handle);
}

/// Get garbage collector statistics
#[no_mangle]
pub extern "C" fn js_gc_get_stats(gc_handle: RustGCHandle) -> GCStatistics {
    if gc_handle.is_null() {
        return GCStatistics {
            allocation_count: 0,
            collection_count: 0,
            objects_freed: 0,
            young_generation_size: 0,
            old_generation_size: 0,
        };
    }

    // Safety: We trust the handle to be valid
    let gc = unsafe { &*(gc_handle as *const GarbageCollector) };
    gc.statistics()
}

/// Create a new JavaScript object
#[no_mangle]
pub extern "C" fn js_create_object(gc_handle: RustGCHandle, obj_type: c_int) -> RustObjectHandle {
    if gc_handle.is_null() {
        return ptr::null_mut();
    }

    // Safety: We trust the handle to be valid
    let gc = unsafe { &*(gc_handle as *const GarbageCollector) };
    
    // Convert C int to JSObjectType enum
    let obj_type = match obj_type {
        0 => JSObjectType::Object,
        1 => JSObjectType::Array,
        2 => JSObjectType::Function,
        3 => JSObjectType::String,
        4 => JSObjectType::Number,
        5 => JSObjectType::Boolean,
        6 => JSObjectType::Null,
        7 => JSObjectType::Undefined,
        _ => JSObjectType::Object, // Default to plain object
    };
    
    // Create object and get raw pointer
    let handle = gc.create_object(obj_type);
    Arc::into_raw(handle.ptr) as *mut JSObject
}

/// Release an object handle
#[no_mangle]
pub extern "C" fn js_release_object(obj_handle: RustObjectHandle) {
    if !obj_handle.is_null() {
        // Safety: Convert raw pointer back to Arc and let it drop
        unsafe {
            let _ = Arc::from_raw(obj_handle);
        }
    }
}

/// Set a property on an object with a string value
#[no_mangle]
pub extern "C" fn js_set_property_string(
    obj_handle: RustObjectHandle,
    key: *const c_char,
    value: *const c_char,
) -> c_int {
    if obj_handle.is_null() || key.is_null() || value.is_null() {
        return 0;
    }

    // Safety: Convert raw pointers to Rust types
    unsafe {
        let obj = &*(obj_handle as *const JSObject);
        let key_str = CStr::from_ptr(key).to_str().unwrap_or("");
        let val_str = CStr::from_ptr(value).to_str().unwrap_or("");
        
        // Use interned strings for both keys and values
        obj.set_property(key_str, JSValue::String(InternedString::new(val_str)));
        1
    }
}

/// Set a property on an object with a number value
#[no_mangle]
pub extern "C" fn js_set_property_number(
    obj_handle: RustObjectHandle,
    key: *const c_char,
    value: c_double,
) -> c_int {
    if obj_handle.is_null() || key.is_null() {
        return 0;
    }

    // Safety: Convert raw pointers to Rust types
    unsafe {
        let obj = &*(obj_handle as *const JSObject);
        let key_str = CStr::from_ptr(key).to_str().unwrap_or("");
        
        obj.set_property(key_str, JSValue::Number(value));
        1
    }
}

/// Set a property on an object with a boolean value
#[no_mangle]
pub extern "C" fn js_set_property_boolean(
    obj_handle: RustObjectHandle,
    key: *const c_char,
    value: c_int,
) -> c_int {
    if obj_handle.is_null() || key.is_null() {
        return 0;
    }

    // Safety: Convert raw pointers to Rust types
    unsafe {
        let obj = &*(obj_handle as *const JSObject);
        let key_str = CStr::from_ptr(key).to_str().unwrap_or("");
        
        obj.set_property(key_str, JSValue::Boolean(value != 0));
        1
    }
}

/// Set a property on an object with an object value
#[no_mangle]
pub extern "C" fn js_set_property_object(
    obj_handle: RustObjectHandle,
    key: *const c_char,
    value: RustObjectHandle,
) -> c_int {
    if obj_handle.is_null() || key.is_null() || value.is_null() {
        return 0;
    }

    // Safety: Convert raw pointers to Rust types
    unsafe {
        let obj = &*(obj_handle as *const JSObject);
        let key_str = CStr::from_ptr(key).to_str().unwrap_or("");
        
        // Create a handle from the raw pointer
        if let Some(value_handle) = JSObjectHandle::from_raw(value) {
            obj.set_property(key_str, JSValue::Object(value_handle));
            1
        } else {
            0
        }
    }
}

/// Get a string property from an object
#[no_mangle]
pub extern "C" fn js_get_property_string(
    obj_handle: RustObjectHandle,
    key: *const c_char,
    buffer: *mut c_char,
    buffer_size: size_t,
) -> c_int {
    if obj_handle.is_null() || key.is_null() || buffer.is_null() || buffer_size == 0 {
        return 0;
    }

    // Safety: Convert raw pointers to Rust types
    unsafe {
        let obj = &*(obj_handle as *const JSObject);
        let key_str = CStr::from_ptr(key).to_str().unwrap_or("");
        
        // Get the property
        let value = obj.get_property(key_str);
        
        // Extract string value
        if let JSValue::String(s) = value {
            // InternedString implements Deref<Target=str>, so we can use as_bytes() directly
            let bytes = s.as_bytes();
            let copy_size = bytes.len().min(buffer_size - 1);
            
            ptr::copy_nonoverlapping(bytes.as_ptr(), buffer as *mut u8, copy_size);
            *buffer.add(copy_size) = 0; // Null terminate
            
            1
        } else {
            0
        }
    }
}

/// Get a number property from an object
#[no_mangle]
pub extern "C" fn js_get_property_number(
    obj_handle: RustObjectHandle,
    key: *const c_char,
    out_value: *mut c_double,
) -> c_int {
    if obj_handle.is_null() || key.is_null() || out_value.is_null() {
        return 0;
    }

    // Safety: Convert raw pointers to Rust types
    unsafe {
        let obj = &*(obj_handle as *const JSObject);
        let key_str = CStr::from_ptr(key).to_str().unwrap_or("");
        
        // Get the property
        let value = obj.get_property(key_str);
        
        // Extract number value
        if let JSValue::Number(n) = value {
            *out_value = n;
            1
        } else {
            0
        }
    }
}

/// Get a boolean property from an object
#[no_mangle]
pub extern "C" fn js_get_property_boolean(
    obj_handle: RustObjectHandle,
    key: *const c_char,
    out_value: *mut c_int,
) -> c_int {
    if obj_handle.is_null() || key.is_null() || out_value.is_null() {
        return 0;
    }

    // Safety: Convert raw pointers to Rust types
    unsafe {
        let obj = &*(obj_handle as *const JSObject);
        let key_str = CStr::from_ptr(key).to_str().unwrap_or("");
        
        // Get the property
        let value = obj.get_property(key_str);
        
        // Extract boolean value
        if let JSValue::Boolean(b) = value {
            *out_value = if b { 1 } else { 0 };
            1
        } else {
            0
        }
    }
}

/// Get an object property from an object
#[no_mangle]
pub extern "C" fn js_get_property_object(
    obj_handle: RustObjectHandle,
    key: *const c_char,
    out_value: *mut RustObjectHandle,
) -> c_int {
    if obj_handle.is_null() || key.is_null() || out_value.is_null() {
        return 0;
    }

    // Safety: Convert raw pointers to Rust types
    unsafe {
        let obj = &*(obj_handle as *const JSObject);
        let key_str = CStr::from_ptr(key).to_str().unwrap_or("");
        
        // Get the property
        let value = obj.get_property(key_str);
        
        // Extract object value
        if let JSValue::Object(handle) = value {
            // Increment ref count to avoid dropping when this function returns
            let ptr = Arc::into_raw(handle.ptr.clone()) as *mut JSObject;
            *out_value = ptr;
            1
        } else {
            *out_value = ptr::null_mut();
            0
        }
    }
}

/// Set a finalizer function for an object
#[no_mangle]
pub extern "C" fn js_set_finalizer(
    obj_handle: RustObjectHandle,
    finalizer: extern "C" fn(*mut JSObject)
) -> c_int {
    if obj_handle.is_null() {
        return 0;
    }

    // Safety: We trust the handle to be valid
    unsafe {
        let obj = &*(obj_handle as *const JSObject);
        obj.set_finalizer(finalizer);
        1
    }
}

/// Get the type of an object
#[no_mangle]
pub extern "C" fn js_get_object_type(obj_handle: RustObjectHandle) -> c_int {
    if obj_handle.is_null() {
        return -1;
    }

    // Safety: We trust the handle to be valid
    unsafe {
        let obj = &*(obj_handle as *const JSObject);
        let obj_type = obj.inner.read().obj_type;
        
        // Convert JSObjectType to C int
        match obj_type {
            JSObjectType::Object => 0,
            JSObjectType::Array => 1,
            JSObjectType::Function => 2,
            JSObjectType::String => 3,
            JSObjectType::Number => 4,
            JSObjectType::Boolean => 5,
            JSObjectType::Null => 6,
            JSObjectType::Undefined => 7,
        }
    }
}

/// Get the number of unique strings in the string interner
#[no_mangle]
pub extern "C" fn js_get_interned_string_count() -> size_t {
    let (count, _) = get_interner_stats();
    count
}

/// Get the approximate memory usage of the string interner
#[no_mangle]
pub extern "C" fn js_get_interned_string_memory() -> size_t {
    let (_, memory) = get_interner_stats();
    memory
}