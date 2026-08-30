# 系统 USB ISP Bootloader

`system_enter_bootloader()` 只使用已安装 CH32X035 noneos-sdk 提供的定义：

- `FLASH_TypeDef.BOOT_MODEKEYR`：作为 `FLASH` 的一部分，在 `Peripheral/ch32x035/inc/ch32x035.h` 中声明。
- `FLASH_TypeDef.STATR`：在同一头文件中声明。
- `Start_Mode_BOOT`（`0x00004000`）：在 `Peripheral/ch32x035/inc/ch32x035_flash.h` 中声明。
- `SystemReset_StartMode()`：在 `ch32x035_flash.h` 中声明，在 `Peripheral/ch32x035/src/ch32x035_flash.c` 中实现。SDK 实现会解锁 Flash、向 `FLASH->BOOT_MODEKEYR` 写入启动模式密钥序列、设置 `STATR[14]`，然后锁定 Flash。
- `__disable_irq()` 和 `NVIC_SystemReset()`：在 `Core/ch32x035/core_riscv.h` 中作为 inline core 函数声明。复位函数通过 `NVIC->CFGR` 写入 SDK 定义的 PFIC 复位请求寄存器。

选择启动模式前，固件通过 `fan_controller` 启用风扇并设置 100% 占空比，然后禁用中断。该命令刻意不发送响应，因为复位后执行权会转移到 System FLASH USB ISP Bootloader。
