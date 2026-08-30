/* RAM-only diagnostic image. There is deliberately no FLASH region. */
ENTRY(_start)

__stack_bottom = 0x200047F0;
__stack_top = 0x20004FF0;
__dfu_magic_start = 0x20004FF0;
__dfu_magic_end = 0x20005000;

MEMORY { RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 0x4FF0 }

SECTIONS
{
    __ram_start = ORIGIN(RAM);
    .init : { . = ALIGN(4); KEEP(*(.init)) . = ALIGN(4); } > RAM
    .vector ALIGN(64) :
    {
        __vector_start = .; KEEP(*(.vector)); . = ALIGN(64); __vector_end = .;
    } > RAM
    .text :
    {
        __text_start = .; *(.text .text.*) *(.rodata .rodata.*)
        *(.rodata.ram_usb_descriptor)
        *(.gnu.linkonce.t.*); . = ALIGN(4); __text_end = .;
    } > RAM
    .data :
    {
        . = ALIGN(4); __data_start = .; *(.data .data.*)
        . = ALIGN(8); PROVIDE(__global_pointer$ = . + 0x800);
        *(.sdata .sdata.*); . = ALIGN(4); __data_end = .;
    } > RAM
    .bss (NOLOAD) :
    {
        . = ALIGN(4); __bss_start = .; *(.sbss .sbss.*) *(.bss .bss.*)
        *(COMMON); . = ALIGN(4); __bss_end = .;
    } > RAM
    __image_end = .;
    .usb_dma (NOLOAD) :
    {
        . = ALIGN(4); __usb_dma_start = .; KEEP(*(.usb_dma))
        . = ALIGN(4); __usb_dma_end = .;
    } > RAM
    .usb_trace (NOLOAD) :
    {
        . = ALIGN(4); __usb_trace_start = .; KEEP(*(.usb_trace))
        . = ALIGN(4); __usb_trace_end = .;
    } > RAM
    .stack 0x200047F0 (NOLOAD) :
    {
        . = ALIGN(16);
        __susrstack = ABSOLUTE(0x200047F0);
        . += 0x800;
        __eusrstack = ABSOLUTE(0x20004FF0);
    } > RAM
    /DISCARD/ : { *(.comment .note .eh_frame .eh_frame.*) }
}

ASSERT(__vector_start % 64 == 0, "RAM vector table is not 64-byte aligned")
ASSERT(__image_end <= __usb_dma_start, "RAM image overlaps USB DMA")
ASSERT(__usb_dma_end <= __stack_bottom, "USB DMA overlaps stack")
ASSERT(__usb_trace_end <= __stack_bottom, "USB trace overlaps stack")
ASSERT(__image_end <= __stack_bottom, "RAM image overlaps stack")
ASSERT(__stack_bottom < __stack_top, "invalid stack bounds")
ASSERT(__stack_top <= __dfu_magic_start, "stack overlaps DFU magic")
ASSERT(__dfu_magic_start == 0x20004FF0, "DFU magic boundary changed")
ASSERT(__dfu_magic_end == 0x20005000, "DFU magic end boundary changed")
