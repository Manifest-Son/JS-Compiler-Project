use std::env;
use std::path::PathBuf;

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let output_file = PathBuf::from(&crate_dir)
        .parent()
        .unwrap()
        .join("include")
        .join("js_memory_manager.h");

    // Create the include directory if it doesn't exist
    std::fs::create_dir_all(output_file.parent().unwrap()).unwrap();

    cbindgen::Builder::new()
        .with_crate(crate_dir)
        .with_language(cbindgen::Language::Cpp)
        .with_namespace("rust_memory")
        .with_parse_deps(true)
        .with_parse_include(&["js_memory_manager"])
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(output_file);

    // Tell Cargo to rerun this build script if the wrapper changes
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/gc.rs");
    println!("cargo:rerun-if-changed=src/object.rs");
    println!("cargo:rerun-if-changed=src/ffi.rs");
}