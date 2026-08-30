#ifndef RAM_USB_DMA_H
#define RAM_USB_DMA_H
#include <stdint.h>
#define RAM_USB_SRAM_BASE 0x20000000u
#define RAM_USB_SRAM_LIMIT 0x20004FEFu
/* CH32X035 USBFS DMA registers use an SRAM offset, not a CPU address. */
static inline uint16_t ram_usb_dma_addr_checked(const void *ptr)
{
    uintptr_t address = (uintptr_t)ptr;
    if (address < RAM_USB_SRAM_BASE || address > RAM_USB_SRAM_LIMIT)
        return UINT16_MAX;
    return (uint16_t)(address - RAM_USB_SRAM_BASE);
}
#define USB_DMA_ADDR(ptr) ram_usb_dma_addr_checked((ptr))
#endif
