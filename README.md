# CheeseCool 固件 V1

面向 CheeseCool V1 硬件的 CH32X035F8U6 裸机风扇控制器。

使用 `pio run` 构建。Application 环境使用自定义 HID-to-DFU 上传器；其 raw binary 链接地址为 `0x2000`，不得烧录到地址零。首次安装时，通过 ROM ISP 执行一次 `wchisp flash .pio/build/ch32x035f8u6_evt_r0/cheesecool-factory.bin`。详见 [docs/FIRMWARE_UPDATE.md](docs/FIRMWARE_UPDATE.md)。

PA0 通过 TIM2 通道 1 和反相 2N7002K 级驱动 25 kHz 风扇 PWM。API 隐藏了反相关系：`fan_pwm_set_duty(0)` 表示风扇 0%，`fan_pwm_set_duty(100)` 表示风扇 100%。PA1 是转速输入，由 EXTI 测量，每转两脉冲。由于 MCU 引脚尚未确认，`PWR_FAULT` 被刻意禁用；获得原理图映射后更新 `board_config.h`。

硬件验证已确认 PA0 直接 GPIO 控制，以及 TIM2_CH1 到 PA0 的 PWM 控制。此前 TIM1_CH1/PA0 配对失败，因为 CH32X035F8U6 不存在该复用功能路径。已确认转速从 0% 到 100% 的可见变化趋势；精确的 25 kHz 频率和占空比仍需示波器验证。

复位时，栅极下拉会短暂提供全速；随后 Application 设置 0% 占空比并进入 `BOOT_WAIT`。第一次使用已知命令 ID 的有效请求会建立 Host activity；连续五分钟没有该请求时进入仅 RAM 配置的默认 50% fail-safe；已经建立 Host 连接后，连续 30 秒没有有效 activity 也会进入 fail-safe。旧版 `CMD_ENTER_BOOTLOADER` 别名请求 CheeseCool DFU hand-off；正常 Application 流程不使用未经验证的 WCH ROM ISP 路径。

Application 现在以开发专用 WCH VID:PID `1a86:fe01` 提供 64 字节 USB Full-Speed HID 设备。协议见 [docs/USB_PROTOCOL.md](docs/USB_PROTOCOL.md)，传输细节见 [docs/USB_DEVICE.md](docs/USB_DEVICE.md)。`CMD_ENTER_DFU`（兼容命令 ID 8）请求独立的 CheeseCool DFU Bootloader，而不是 WCH ROM ISP。更新流程见 [docs/FIRMWARE_UPDATE.md](docs/FIRMWARE_UPDATE.md)。

## 目录结构

`include/` 存放配置和模块 API；`src/` 存放实现；`docs/` 存放架构、协议和硬件验证资料。
