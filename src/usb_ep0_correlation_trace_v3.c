#include "app_config.h"

#if FEATURE_USB_EP0_CORRELATION_TRACE_V3

#include <ch32x035.h>
#include "ch32x035_usb.h"
#include "usb_device.h"
#include "usb_ep0_correlation_trace_v3.h"

volatile usb_ep0_corr_storage_v3_t usb_ep0_corr_trace_v3;

void usb_ep0_corr_trace_v3_init(void)
{
    volatile usb_ep0_corr_header_v3_t *header=&usb_ep0_corr_trace_v3.header;
    header->magic=USB_EP0_CORR_TRACE_V3_MAGIC;
    header->version=USB_EP0_CORR_TRACE_V3_VERSION;
    header->header_size=sizeof(usb_ep0_corr_header_v3_t);
    header->entry_size=sizeof(usb_ep0_corr_entry_v3_t);
    header->capacity=USB_EP0_CORR_TRACE_V3_CAPACITY;
    header->count=0U;
    header->next_sequence=0U;
    header->frozen=0U;
    header->overflow=0U;
}

void usb_ep0_corr_trace_v3_log(uint8_t event,
                               uint8_t dispatch,
                               uint8_t int_fg,
                               uint8_t int_st,
                               uint8_t usb_configuration,
                               const uint8_t ep0[8])
{
    volatile usb_ep0_corr_header_v3_t *header=&usb_ep0_corr_trace_v3.header;
    volatile usb_ep0_corr_entry_v3_t *entry;
    uint16_t index;

    if (header->frozen != 0U) return;
    index=header->count;
    if (index >= USB_EP0_CORR_TRACE_V3_CAPACITY) {
        header->overflow=1U;
        header->frozen=1U;
        return;
    }

    entry=&usb_ep0_corr_trace_v3.entries[index];
    entry->sequence=header->next_sequence;
    entry->event=event;
    entry->dispatch=dispatch;
    entry->int_fg=int_fg;
    entry->int_st=int_st;
    entry->mis_st=USBFSD->MIS_ST;
    entry->dev_addr=USBFSD->DEV_ADDR;
    entry->rx_len=USBFSD->RX_LEN;
    entry->usb_configuration=usb_configuration;
    entry->decoded_flags=0U;
    if ((int_fg&USBFS_UIF_TRANSFER) != 0U) entry->decoded_flags|=USB_EP0_CORR_DECODE_TRANSFER;
    if ((int_fg&USBFS_UIF_BUS_RST) != 0U) entry->decoded_flags|=USB_EP0_CORR_DECODE_BUS_RST;
    if ((int_fg&USBFS_UIF_SUSPEND) != 0U) entry->decoded_flags|=USB_EP0_CORR_DECODE_SUSPEND;
    if ((int_fg&USBFS_UIF_FIFO_OV) != 0U) entry->decoded_flags|=USB_EP0_CORR_DECODE_FIFO_OV;
    if ((int_st&USBFS_UIS_TOG_OK) != 0U) entry->decoded_flags|=USB_EP0_CORR_DECODE_TOG_OK;
    if ((int_st&USBFS_SETUP_ACT) != 0U) entry->decoded_flags|=USB_EP0_CORR_DECODE_SETUP_ACT;
    entry->token=int_st&USBFS_UIS_TOKEN_MASK;
    entry->endpoint=int_st&USBFS_UIS_ENDP_MASK;
    entry->setup_count=(uint16_t)usb_setup_count;
    entry->bus_reset_count=(uint16_t)usb_bus_reset_count;
    entry->ep0[0]=ep0[0]; entry->ep0[1]=ep0[1];
    entry->ep0[2]=ep0[2]; entry->ep0[3]=ep0[3];
    entry->ep0[4]=ep0[4]; entry->ep0[5]=ep0[5];
    entry->ep0[6]=ep0[6]; entry->ep0[7]=ep0[7];
    entry->reserved[0]=0U; entry->reserved[1]=0U; entry->reserved[2]=0U;
    entry->reserved[3]=0U; entry->reserved[4]=0U; entry->reserved[5]=0U;

    header->next_sequence++;
    header->count=(uint16_t)(index+1U);
    if (header->count >= USB_EP0_CORR_TRACE_V3_CAPACITY) {
        header->overflow=1U;
        header->frozen=1U;
    }
}

#endif
