extern crate libc;
use libc::{c_float, c_char};
use std::{thread, time};
use std::ffi::CString;


extern "C" {
    fn bacnet_read_property(device_instance: *const c_char,
                            object_type: *const c_char,
                            object_instance: *const c_char,
                            property_name: *const c_char,
                            object_index: *const c_char) -> c_float;
}

fn call_bacnet_read_property(device_instance: &str, object_type: &str, object_instance: &str, property_name: &str, object_index: Option<&str>) -> f32 {
    let device_instance_c = CString::new(device_instance).unwrap();
    let object_type_c = CString::new(object_type).unwrap();
    let object_instance_c = CString::new(object_instance).unwrap();
    let property_name_c = CString::new(property_name).unwrap();
    let object_index_c = object_index.map(|s| CString::new(s).unwrap());

    unsafe {
        bacnet_read_property(device_instance_c.as_ptr(),
                             object_type_c.as_ptr(),
                             object_instance_c.as_ptr(),
                             property_name_c.as_ptr(),
                             object_index_c.as_ref().map_or(std::ptr::null(), |c| c.as_ptr()))
    }
}



fn main() {
    let device_instance = "201201";
    let object_type = "analog-input";
    let object_instance = "2";
    let property_name = "present-value";

    loop {
        let result = call_bacnet_read_property(device_instance, object_type, object_instance, property_name, None);
        println!("Result: {}", result);

        // Example with index
        let result_with_index = call_bacnet_read_property(device_instance, object_type, object_instance, property_name, Some("3"));
        println!("Result with index: {}", result_with_index);

        // Sleep for 60 seconds
        thread::sleep(time::Duration::from_secs(60));
    }
}

