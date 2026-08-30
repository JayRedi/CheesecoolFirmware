#include <ch32x035.h>
#include "system_status.h"
#include "fan_tach.h"
#include "fan_controller.h"
#include "power_monitor.h"
#include "failsafe.h"
#include "usb_device.h"
#include "debug_test.h"

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

int main(void) {
    SystemCoreClockUpdate();
    system_status_init();
    fan_controller_init(); fan_tach_init(); power_monitor_init(); failsafe_init();
#if FEATURE_USB_ENUM_TRACE
    usb_enum_trace_start();
#elif FEATURE_USB_ATTACH_DIAG || FEATURE_USB_SETUP_DIAG || FEATURE_USB_DEVICE_DESC_DIAG || FEATURE_USB_SET_ADDRESS_DIAG || FEATURE_USB_CONFIG_DIAG || FEATURE_USB_POST_ADDRESS_DIAG || FEATURE_USB_POST_ADDRESS_FIX_DIAG || FEATURE_USB_CONFIG_LENGTH_DIAG || FEATURE_USB_CONFIG_IN_DIAG || FEATURE_USB_CONFIG_RUNTIME_DIAG || FEATURE_USB_BINARY_WLENGTH9_DIAG || FEATURE_USB_BINARY_CONFIG_SEEN_DIAG || FEATURE_USB_BINARY_POST_ADDRESS_SETUP_DIAG || FEATURE_USB_BINARY_POST_ADDRESS_GET_DESCRIPTOR_DIAG || FEATURE_USB_BINARY_POST_ADDRESS_CONFIG_TYPE_DIAG || FEATURE_USB_BINARY_POST_ADDRESS_STRING_TYPE_DIAG || FEATURE_USB_RAM_TRACE_DIAG
    usb_device_init();
    usb_attach_diag_start();
#else
    debug_test_init();
    usb_device_init();
#endif
    while (1) { usb_device_task(); usb_enum_trace_task(); fan_tach_update(); fan_controller_update(); power_monitor_update();
#if !FEATURE_USB_ENUM_TRACE && !FEATURE_USB_ATTACH_DIAG && !FEATURE_USB_SETUP_DIAG && !FEATURE_USB_DEVICE_DESC_DIAG && !FEATURE_USB_SET_ADDRESS_DIAG && !FEATURE_USB_CONFIG_DIAG && !FEATURE_USB_POST_ADDRESS_DIAG && !FEATURE_USB_POST_ADDRESS_FIX_DIAG && !FEATURE_USB_CONFIG_LENGTH_DIAG && !FEATURE_USB_CONFIG_IN_DIAG && !FEATURE_USB_CONFIG_RUNTIME_DIAG && !FEATURE_USB_BINARY_WLENGTH9_DIAG && !FEATURE_USB_BINARY_CONFIG_SEEN_DIAG && !FEATURE_USB_BINARY_POST_ADDRESS_SETUP_DIAG && !FEATURE_USB_BINARY_POST_ADDRESS_GET_DESCRIPTOR_DIAG && !FEATURE_USB_BINARY_POST_ADDRESS_CONFIG_TYPE_DIAG && !FEATURE_USB_BINARY_POST_ADDRESS_STRING_TYPE_DIAG && !FEATURE_USB_RAM_TRACE_DIAG
        debug_test_update();
        failsafe_update();
#endif
        system_status_update(); }
}

void NMI_Handler(void) {}
void HardFault_Handler(void) { while (1) {} }
