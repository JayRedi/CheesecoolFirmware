# 功能说明（冻结 V1）

本文件描述标签 `software-dfu-removed-v1` 的设备行为，不是未来功能路线图。

## 风扇 PWM

- 输出：PA0，`TIM2_CH1`，目标频率 25 kHz。
- 电气路径：PA0 → R403（100 Ω）→ Q401（2N7002K）→ `FAN_PWM_OC` → 风扇 Pin 4。
- Q401 是开漏、反相级：PA0 高时拉低 PWM 线，PA0 低时释放 PWM 线。`fan_pwm_set_duty()` 已把这个反相关系封装起来，调用方使用的 0–100% 始终表示风扇侧的逻辑 duty。
- `0%` 是标准 PWM 风扇的最小控制 duty，不等于切断 12 V 供电，不承诺物理停转。

## 转速反馈

PA1 接收风扇 Pin 3 的 `TACH`。输入带 10 kΩ 上拉到 3.3 V，固件在下降沿计数；默认按每转 2 脉冲、1 秒测量窗口计算 RPM。不同风扇的每转脉冲数必须经硬件验证后才可更改。

## USB HID 与 Protocol V1

- VID:PID：`1A86:FE01`（WCH 开发 VID/PID，不是商业 USB 身份）。
- 产品字符串：`CheeseCool USB HID`。
- Full-Speed HID：EP0 控制端点、EP1 IN 和 EP2 OUT；report 固定 64 字节，末字节为 bytes 0–62 的 XOR checksum。
- 常用命令：`0x01 PING`、`0x09 GET_STATUS`、`0x0A SET_MODE`、`0x0B SET_DUTY`、`0x0C SET_CURVE`。字段与返回值见 [USB_PROTOCOL.md](USB_PROTOCOL.md)。
- `0x08`、`0x0D` 是永久 `RESERVED`，返回 `BAD_COMMAND`，不得重用。

## 控制模式与安全

`HOST_CONTROLLED` 允许主机设置 duty；`MAX` 强制 100% duty。在任一模式下，failsafe 均优先于普通输出命令。

MCU 在已有有效 Protocol V1 主机活动后，约 30 秒没有有效已知命令会进入 failsafe，默认设为 50% duty；`GET_STATUS` 也会刷新此计时器。上电而从未见到主机活动的独立保护等待为 5 分钟。`AUTO` 温度算法不在 MCU 中：macOS 客户端计算目标 duty，MCU 只执行该结果与本地 failsafe。

## Bootloader 边界

Application 不包含 software DFU 入口。Bootloader 仅保留无有效 Application 时的 DFU 防砖回退；使用限制见 [BOOTLOADER.md](BOOTLOADER.md)。
