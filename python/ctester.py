import ctypes

# Load the shared library
lib = ctypes.CDLL("./bacrp.so")

# Specify the return type as c_float for the C function
lib.bacnet_read_property.restype = ctypes.c_float

def call_bacnet_read_property(device_instance, 
                              object_type, 
                              object_instance, 
                              property_name, 
                              object_index=None):
    
    # Encode string arguments as UTF-8 and call the C function
    result = lib.bacnet_read_property(
        device_instance.encode("utf-8"),
        object_type.encode("utf-8"),
        object_instance.encode("utf-8"),
        property_name.encode("utf-8"),
        str(object_index).encode("utf-8") if object_index is not None else None
    )
    return result

# Example usage of the function
device_instance = "201201"
object_type = "analog-input"
object_instance = "2"
property_name = "present-value"

result = call_bacnet_read_property(device_instance, object_type, object_instance, property_name)
print(result)  # Should call BACnet request without index

# Example with index
result_with_index = call_bacnet_read_property(device_instance, object_type, object_instance, property_name, 3)
print(result_with_index)  # Should call BACnet request with index
