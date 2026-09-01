ENTRY(_start)
__stack_size = 2048;
PROVIDE(_stack_size = __stack_size);
MEMORY {
    FLASH (rx) : ORIGIN = 0x00002000, LENGTH = 54K
    RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 0x5000
}
SECTIONS {
    .init : { . = ALIGN(4); KEEP(*(SORT_NONE(.init))) . = ALIGN(4); } > FLASH AT> FLASH
    .vector : { *(.vector); . = ALIGN(64); } > FLASH AT> FLASH
    .text : { . = ALIGN(4); *(.text) *(.text.*) *(.rodata) *(.rodata*) *(.gnu.linkonce.t.*) . = ALIGN(4); } > FLASH AT> FLASH
    .fini : { KEEP(*(SORT_NONE(.fini))) . = ALIGN(4); } > FLASH AT> FLASH
    PROVIDE(_etext = .);
    .preinit_array : { PROVIDE_HIDDEN(__preinit_array_start = .); KEEP(*(.preinit_array)) PROVIDE_HIDDEN(__preinit_array_end = .); } > FLASH AT> FLASH
    .init_array : { PROVIDE_HIDDEN(__init_array_start = .); KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*))) KEEP(*(.init_array)) PROVIDE_HIDDEN(__init_array_end = .); } > FLASH AT> FLASH
    .fini_array : { PROVIDE_HIDDEN(__fini_array_start = .); KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*))) KEEP(*(.fini_array)) PROVIDE_HIDDEN(__fini_array_end = .); } > FLASH AT> FLASH
    .dalign : { . = ALIGN(4); PROVIDE(_data_vma = .); } > RAM AT> FLASH
    .dlalign : { . = ALIGN(4); PROVIDE(_data_lma = .); } > FLASH AT> FLASH
    .data : { *(.data .data.*) . = ALIGN(8); PROVIDE(__global_pointer$ = . + 0x800); *(.sdata .sdata.*) . = ALIGN(4); PROVIDE(_edata = .); } > RAM AT> FLASH
    .bss : { . = ALIGN(4); PROVIDE(_sbss = .); *(.sbss*) *(.bss*) *(COMMON) . = ALIGN(4); PROVIDE(_ebss = .); } > RAM
    PROVIDE(_end = _ebss); PROVIDE(end = .);
    .stack ORIGIN(RAM) + LENGTH(RAM) - __stack_size : { PROVIDE(_heap_end = .); . = ALIGN(4); PROVIDE(_susrstack = .); . = . + __stack_size; PROVIDE(_eusrstack = .); } > RAM
}
