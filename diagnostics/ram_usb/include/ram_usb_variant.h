#ifndef RAM_USB_VARIANT_H
#define RAM_USB_VARIANT_H
#ifndef USB_IRQ_BUS_RESET_FIRST
#define USB_IRQ_BUS_RESET_FIRST 0
#endif
#if (USB_IRQ_BUS_RESET_FIRST != 0) && (USB_IRQ_BUS_RESET_FIRST != 1)
#error "USB_IRQ_BUS_RESET_FIRST must be 0 or 1"
#endif
/* 0: TRANSFER-first; 1: future BUS_RST-first. */
#endif
