#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h> /* for time */
#if (__STDC_VERSION__ >= 199901L) && defined(__STDC_ISO_10646__)
#include <locale.h>
#endif

#define PRINT_ENABLED 1

#include "bacnet/bacdef.h"
#include "bacnet/config.h"
#include "bacnet/bactext.h"
#include "bacnet/bacerror.h"
#include "bacnet/iam.h"
#include "bacnet/arf.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/basic/object/device.h"
#include "bacport.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/whois.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/dlenv.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* the invoke id is needed to filter incoming messages */
static uint8_t Request_Invoke_ID = 0;
static BACNET_ADDRESS Target_Address;
static bool Error_Detected = false;

static int32_t Target_Object_Index = BACNET_ARRAY_ALL;

BACNET_APPLICATION_DATA_VALUE global_read_request_value;


/* Function prototypes */
static void Init_Service_Handlers(void);
static void MyErrorHandler(BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code);
static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server);
static void MyRejectHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason);
static void My_Read_Property_Ack_Handler(uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data);

static float read_bacnet_property(uint32_t Target_Device_Object_Instance,
    BACNET_OBJECT_TYPE Target_Object_Type,
    uint32_t Target_Object_Instance,
    BACNET_PROPERTY_ID Target_Object_Property,
    int Target_Object_Index);

/* Exposed function for Python ctypes */
__attribute__((visibility("default"))) float bacnet_read_property(
    const char *device_instance_str,
    const char *object_type_str,
    const char *object_instance_str,
    const char *property_name_str,
    const char *object_index_str)
{
    printf("Debug: Entered bacnet_read_property\n");
    printf("Debug: device_instance_str = %s\n", device_instance_str);
    printf("Debug: object_type_str = %s\n", object_type_str);
    printf("Debug: object_instance_str = %s\n", object_instance_str);
    printf("Debug: property_name_str = %s\n", property_name_str);
    printf("Debug: object_index_str = %s\n", object_index_str);

    uint32_t device_instance = strtoul(device_instance_str, NULL, 0);
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance = strtoul(object_instance_str, NULL, 0);
    BACNET_PROPERTY_ID property_id;
    int Target_Object_Index = BACNET_ARRAY_ALL;

    /* Convert string to BACNET_OBJECT_TYPE */
    if (!bactext_object_type_strtol(object_type_str, &object_type)) {
        fprintf(stderr, "Invalid object type: %s\n", object_type_str);
        return -1.0f;
    }

    /* Convert string to BACNET_PROPERTY_ID */
    if (!bactext_property_strtol(property_name_str, &property_id)) {
        fprintf(stderr, "Invalid property id: %s\n", property_name_str);
        return -1.0f;
    }

    if (object_index_str != NULL && strlen(object_index_str) > 0) {
        Target_Object_Index = strtol(object_index_str, NULL, 0);
    }

    printf("Debug: Converted arguments successfully\n");

    return read_bacnet_property(device_instance, object_type, object_instance,
        property_id, Target_Object_Index);
}

static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, My_Read_Property_Ack_Handler);
    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_READ_PROPERTY, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void MyErrorHandler(BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("BACnet Error: %s: %s\n",
            bactext_error_class_name((int)error_class),
            bactext_error_code_name((int)error_code));
        Error_Detected = true;
    }
}

static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    (void)server;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf(
            "BACnet Abort: %s\n", bactext_abort_reason_name((int)abort_reason));
        Error_Detected = true;
    }
}

static void MyRejectHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("BACnet Reject: %s\n",
            bactext_reject_reason_name((int)reject_reason));
        Error_Detected = true;
    }
}

static void My_Read_Property_Ack_Handler(uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    int len = 0;
    BACNET_READ_PROPERTY_DATA rp_data;
    BACNET_APPLICATION_DATA_VALUE value;

    if (address_match(&Target_Address, src) &&
        (service_data->invoke_id == Request_Invoke_ID)) {
        len = rp_ack_decode_service_request(service_request, service_len, &rp_data);
        if (len < 0) {
            printf("<decode failed!>\n");
            Error_Detected = true;  /* Set error flag if decode fails */
        } else {
            /* Decode application data */
            len = bacapp_decode_application_data(rp_data.application_data,
                                                 (uint8_t)rp_data.application_data_len,
                                                 &value);
            if (len > 0) {
                global_read_request_value = value;  /* Store the value in the global variable */
                printf("Property value received and stored.\n");
            } else {
                printf("No valid data in the response.\n");
                Error_Detected = true;  /* Set error flag if no valid data */
            }
        }
    }
}

static float read_bacnet_property(uint32_t Target_Device_Object_Instance,
                                  BACNET_OBJECT_TYPE Target_Object_Type,
                                  uint32_t Target_Object_Instance,
                                  BACNET_PROPERTY_ID Target_Object_Property,
                                  int Target_Object_Index) {
    /* Initialization */
    BACNET_ADDRESS src = {0};
    uint16_t pdu_len = 0;
    unsigned timeout = 100; /* milliseconds */
    unsigned max_apdu = 0;
    time_t elapsed_seconds = 0, last_seconds = 0, current_seconds = 0, timeout_seconds = 0;
    bool found = false;
    bool Error_Detected = false;

    /* Configure timeout values */
    last_seconds = time(NULL);
    timeout_seconds = (apdu_timeout() / 1000) * apdu_retries();

    /* Initialize address, service handlers, and environment */
    address_init();
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    dlenv_init();

    /* Try to bind with the device */
    found = address_bind_request(Target_Device_Object_Instance, &max_apdu, &Target_Address);
    if (!found) {
        Send_WhoIs(Target_Device_Object_Instance, Target_Device_Object_Instance);
    }

    /* Main operational loop */
    for (;;) {
        /* Time management */
        current_seconds = time(NULL);
        if (current_seconds != last_seconds) {
            tsm_timer_milliseconds((uint16_t)((current_seconds - last_seconds) * 1000));
        }

        /* Error check */
        if (Error_Detected) {
            break;
        }

        /* Device binding or request sending */
        if (!found) {
            found = address_bind_request(Target_Device_Object_Instance, &max_apdu, &Target_Address);
        }
        if (found) {
            if (Request_Invoke_ID == 0) {
                Request_Invoke_ID = Send_Read_Property_Request(Target_Device_Object_Instance,
                                                               Target_Object_Type, Target_Object_Instance,
                                                               Target_Object_Property, Target_Object_Index);
            } else if (tsm_invoke_id_free(Request_Invoke_ID)) {
                break;
            } else if (tsm_invoke_id_failed(Request_Invoke_ID)) {
                fprintf(stderr, "Error: TSM Timeout!\n");
                tsm_free_invoke_id(Request_Invoke_ID);
                Error_Detected = true;
                break;
            }
        } else {
            elapsed_seconds += (current_seconds - last_seconds);
            if (elapsed_seconds > timeout_seconds) {
                printf("Error: APDU Timeout!\n");
                Error_Detected = true;
                break; /* Device discovery timeout */
            }
        }

        /* Receiving and processing the PDU */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }

        /* Update last time check */
        last_seconds = current_seconds;
    }

    /* Return based on error detection */
    if (Error_Detected) {
        return 1.0f;
    }

    if (global_read_request_value.tag == BACNET_APPLICATION_TAG_REAL) {
        return global_read_request_value.type.Real; 
    } else {
        /* TODO: Handle cases where the received data is not a float  */
        return -1.0f;
    }
}


int main(void)
{
    return 0;
}