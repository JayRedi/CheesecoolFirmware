#ifndef USB_DEVICE_H
#define USB_DEVICE_H
#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"
void usb_device_init(void); void usb_device_task(void); bool usb_device_send_report(const uint8_t report[USB_REPORT_SIZE]); bool usb_device_receive_report(uint8_t report[USB_REPORT_SIZE]);
bool usb_device_is_configured(void);
bool usb_device_tx_complete(void);
void usb_enum_trace_start(void);
void usb_enum_trace_task(void);
void usb_attach_diag_start(void);
extern volatile bool usb_bus_reset_seen;
extern volatile bool usb_setup_seen;
extern volatile bool usb_get_device_descriptor_seen;
extern volatile bool usb_device_descriptor_response_started;
extern volatile bool usb_device_descriptor_in_complete;
extern volatile bool usb_set_address_seen;
extern volatile bool usb_address_applied;
extern volatile uint8_t usb_set_address_value;
extern volatile uint8_t usb_address_applied_value;
extern volatile bool usb_get_config_descriptor_seen;
extern volatile bool usb_config_descriptor_in_complete;
extern volatile bool usb_set_configuration_seen;
extern volatile bool usb_configuration_applied;
extern volatile bool usb_ep0_rearmed_after_address;
extern volatile uint16_t usb_ep0_ctrl_after_address;
extern volatile bool usb_post_address_setup_seen;
extern volatile uint8_t post_address_bRequest;
extern volatile uint8_t post_address_bmRequestType;
extern volatile uint16_t post_address_wValue;
extern volatile uint16_t post_address_wIndex;
extern volatile uint16_t post_address_wLength;
extern volatile uint8_t usb_enum_trace_stage;
extern volatile uint8_t final_trace_stage;
extern volatile uint32_t usb_bus_reset_count;
extern volatile uint32_t usb_setup_count;
extern volatile uint32_t usb_get_device_desc_count;
extern volatile uint32_t usb_set_address_count;
extern volatile uint32_t usb_get_config_desc_count;
extern volatile uint32_t usb_get_string_desc_count;
extern volatile uint32_t usb_set_configuration_count;
#endif
