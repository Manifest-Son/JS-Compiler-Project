//! JavaScript Memory Manager implemented in Rust
//!
//! This library provides memory management and garbage collection
//! capabilities for the JavaScript Compiler project.

mod gc;
mod object;
mod ffi;
mod shape;
mod string_interner;

// Re-export items that need to be accessible from the FFI boundary
pub use ffi::*;
pub use gc::GarbageCollector;
pub use object::{JSObject, JSObjectHandle, JSObjectType, JSValue};
pub use shape::PropertyShape;
pub use string_interner::{InternedString, get_interner_stats};

#[cfg(test)]
mod tests {
    use super::*;
    use crate::string_interner::InternedString;
    use std::mem::size_of;
    use std::sync::Arc;
    use std::ops::Deref;

    #[test]
    fn test_create_object() {
        let gc = GarbageCollector::new();
        let obj = gc.create_object(JSObjectType::Object);
        assert!(!obj.is_null());
    }

    #[test]
    fn test_shape_based_properties() {
        use crate::object::{JSObject, JSValue};
        use crate::shape::PropertyShape;

        // Create a basic object
        let obj1 = JSObject::new(JSObjectType::Object);
        
        // Add some properties
        obj1.set_property("name", JSValue::String("Object 1".to_string()));
        obj1.set_property("value", JSValue::Number(42.0));
        
        // Create another object with the same property names
        let obj2 = JSObject::new(JSObjectType::Object);
        obj2.set_property("name", JSValue::String("Object 2".to_string()));
        obj2.set_property("value", JSValue::Number(100.0));
        
        // Both objects should have the same shape
        {
            let inner1 = obj1.inner.read();
            let inner2 = obj2.inner.read();
            assert_eq!(inner1.shape.get_property_map().len(), inner2.shape.get_property_map().len());
            
            // Get shape IDs through debug output (implementation detail)
            let shape1_dbg = format!("{:?}", inner1.shape);
            let shape2_dbg = format!("{:?}", inner2.shape);
            
            // For objects with same property names added in same order, shapes should be identical
            assert_eq!(shape1_dbg, shape2_dbg);
        }
        
        // Values should be correctly stored and retrieved
        assert!(matches!(obj1.get_property("name"), JSValue::String(s) if s == "Object 1"));
        assert!(matches!(obj1.get_property("value"), JSValue::Number(n) if n == 42.0));
        
        assert!(matches!(obj2.get_property("name"), JSValue::String(s) if s == "Object 2"));
        assert!(matches!(obj2.get_property("value"), JSValue::Number(n) if n == 100.0));
        
        // Add an additional property to obj2
        obj2.set_property("extra", JSValue::Boolean(true));
        
        // Now shapes should be different
        {
            let inner1 = obj1.inner.read();
            let inner2 = obj2.inner.read();
            
            // obj2 should have one more property than obj1
            assert_eq!(inner1.shape.get_property_map().len() + 1, inner2.shape.get_property_map().len());
            
            // Shape objects should be different
            let shape1_dbg = format!("{:?}", inner1.shape);
            let shape2_dbg = format!("{:?}", inner2.shape);
            assert_ne!(shape1_dbg, shape2_dbg);
        }
        
        // Original properties still accessible
        assert!(matches!(obj2.get_property("name"), JSValue::String(s) if s == "Object 2"));
        assert!(matches!(obj2.get_property("value"), JSValue::Number(n) if n == 100.0));
        
        // New property also accessible
        assert!(matches!(obj2.get_property("extra"), JSValue::Boolean(b) if b == true));
        
        // Property shouldn't exist on obj1
        assert!(matches!(obj1.get_property("extra"), JSValue::Undefined));
    }
    
    #[test]
    fn test_string_interning() {
        // Create multiple identical strings
        let s1 = InternedString::new("hello world");
        let s2 = InternedString::new("hello world");
        let s3 = InternedString::new("hello world");
        
        // Different content should be different interned strings
        let s4 = InternedString::new("different");
        
        // Test pointer equality - all identical strings should share the same storage
        assert!(Arc::ptr_eq(&s1.inner, &s2.inner));
        assert!(Arc::ptr_eq(&s1.inner, &s3.inner));
        
        // Different content should not be pointer equal
        assert!(!Arc::ptr_eq(&s1.inner, &s4.inner));
        
        // Test value equality
        assert_eq!(s1.deref(), "hello world");
        assert_eq!(s2.deref(), "hello world");
        assert_eq!(s3.deref(), "hello world");
        assert_eq!(s4.deref(), "different");
        
        // Test that we can use them in hash maps
        use std::collections::HashMap;
        let mut map = HashMap::new();
        map.insert(s1.clone(), 1);
        map.insert(s2.clone(), 2);  // Should overwrite the first entry since they're equal
        
        assert_eq!(map.len(), 1);   // Only one entry should exist
        assert_eq!(map.get(&s3), Some(&2));  // s3 should find the entry even though we inserted s2
    }
    
    #[test]
    fn test_interned_strings_with_jsvalue() {
        // Create objects with string properties that have the same content
        let obj1 = JSObject::new(JSObjectType::Object);
        let obj2 = JSObject::new(JSObjectType::Object);
        
        // Set properties with identical content
        obj1.set_property("name", JSValue::from("John Doe"));
        obj1.set_property("city", JSValue::from("New York"));
        
        obj2.set_property("name", JSValue::from("John Doe"));
        obj2.set_property("city", JSValue::from("New York"));
        
        // Access the properties and verify they're interned
        if let JSValue::String(s1) = obj1.get_property("name") {
            if let JSValue::String(s2) = obj2.get_property("name") {
                // Both should point to the same string in memory
                assert!(Arc::ptr_eq(&s1.inner, &s2.inner));
            } else {
                panic!("Expected string value");
            }
        } else {
            panic!("Expected string value");
        }
        
        // Check interning stats
        let (count, memory) = get_interner_stats();
        println!("Interned strings: {}, Memory usage: {} bytes", count, memory);
        
        // We should have 2 unique strings (not 4), since "John Doe" and "New York" are each used twice
        assert_eq!(count, 2);
    }
}