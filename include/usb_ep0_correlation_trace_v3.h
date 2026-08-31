#ifndef USB_EP0_CORRELATION_TRACE_V3_H
#define USB_EP0_CORRELATION_TRACE_V3_H

#include <stdint.h>

#define USB_EP0_CORR_TRACE_V3_MAGIC 0x33435045UL
#define USB_EP0_CORR_TRACE_V3_VERSION 3U
#define USB_EP0_CORR_TRACE_V3_CAPACITY 64U

typedef enum {
    USB_EP0_CORR_EVENT_IRQ_ENTRY = 1,
    USB_EP0_CORR_EVENT_TRANSFER_BRANCH_ENTERED,
    USB_EP0_CORR_EVENT_SETUP_TOKEN_BRANCH_ENTERED,
    USB_EP0_CORR_EVENT_SETUP_EP0_ACCEPTED,
    USB_EP0_CORR_EVENT_SETUP_COUNT_INCREMENTED,
    USB_EP0_CORR_EVENT_CONTROL_SETUP_ENTRY,
    USB_EP0_CORR_EVENT_BUS_RST_BRANCH_ENTERED
} usb_ep0_corr_event_v3_t;

typedef enum {
    USB_EP0_CORR_DISPATCH_NONE = 0,
    USB_EP0_CORR_DISPATCH_TRANSFER,
    USB_EP0_CORR_DISPATCH_BUS_RST,
    USB_EP0_CORR_DISPATCH_SUSPEND,
    USB_EP0_CORR_DISPATCH_FALLBACK
} usb_ep0_corr_dispatch_v3_t;

enum {
    USB_EP0_CORR_DECODE_TRANSFER = 0x01,
    USB_EP0_CORR_DECODE_BUS_RST = 0x02,
    USB_EP0_CORR_DECODE_SUSPEND = 0x04,
    USB_EP0_CORR_DECODE_TOG_OK = 0x08,
    USB_EP0_CORR_DECODE_SETUP_ACT = 0x10,
    USB_EP0_CORR_DECODE_FIFO_OV = 0x20
};

typedef struct __attribute__((packed)) {
    uint16_t sequence;
    uint8_t event;
    uint8_t dispatch;
    uint8_t int_fg;
    uint8_t int_st;
    uint8_t mis_st;
    uint8_t dev_addr;
    uint16_t rx_len;
    uint8_t usb_configuration;
    uint8_t decoded_flags;
    uint8_t token;
    uint8_t endpoint;
    uint16_t setup_count;
    uint16_t bus_reset_count;
    uint8_t ep0[8];
    uint8_t reserved[6];
} usb_ep0_corr_entry_v3_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t header_size;
    uint16_t entry_size;
    uint16_t capacity;
    uint16_t count;
    uint16_t next_sequence;
    uint8_t frozen;
    uint8_t overflow;
    uint8_t reserved[6];
} usb_ep0_corr_header_v3_t;

typedef struct __attribute__((packed)) {
    usb_ep0_corr_header_v3_t header;
    usb_ep0_corr_entry_v3_t entries[USB_EP0_CORR_TRACE_V3_CAPACITY];
} usb_ep0_corr_storage_v3_t;

_Static_assert(sizeof(usb_ep0_corr_header_v3_t) == 24U,
               "EP0 correlation trace V3 header must be 24 bytes");
_Static_assert(sizeof(usb_ep0_corr_entry_v3_t) == 32U,
               "EP0 correlation trace V3 entry must be 32 bytes");
_Static_assert(sizeof(usb_ep0_corr_storage_v3_t) == 2072U,
               "EP0 correlation trace V3 storage must be 2072 bytes");

extern volatile usb_ep0_corr_storage_v3_t usb_ep0_corr_trace_v3;

void usb_ep0_corr_trace_v3_init(void);
void usb_ep0_corr_trace_v3_log(uint8_t event,
                               uint8_t dispatch,
                               uint8_t int_fg,
                               uint8_t int_st,
                               uint8_t usb_configuration,
                               const uint8_t ep0[8]);

#endif
