# RAM USB 诊断骨架

此目录与正常 PlatformIO Application 和 Bootloader 隔离。本版本仅是静态骨架：
不实现 USB attach、USBFS 寄存器配置、端点处理或 USBFS ISR。

内存约定：

- SRAM image: `0x20000000` upward
- usable RAM end: `0x20004FEF`
- stack: `0x200047F0` through `0x20004FEF`
- DFU magic reservation: `0x20004FF0` through `0x20004FFF`
- vector table: linker-enforced 64-byte alignment
- `.usb_dma`: `NOLOAD`, after normal image sections and before stack

骨架在 `.usb_dma` 中声明一个对齐的 64 字节 EP0 占位区，并在
`.rodata.ram_usb_descriptor` 中声明 RAM descriptor 占位区；本版本不会将二者用于 USB 流量。

链接脚本没有 Flash memory region，也没有 `AT> FLASH`。因此，未来的 RAM image loader
会直接将 `.data` 加载到其 RAM VMA。Startup 只清零 `.bss`，不会从 Flash 复制数据。

`startup_ram.S` 遵循 WCH startup 结构：初始化 `gp` 和 `sp`，清零 `.bss`，初始化当前项目使用的
PFIC 相关 CSR，在 fast-interrupt mode 下将 `mtvec` 设置为 RAM vector table，调用驻留 RAM 的
`SystemInit`，并通过 `mepc`/`mret` 进入 `main`。弱符号 `SystemInit` 是未来 RAM 时钟实现的空操作占位符。

此处未启用 USBFS。后续 USB 阶段必须提供驻留 RAM 的 `USBFS_IRQHandler` 和
`USBFSWakeUp_IRQHandler`，然后显式初始化 USBFS 和 PFIC。

`USB_DMA_ADDR(ptr)` 执行已确认的 CH32X035 转换：CPU SRAM 地址减去 `0x20000000`，并通过
`ram_usb_dma_addr_checked()` 做边界检查。

`USB_IRQ_BUS_RESET_FIRST` 默认值为 `0`。它在本骨架中仅是配置切口，目前尚未实现 A/B USB 控制流。
未来的 A/B 构建必须保持编译器选项、链接器、descriptor 和源代码完全一致，只改变此宏。

未来的 RAM 诊断代码不得引用 Flash 擦除/写入 API、option byte 或 NVRAM 写入、Application 跳转或
DFU magic 写入。本骨架不含这些依赖，也不包含 USB 行为。

在任何加载或执行前，检查 ELF/map/sections，并验证所有 `PT_LOAD` 和 symbol 均位于
`0x20000000–0x20004FEF`；vector 和所有 ISR symbol 均位于 RAM；`.usb_dma` 位于 stack 之前；
DFU magic 范围未被触碰；且不存在 Flash-programming symbol。
