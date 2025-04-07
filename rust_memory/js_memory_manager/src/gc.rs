use crate::object::{JSObject, JSObjectHandle, JSObjectType};
use libc::{c_char, c_void};
use parking_lot::{Mutex, RwLock};
use std::collections::{HashMap, HashSet, VecDeque};
use std::ffi::{CStr, CString};
use std::mem;
use std::sync::{Arc, Weak};
use std::time::{Duration, Instant};

/// Configuration options for the garbage collector
#[derive(Debug, Clone)]
pub struct GCConfiguration {
    /// Size threshold (KB) for young generation collection
    pub young_gen_threshold_kb: usize,
    /// Size threshold (KB) for old generation collection
    pub old_gen_threshold_kb: usize,
    /// Maximum pause time in milliseconds
    pub max_pause_ms: u64,
    /// Whether to use incremental collection
    pub incremental: bool,
    /// Whether to print verbose GC debugging information
    pub verbose: bool,
}

impl Default for GCConfiguration {
    fn default() -> Self {
        Self {
            young_gen_threshold_kb: 256,   // 256KB
            old_gen_threshold_kb: 4096,    // 4MB
            max_pause_ms: 10,              // 10ms
            incremental: true,
            verbose: false,
        }
    }
}

/// Statistics about garbage collection
#[derive(Debug, Clone, Copy)]
pub struct GCStatistics {
    /// Total number of allocations
    pub allocation_count: usize,
    /// Total number of collections performed
    pub collection_count: usize,
    /// Total number of objects freed
    pub objects_freed: usize,
    /// Current size of young generation in bytes
    pub young_generation_size: usize,
    /// Current size of old generation in bytes
    pub old_generation_size: usize,
}

impl Default for GCStatistics {
    fn default() -> Self {
        Self {
            allocation_count: 0,
            collection_count: 0,
            objects_freed: 0,
            young_generation_size: 0,
            old_generation_size: 0,
        }
    }
}

/// Generational garbage collector for JavaScript objects
pub struct GarbageCollector {
    /// Young generation objects (recently allocated)
    young_generation: Mutex<Vec<Arc<JSObject>>>,
    
    /// Old generation objects (survived several collections)
    old_generation: Mutex<Vec<Arc<JSObject>>>,
    
    /// Objects that should never be collected (roots)
    roots: Mutex<HashSet<*const JSObject>>,
    
    /// Configuration options
    config: RwLock<GCConfiguration>,
    
    /// Collection statistics
    stats: RwLock<GCStatistics>,
    
    /// Whether the GC is currently running a collection
    collecting: Mutex<bool>,
}

impl GarbageCollector {
    /// Create a new garbage collector with default configuration
    pub fn new() -> Arc<Self> {
        Arc::new(Self {
            young_generation: Mutex::new(Vec::new()),
            old_generation: Mutex::new(Vec::new()),
            roots: Mutex::new(HashSet::new()),
            config: RwLock::new(GCConfiguration::default()),
            stats: RwLock::new(GCStatistics::default()),
            collecting: Mutex::new(false),
        })
    }
    
    /// Update the GC configuration
    pub fn configure(&self, config: GCConfiguration) {
        let mut current_config = self.config.write();
        *current_config = config;
    }
    
    /// Get current statistics
    pub fn statistics(&self) -> GCStatistics {
        *self.stats.read()
    }
    
    /// Create a new JavaScript object and add it to the young generation
    pub fn create_object(&self, obj_type: JSObjectType) -> JSObjectHandle {
        // Create the new object
        let obj = JSObject::new(obj_type);
        
        // Track the object in the young generation
        {
            let mut young = self.young_generation.lock();
            young.push(obj.clone());
            
            // Update allocation statistics
            let mut stats = self.stats.write();
            stats.allocation_count += 1;
            stats.young_generation_size += self.estimate_object_size(&obj);
            
            // Check if we need to trigger a young generation collection
            if stats.young_generation_size > self.config.read().young_gen_threshold_kb * 1024 {
                // Drop the lock before collecting
                drop(stats);
                drop(young);
                self.collect_young();
            }
        }
        
        JSObjectHandle { ptr: obj }
    }
    
    /// Add a root object that shouldn't be collected
    pub fn add_root(&self, ptr: *mut JSObject) {
        if !ptr.is_null() {
            let mut roots = self.roots.lock();
            roots.insert(ptr as *const JSObject);
        }
    }
    
    /// Remove a root object
    pub fn remove_root(&self, ptr: *mut JSObject) {
        if !ptr.is_null() {
            let mut roots = self.roots.lock();
            roots.remove(&(ptr as *const JSObject));
        }
    }
    
    /// Trigger a garbage collection
    pub fn collect(&self) {
        // Make sure we're not already collecting
        let mut collecting = self.collecting.lock();
        if *collecting {
            return;
        }
        *collecting = true;
        
        // Collect both generations
        self.collect_young();
        self.collect_old();
        
        // Update stats
        let mut stats = self.stats.write();
        stats.collection_count += 1;
        
        // Reset collection flag
        *collecting = false;
    }
    
    /// Collect only the young generation (minor collection)
    fn collect_young(&self) {
        let start_time = Instant::now();
        let config = self.config.read();
        
        if config.verbose {
            println!("Starting young generation collection");
        }
        
        // Mark phase - mark all reachable objects
        self.mark_roots();
        
        // Sweep phase for young generation
        let mut survivors = Vec::new();
        let mut freed = 0;
        let mut young_gen_size = 0;
        
        {
            let mut young = self.young_generation.lock();
            
            // Process each object
            for obj in young.drain(..) {
                if obj.is_marked() {
                    // Object is alive, unmark and either promote or keep in young gen
                    obj.unmark();
                    
                    // Promote to old generation after surviving several collections
                    // This is a simplification - in a real GC we would track ages
                    if Arc::strong_count(&obj) > 2 {
                        let mut old = self.old_generation.lock();
                        old.push(obj);
                    } else {
                        survivors.push(obj);
                    }
                } else {
                    // Object is unreachable, will be dropped
                    freed += 1;
                }
            }
            
            // Put survivors back in young generation
            *young = survivors;
            
            // Calculate new size
            for obj in &*young {
                young_gen_size += self.estimate_object_size(obj);
            }
        }
        
        // Update statistics
        let mut stats = self.stats.write();
        stats.objects_freed += freed;
        stats.young_generation_size = young_gen_size;
        
        if config.verbose {
            println!("Young generation collection completed in {}ms, freed {} objects",
                     start_time.elapsed().as_millis(), freed);
        }
    }
    
    /// Collect the old generation (major collection)
    fn collect_old(&self) {
        let start_time = Instant::now();
        let config = self.config.read();
        
        // Check if we need to run a major collection based on old gen size
        {
            let stats = self.stats.read();
            if stats.old_generation_size < config.old_gen_threshold_kb * 1024 {
                return;
            }
        }
        
        if config.verbose {
            println!("Starting old generation collection");
        }
        
        // Mark phase - mark all reachable objects
        // (roots should already be marked by young gen collection)
        
        // Sweep phase for old generation
        let mut survivors = Vec::new();
        let mut freed = 0;
        let mut old_gen_size = 0;
        
        {
            let mut old = self.old_generation.lock();
            
            // Process each object
            for obj in old.drain(..) {
                if obj.is_marked() {
                    // Object is alive, unmark and keep in old gen
                    obj.unmark();
                    survivors.push(obj);
                } else {
                    // Object is unreachable, will be dropped
                    freed += 1;
                }
            }
            
            // Put survivors back in old generation
            *old = survivors;
            
            // Calculate new size
            for obj in &*old {
                old_gen_size += self.estimate_object_size(obj);
            }
        }
        
        // Update statistics
        let mut stats = self.stats.write();
        stats.objects_freed += freed;
        stats.old_generation_size = old_gen_size;
        
        if config.verbose {
            println!("Old generation collection completed in {}ms, freed {} objects",
                     start_time.elapsed().as_millis(), freed);
        }
    }
    
    /// Mark all root objects and their references
    fn mark_roots(&self) {
        // Get local copies of roots to avoid holding lock during marking
        let roots: Vec<*const JSObject> = {
            let roots = self.roots.lock();
            roots.iter().cloned().collect()
        };
        
        // Mark each root object
        for &root_ptr in &roots {
            // Safety: The root pointers should be valid JSObjects
            let obj = unsafe { &*(root_ptr) };
            obj.mark();
        }
    }
    
    /// Estimate the memory size of an object
    fn estimate_object_size(&self, obj: &JSObject) -> usize {
        // Base size of the object
        let mut size = mem::size_of::<JSObject>();
        
        // Add size of properties
        let inner = obj.inner.read();
        let properties = &inner.properties;
        size += properties.len() * (mem::size_of::<String>() + mem::size_of::<JSObject>());
        
        // Approximate size of property keys and values
        for (key, value) in properties {
            size += key.len();
            match value {
                crate::object::JSValue::String(s) => {
                    size += s.len();
                }
                _ => {
                    size += mem::size_of::<crate::object::JSValue>();
                }
            }
        }
        
        size
    }
}