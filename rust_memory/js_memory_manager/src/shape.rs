use std::collections::HashMap;
use std::sync::{Arc, Weak};
use std::sync::atomic::{AtomicUsize, Ordering};
use parking_lot::RwLock;
use crate::string_interner::InternedString;

/// A PropertyShape represents the structure of an object's properties
/// It contains the property names and their corresponding index in the values vector
#[derive(Debug)]
pub struct PropertyShape {
    // Unique identifier for this shape
    id: usize,
    // Maps property names to indices in the values array
    // Using InternedString for optimized storage and comparison
    property_map: HashMap<InternedString, usize>,
    // Reference to the parent shape (for shape transitions)
    parent: Option<Weak<PropertyShape>>,
    // Property added in this shape (compared to parent)
    added_property: Option<InternedString>,
    // Cache of transitions to other shapes
    transitions: RwLock<HashMap<InternedString, Weak<PropertyShape>>>,
    // Number of objects using this shape (for statistics)
    ref_count: AtomicUsize,
}

impl PropertyShape {
    /// Create a new empty property shape (root shape)
    pub fn new_empty() -> Arc<Self> {
        static NEXT_SHAPE_ID: AtomicUsize = AtomicUsize::new(0);
        
        Arc::new(Self {
            id: NEXT_SHAPE_ID.fetch_add(1, Ordering::SeqCst),
            property_map: HashMap::new(),
            parent: None,
            added_property: None,
            transitions: RwLock::new(HashMap::new()),
            ref_count: AtomicUsize::new(0),
        })
    }
    
    /// Get the index of a property in the values array
    pub fn get_property_index(&self, name: &str) -> Option<usize> {
        // Create a temporary interned string for lookup only
        let interned_name = InternedString::new(name);
        self.property_map.get(&interned_name).copied()
    }
    
    /// Get a transition shape by adding a new property
    pub fn transition_to(&self, property: &str) -> Arc<PropertyShape> {
        // Intern the property name for efficient storage and comparison
        let interned_property = InternedString::new(property);
        
        // First check if we already have this transition
        {
            let transitions = self.transitions.read();
            if let Some(weak_shape) = transitions.get(&interned_property) {
                if let Some(shape) = weak_shape.upgrade() {
                    return shape;
                }
            }
        }
        
        // Create new shape as a transition from this one
        let next_index = self.property_map.len();
        let mut new_map = self.property_map.clone();
        new_map.insert(interned_property.clone(), next_index);
        
        let self_arc = match &self.parent {
            Some(parent_weak) => {
                if let Some(parent) = parent_weak.upgrade() {
                    // Try to get grandparent's strong reference
                    parent
                } else {
                    // Fall back to empty shape if parent is gone
                    PropertyShape::new_empty()
                }
            },
            None => PropertyShape::new_empty(),
        };
        
        static NEXT_SHAPE_ID: AtomicUsize = AtomicUsize::new(0);
        
        // Create the new shape
        let new_shape = Arc::new(PropertyShape {
            id: NEXT_SHAPE_ID.fetch_add(1, Ordering::SeqCst),
            property_map: new_map,
            parent: Some(Arc::downgrade(&self_arc)),
            added_property: Some(interned_property.clone()),
            transitions: RwLock::new(HashMap::new()),
            ref_count: AtomicUsize::new(0),
        });
        
        // Cache this transition
        let mut transitions = self.transitions.write();
        transitions.insert(interned_property, Arc::downgrade(&new_shape));
        
        new_shape
    }
    
    /// Get the number of properties in this shape
    pub fn property_count(&self) -> usize {
        self.property_map.len()
    }
    
    /// Increment the reference count when an object adopts this shape
    pub fn add_reference(&self) {
        self.ref_count.fetch_add(1, Ordering::SeqCst);
    }
    
    /// Decrement the reference count when an object no longer uses this shape
    pub fn remove_reference(&self) {
        self.ref_count.fetch_sub(1, Ordering::SeqCst);
    }
    
    /// Get all property names in this shape
    pub fn property_names(&self) -> Vec<String> {
        self.property_map.keys()
            .map(|interned| interned.as_str().to_string())
            .collect()
    }
    
    /// Get a map of property names to their indices
    pub fn get_property_map(&self) -> &HashMap<InternedString, usize> {
        &self.property_map
    }
}