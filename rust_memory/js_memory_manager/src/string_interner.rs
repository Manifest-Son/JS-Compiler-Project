use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use std::fmt;
use std::hash::{Hash, Hasher};
use std::ops::Deref;
use std::borrow::Borrow;
use lazy_static::lazy_static;

/// A JavaScript string that's been interned for deduplication
#[derive(Clone)]
pub struct InternedString {
    // Arc allows shared ownership of the string data
    inner: Arc<String>,
}

impl InternedString {
    /// Create a new interned string
    pub fn new(s: &str) -> Self {
        STRING_INTERNER.with(|interner| interner.intern(s))
    }
    
    /// Get the underlying string as a str slice
    pub fn as_str(&self) -> &str {
        &self.inner
    }
}

// Custom implementations for InternedString

impl PartialEq for InternedString {
    fn eq(&self, other: &Self) -> bool {
        // Since interned strings are deduplicated, 
        // we can compare their Arc pointers directly
        Arc::ptr_eq(&self.inner, &other.inner)
    }
}

impl Eq for InternedString {}

impl Hash for InternedString {
    fn hash<H: Hasher>(&self, state: &mut H) {
        // Use the address of the string as the hash
        Arc::as_ptr(&self.inner).hash(state);
    }
}

impl fmt::Debug for InternedString {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&**self.inner, f)
    }
}

impl fmt::Display for InternedString {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Display::fmt(&**self.inner, f)
    }
}

impl Deref for InternedString {
    type Target = str;
    
    fn deref(&self) -> &Self::Target {
        &self.inner
    }
}

impl Borrow<str> for InternedString {
    fn borrow(&self) -> &str {
        &self.inner
    }
}

impl AsRef<str> for InternedString {
    fn as_ref(&self) -> &str {
        &self.inner
    }
}

impl From<&str> for InternedString {
    fn from(s: &str) -> Self {
        InternedString::new(s)
    }
}

impl From<String> for InternedString {
    fn from(s: String) -> Self {
        InternedString::new(&s)
    }
}

// Actual interner implementation

/// String interner for deduplicating strings
pub struct StringInterner {
    // Map of string content to interned string references
    strings: Mutex<HashMap<String, Arc<String>>>,
}

impl StringInterner {
    /// Create a new string interner
    pub fn new() -> Self {
        Self {
            strings: Mutex::new(HashMap::new()),
        }
    }

    /// Intern a string, returning a deduplicated reference
    pub fn intern(&self, s: &str) -> InternedString {
        let mut strings = self.strings.lock().unwrap();

        if let Some(interned) = strings.get(s) {
            // String already exists, return existing reference
            InternedString { inner: Arc::clone(interned) }
        } else {
            // String doesn't exist yet, add to the interner
            let string_arc = Arc::new(s.to_string());
            strings.insert(s.to_string(), Arc::clone(&string_arc));
            InternedString { inner: string_arc }
        }
    }

    /// Get the number of unique strings in the interner
    pub fn len(&self) -> usize {
        self.strings.lock().unwrap().len()
    }

    /// Check if the interner is empty
    pub fn is_empty(&self) -> bool {
        self.strings.lock().unwrap().is_empty()
    }
}

// Global string interner
thread_local! {
    static STRING_INTERNER: StringInterner = StringInterner::new();
}

/// Get statistics about the string interner
pub fn get_interner_stats() -> (usize, usize) {
    STRING_INTERNER.with(|interner| {
        let strings = interner.strings.lock().unwrap();
        let count = strings.len();
        
        // Calculate approximate memory usage (key + value)
        let memory = strings.iter()
            .map(|(k, v)| k.len() + std::mem::size_of::<Arc<String>>())
            .sum();
        
        (count, memory)
    })
}

/// Clear the string interner (mainly for testing)
#[cfg(test)]
pub fn clear_interner() {
    STRING_INTERNER.with(|interner| {
        let mut strings = interner.strings.lock().unwrap();
        strings.clear();
    });
}