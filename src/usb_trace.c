#include "ch32x035.h"
#include "usb_trace.h"
volatile usb_trace_entry_t usb_trace_buffer[USB_TRACE_CAPACITY];
volatile uint8_t usb_trace_count;
volatile bool usb_trace_overflow;
volatile bool usb_trace_frozen;

static void usb_trace_store(uint8_t event, const uint8_t *setup)
{
    if (usb_trace_frozen) return;

    uint8_t index;
    if (usb_trace_frozen) return;
    if (usb_trace_count >= USB_TRACE_CAPACITY) {
        usb_trace_overflow=true;
        return;
    }
    index=usb_trace_count++;
    usb_trace_entry_t *entry=(usb_trace_entry_t *)&usb_trace_buffer[index];
    entry->sequence=(uint32_t)index;
    entry->event=event;
    entry->int_fg=USBFSD->INT_FG;
    entry->int_st=USBFSD->INT_ST;
    entry->mis_st=USBFSD->MIS_ST;
    entry->dev_addr=USBFSD->DEV_ADDR;
    entry->ep0_ctrl_h=USBFSD->UEP0_CTRL_H;
    entry->ep0_tx_len=USBFSD->UEP0_TX_LEN;
    entry->ep0_dma=USBFSD->UEP0_DMA;
    if (setup) {
        entry->setup[0]=setup[0]; entry->setup[1]=setup[1];
        entry->setup[2]=setup[2]; entry->setup[3]=setup[3];
        entry->setup[4]=setup[4]; entry->setup[5]=setup[5];
        entry->setup[6]=setup[6]; entry->setup[7]=setup[7];
    } else {
        entry->setup[0]=0; entry->setup[1]=0;
        entry->setup[2]=0; entry->setup[3]=0;
        entry->setup[4]=0; entry->setup[5]=0;
        entry->setup[6]=0; entry->setup[7]=0;
    }
}

void usb_trace_init(void)
{
    usb_trace_count=0;
    usb_trace_overflow=false;
    usb_trace_frozen=false;
}

void usb_trace_log(uint8_t event)
{
    usb_trace_store(event,0);
}

void usb_trace_log_setup(uint8_t event, const uint8_t setup[8])
{
    usb_trace_store(event,setup);
}

void usb_trace_freeze(void)
{
    usb_trace_frozen=true;
}
