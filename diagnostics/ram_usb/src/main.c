#include <stdint.h>
#include "ram_usb_variant.h"
#include "ram_usb_sections.h"
volatile uint32_t ram_usb_skeleton_alive;
volatile uint8_t ram_usb_variant_value = USB_IRQ_BUS_RESET_FIRST;
volatile uint32_t ram_usb_trace_count;
volatile uint32_t ram_usb_trace_overflow;
RAM_USB_DMA_OBJECT volatile uint8_t ram_usb_ep0_placeholder[64];
RAM_USB_DESCRIPTOR_OBJECT const uint8_t ram_usb_descriptor_placeholder[] = { 0 };
void usb_ram_diag_init(void);
int main(void)
{
    usb_ram_diag_init();
    ram_usb_skeleton_alive = 1;
    for (;;) ram_usb_skeleton_alive++;
}
