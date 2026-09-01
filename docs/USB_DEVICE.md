# CheeseCool USB HID 设备

Application 使用 CH32X035 USBFS 外设的 Full-Speed device mode。实现遵循已安装 WCH SDK 以及项目已验证 DFU 设备所使用的 register/DMA 模型。

- 开发用 VID:PID：`1a86:fe01`（WCH 开发身份，不是商业 CheeseCool VID/PID）。
- 控制端点：EP0，64 字节。
- HID 中断 IN：EP1，64 字节，间隔 1 ms。
- HID 中断 OUT：EP2，64 字节，间隔 1 ms。
- USB 引脚：SDK/开发板 USBFS 引脚 PC16/PC17。
- USB IRQ：`USBFS_IRQHandler`，使用 `interrupt("WCH-Interrupt-fast")`，按照 WCH 编译器 ABI 通过 `mret` 返回。

中断处理器只搬运端点数据并推进 USB 控制状态。协议解析及风扇动作在非阻塞主循环的 `usb_device_task()` 中执行；Application 不含 DFU 动作。USB 初始化不会触碰 SysTick、TIM2、PA0 或风扇 PWM 实现。

Host 辅助工具是 `tools/cheesecoolctl/cheesecoolctl.py`，需要 Python `hidapi`。
