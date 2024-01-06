# rust-python-to-bacnet-stack
Exploratory Foreign Function Interface (FFI) for Rust and Python to interface with the robust and distinguished [bacnet-stack](https://github.com/bacnet-stack/bacnet-stack) written in C created by Steve Karg.
The process thus far has been working with the example apps in the bacnet-stack and compiling them with `make clean all` per directions of the bacnet-stack.

This project contains a compiled `.so` file from the C stack from the `main.c`. The compiled `.so` containts the FFI for Rust and Python to interface with the intention of
having a bare minimum BACnet read write property interface for Rust and Python edge environment IoT apps.

# Python

See the `ctester.py` for an example of using `ctypes` to perform a read property request and have data returned from the bacnet-stack. Requires Python knowledge and typical environment setup. 

# Rust

Notes to make this work in Rust using `extern "C"` in the `main.rs` file:

1. On Linux to make thing easier do a `$ mv bacrp.so libbacrp.so`.
2. Inside the `rust_to_bacnet_stack` directory there was is a `build.rs` which is used to integrate to the `.so` file.
3. Write Your Rust Code in the src/main.rs file which uses extern "C" blocks to declare the C functions you want to call. Ensure function signatures in Rust match those in the C library.
3. Perform typical Rust build process `cargo check`, `cargo build`, and `cargo run`...

Where this does appear to be working on a test bench when using `cargo run`:
```bash
Debug: Entered bacnet_read_property
Debug: device_instance_str = 201201
Debug: object_type_str = analog-input
Debug: object_instance_str = 2
Debug: property_name_str = present-value
Debug: object_index_str = (null)
Debug: Converted arguments successfully
Result: 78.09999
Debug: Entered bacnet_read_property
Debug: device_instance_str = 201201
Debug: object_type_str = analog-input
Debug: object_instance_str = 2
Debug: property_name_str = present-value
Debug: object_index_str = 3
Debug: Converted arguments successfully
Result with index: 78.09999
```
* Further testing is required to determine the compatibility of this setup with the compiled C `.so` file and the executable file generated during the Rust build process, located within the `rust_to_bacnet_stack/target/debug/rust_to_bacnet_stack` directory. 
Is it worth considering whether relying on Cargo for long-term operation of an IoT application in an edge environment is the most advisable practice or the executable from Rust? More testing to come...
 

