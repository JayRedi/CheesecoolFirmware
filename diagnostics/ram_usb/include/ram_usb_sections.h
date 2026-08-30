#ifndef RAM_USB_SECTIONS_H
#define RAM_USB_SECTIONS_H

#define RAM_USB_DMA_OBJECT __attribute__((section(".usb_dma"), aligned(4)))
#define RAM_USB_DESCRIPTOR_OBJECT __attribute__((section(".rodata.ram_usb_descriptor")))
#define RAM_USB_TRACE_OBJECT __attribute__((section(".usb_trace"), aligned(4)))

#endif
