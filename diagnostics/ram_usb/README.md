# RAM USB diagnostic skeleton

This directory is isolated from the normal PlatformIO application and
bootloader. This revision is a static skeleton only: it does not implement
USB attach, USBFS register setup, endpoint handling, or a USBFS ISR.

Memory contract:

- SRAM image: `0x20000000` upward
- usable RAM end: `0x20004FEF`
- stack: `0x200047F0` through `0x20004FEF`
- DFU magic reservation: `0x20004FF0` through `0x20004FFF`
- vector table: linker-enforced 64-byte alignment
- `.usb_dma`: `NOLOAD`, after normal image sections and before stack

The skeleton declares one aligned 64-byte EP0 placeholder in `.usb_dma` and a
RAM descriptor placeholder in `.rodata.ram_usb_descriptor`; neither is used
for USB traffic in this revision.

The linker script has no Flash memory region and no `AT> FLASH`. `.data` is
therefore loaded directly at its RAM VMA by a future RAM image loader. Startup
clears only `.bss`; it does not copy data from Flash.

`startup_ram.S` follows the WCH startup shape: initialize `gp` and `sp`, clear
`.bss`, initialize the PFIC-related CSRs used by the current project, set
`mtvec` to the RAM vector table in fast-interrupt mode, call a RAM-resident
`SystemInit`, and enter `main` through `mepc`/`mret`. The weak `SystemInit` is
a no-op placeholder for the future RAM-resident clock implementation.

USBFS is not enabled here. A later USB phase must provide RAM-resident
`USBFS_IRQHandler` and `USBFSWakeUp_IRQHandler`, then initialize USBFS and
PFIC explicitly.

`USB_DMA_ADDR(ptr)` performs the confirmed CH32X035 conversion:
CPU SRAM address minus `0x20000000`, with bounds checking through
`ram_usb_dma_addr_checked()`.

`USB_IRQ_BUS_RESET_FIRST` defaults to `0`. It is only a configuration seam in
this skeleton; no A/B USB control flow is implemented yet. Future A/B builds
must keep compiler flags, linker, descriptors, and source identical while
changing only this macro.

Future RAM diagnostic code must not reference Flash erase/write APIs, option
byte or NVRAM writes, Application jumps, or DFU magic writes. This skeleton
contains none of those dependencies and contains no USB behavior.

Before any load or execution, inspect the ELF/map/sections and verify all
`PT_LOAD` and symbols are in `0x20000000–0x20004FEF`, the vector and all ISR
symbols are in RAM, `.usb_dma` is before the stack, the DFU magic range is
untouched, and no Flash-programming symbols are present.
