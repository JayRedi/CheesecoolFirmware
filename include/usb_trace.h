#ifndef USB_TRACE_H
#define USB_TRACE_H

#include <stdint.h>
#include <stdbool.h>

#define USB_TRACE_CAPACITY 64U

typedef enum {
    USB_TRACE_IRQ_ENTRY = 1,
    USB_TRACE_TRANSFER_BEFORE_HANDLE,
    USB_TRACE_TRANSFER_AFTER_CLEAR,
    USB_TRACE_BUS_RST_BEFORE_HANDLE,
    USB_TRACE_BUS_RST_AFTER_CLEAR,
    USB_TRACE_SUSPEND_BEFORE_HANDLE,
    USB_TRACE_SUSPEND_AFTER_CLEAR,
    USB_TRACE_IRQ_EXIT
} usb_trace_event_t;

typedef struct __attribute__((packed)) {
    uint32_t sequence;
    uint8_t event;
    uint8_t int_fg;
    uint8_t int_st;
    uint8_t mis_st;
    uint8_t dev_addr;
    uint8_t ep0_ctrl_h;
    uint16_t ep0_tx_len;
    uint32_t ep0_dma;
    uint8_t setup[8];
} usb_trace_entry_t;

_Static_assert(sizeof(usb_trace_entry_t) == 24U, "usb trace entry must be 24 bytes");

extern volatile usb_trace_entry_t usb_trace_buffer[USB_TRACE_CAPACITY];
extern volatile uint8_t usb_trace_count;
extern volatile bool usb_trace_overflow;
extern volatile bool usb_trace_frozen;

void usb_trace_init(void);
void usb_trace_log(uint8_t event);
void usb_trace_log_setup(uint8_t event, const uint8_t setup[8]);
void usb_trace_freeze(void);

#endif
