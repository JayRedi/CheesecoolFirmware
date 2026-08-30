#ifndef USB_PROTOCOL_H
#define USB_PROTOCOL_H
#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"
#define PROTOCOL_VERSION 1U
typedef enum { CMD_PING=1, CMD_GET_INFO=2, CMD_SET_FAN_DUTY=3, CMD_GET_FAN_STATUS=4, CMD_FAN_ENABLE=5, CMD_FAN_DISABLE=6, CMD_KEEPALIVE=7, CMD_ENTER_DFU_LEGACY=8, CMD_GET_STATUS=9, CMD_SET_MODE=10, CMD_SET_DUTY=11, CMD_SET_CURVE=12, CMD_ENTER_DFU=13, CMD_ENTER_BOOTLOADER=CMD_ENTER_DFU_LEGACY } usb_command_t;
typedef enum { USB_STATUS_OK=0, USB_STATUS_BAD_PACKET=1, USB_STATUS_BAD_COMMAND=2, USB_STATUS_NOT_SUPPORTED=3, USB_STATUS_BAD_PARAMETER=4 } usb_status_t;
void usb_protocol_init(void); void usb_protocol_process(const uint8_t request[USB_REPORT_SIZE], uint8_t response[USB_REPORT_SIZE]);
bool usb_protocol_dfu_pending(void);
#endif
