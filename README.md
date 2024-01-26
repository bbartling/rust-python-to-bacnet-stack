# rust-python-to-bacnet-stack
Exploratory Foreign Function Interface (FFI) ran on Linux to interface with the [bacnet-stack](https://github.com/bacnet-stack/bacnet-stack) written in C created by Steve Karg.
The repo contains a compiled `.so` file which is based off of the bacnet-stack readprop sample app along with some FFI features written in C.

# Python
This contains a hard coded example doing a read request to an MSTP device on a test bench address 2 on network 12345. 
```bash
$ python app.py
```

# Rust
Modify this below as necessary for your device for the operating system needs to know where to find `libbacrp.so` at runtime.
```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/bbartling/rust-python-to-bacnet-stack/rust/lib
```
Run the compiled program:
```bash
cd /home/bbartling/rust-python-to-bacnet-stack/rust
./target/debug/rust_to_bacnet_stack
```



