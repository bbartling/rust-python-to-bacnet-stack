use std::env;

fn main() {
    println!("cargo:rustc-link-search=native={}", env::current_dir().unwrap().join("lib").display());
    println!("cargo:rustc-link-lib=bacrp"); // Link to 'libbacrp.so'
}
