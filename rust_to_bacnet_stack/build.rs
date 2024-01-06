fn main() {
    println!("cargo:rustc-link-search=native=../"); // Link to the parent directory
    println!("cargo:rustc-link-lib=bacrp");        // Link to 'libbacrp.so'
}
