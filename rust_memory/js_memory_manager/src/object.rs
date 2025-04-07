use libc::{c_char, c_double, c_int, c_void};
use parking_lot::RwLock;
use std::collections::HashMap;
use std::ffi::{CStr, CString};
use std::fmt;
use std::sync::{Arc, Weak};
use std::sync::atomic::{AtomicBool, Ordering};
use crate::shape::PropertyShape;
use crate::string_interner::InternedString;

/// Type of JavaScript object
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum JSObjectType {
    Object,
    Array,
    Function,
    String,
    Number,
    Boolean,
    Null,
    Undefined,
}

/// JavaScript value type
#[derive(Clone)]
pub enum JSValue {
    Undefined,
    Null,
    Boolean(bool),
    Number(f64),
    // Use InternedString instead of String to deduplicate string values
    String(InternedString),
    Object(JSObjectHandle),
}

impl fmt::Debug for JSValue {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            JSValue::Undefined => write!(f, "undefined"),
            JSValue::Null => write!(f, "null"),
            JSValue::Boolean(b) => write!(f, "{}", b),
            JSValue::Number(n) => write!(f, "{}", n),
            JSValue::String(s) => write!(f, "\"{}\"", s),
            JSValue::Object(_) => write!(f, "[object]"),
        }
    }
}

impl Default for JSValue {
    fn default() -> Self {
        JSValue::Undefined
    }
}

// Helper conversion implementations for JSValue
impl From<&str> for JSValue {
    fn from(s: &str) -> Self {
        JSValue::String(InternedString::new(s))
    }
}

impl From<String> for JSValue {
    fn from(s: String) -> Self {
        JSValue::String(InternedString::new(&s))
    }
}

impl From<f64> for JSValue {
    fn from(n: f64) -> Self {
        JSValue::Number(n)
    }
}

impl From<bool> for JSValue {
    fn from(b: bool) -> Self {
        JSValue::Boolean(b)
    }
}

/// Internal structure of a JavaScript object
pub struct JSObjectInner {
    pub obj_type: JSObjectType,
    // Using shape-based optimization
    pub shape: Arc<PropertyShape>,
    pub values: Vec<JSValue>,
    pub marked: bool,
    pub finalizer: Option<extern "C" fn(*mut JSObject)>,
}

impl JSObjectInner {
    /// Create a new JS object inner state
    pub fn new(obj_type: JSObjectType) -> Self {
        Self {
            obj_type,
            shape: PropertyShape::new_empty(),
            values: Vec::new(),
            marked: false,
            finalizer: None,
        }
    }
}

/// JavaScript object - thread-safe wrapper around properties
pub struct JSObject {
    pub inner: RwLock<JSObjectInner>,
}

impl JSObject {
    /// Create a new JavaScript object of the specified type
    pub fn new(obj_type: JSObjectType) -> Arc<Self> {
        Arc::new(Self {
            inner: RwLock::new(JSObjectInner::new(obj_type)),
        })
    }
    
    /// Set a property on this object
    pub fn set_property(&self, key: &str, value: JSValue) {
        let mut inner = self.inner.write();
        
        // Check if property already exists in the current shape
        if let Some(index) = inner.shape.get_property_index(key) {
            // Property exists, just update the value
            if index < inner.values.len() {
                inner.values[index] = value;
            } else {
                // This shouldn't happen if the shape is consistent, but handle it anyway
                inner.values.resize_with(index + 1, || JSValue::Undefined);
                inner.values[index] = value;
            }
        } else {
            // Property doesn't exist, transition to a new shape
            let old_shape = inner.shape.clone();
            let new_shape = old_shape.transition_to(key);
            
            // Update reference counts
            old_shape.remove_reference();
            new_shape.add_reference();
            
            // Get the index for the new property
            let index = new_shape.get_property_index(key).unwrap();
            
            // Ensure values vector has enough capacity
            if index >= inner.values.len() {
                inner.values.resize_with(index + 1, || JSValue::Undefined);
            }
            
            // Set the value and update the shape
            inner.values[index] = value;
            inner.shape = new_shape;
        }
    }
    
    /// Get a property from this object
    pub fn get_property(&self, key: &str) -> JSValue {
        let inner = self.inner.read();
        
        // Check if property exists in the current shape
        if let Some(index) = inner.shape.get_property_index(key) {
            if index < inner.values.len() {
                // Return the value if it exists
                inner.values[index].clone()
            } else {
                // Index out of bounds (shouldn't happen with well-formed shapes)
                JSValue::Undefined
            }
        } else {
            // Property not found
            JSValue::Undefined
        }
    }
    
    /// Mark object for garbage collection
    pub fn mark(&self) {
        let mut inner = self.inner.write();
        inner.marked = true;
        
        // Mark any object properties recursively
        for value in inner.values.iter() {
            if let JSValue::Object(obj) = value {
                obj.ptr.mark();
            }
        }
    }
    
    /// Unmark object after garbage collection
    pub fn unmark(&self) {
        let mut inner = self.inner.write();
        inner.marked = false;
    }
    
    /// Check if object is marked
    pub fn is_marked(&self) -> bool {
        let inner = self.inner.read();
        inner.marked
    }
    
    /// Set a finalizer to be called when object is collected
    pub fn set_finalizer(&self, finalizer: extern "C" fn(*mut JSObject)) {
        let mut inner = self.inner.write();
        inner.finalizer = Some(finalizer);
    }
    
    /// Get all property names in this object
    pub fn property_names(&self) -> Vec<String> {
        let inner = self.inner.read();
        inner.shape.property_names()
    }
}

impl Drop for JSObject {
    fn drop(&mut self) {
        // Call the finalizer if set
        if let Some(finalizer) = self.inner.read().finalizer {
            // Safety: We're passing a raw pointer to the finalizer
            finalizer(self as *mut JSObject);
        }
    }
}

/// Safe handle to a JavaScript object
#[derive(Clone)]
pub struct JSObjectHandle {
    pub ptr: Arc<JSObject>,
}

impl JSObjectHandle {
    /// Create a handle from a raw pointer
    pub fn from_raw(raw: *mut JSObject) -> Option<Self> {
        if raw.is_null() {
            None
        } else {
            // Safety: Convert raw pointer back to Arc
            unsafe {
                let arc = Arc::from_raw(raw);
                let ptr = arc.clone();
                // Don't drop the original Arc when this function returns
                std::mem::forget(arc);
                Some(Self { ptr })
            }
        }
    }
}

impl fmt::Debug for JSObjectHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let inner = self.ptr.inner.read();
        write!(f, "JSObject({:?})", inner.obj_type)
    }
}