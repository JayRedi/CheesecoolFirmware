# CheeseCool Firmware

CheeseCool Firmware 是运行在 CheeseCool V1 风扇控制板上的裸机固件。它把 macOS/Linux 主机的 USB HID 请求转换为标准 4 针 PWM 风扇控制，并把转速与安全状态返回给主机。

## 冻结的 V1 基线

- MCU：CH32X035F8U6。
- USB：Full-Speed HID，开发用 VID:PID `1A86:FE01`，产品字符串为 `CheeseCool USB HID`。
- 风扇控制：PA0 / TIM2_CH1，25 kHz PWM；外部 Q401 开漏级使物理电平反相，固件 API 已处理该反相。
- 转速：PA1 接收 TACH，按默认每转 2 脉冲计算 RPM。
- 模式：`HOST_CONTROLLED` 与 `MAX`。
- 安全：主机建立有效 Protocol V1 活动后约 30 秒无有效活动，MCU 会进入 50% duty failsafe。`GET_STATUS` 等有效 V1 命令会刷新活动计时。
- 协议：64 字节、XOR checksum 的 Protocol V1。`0x08` 和 `0x0D` 永久为 `RESERVED`，返回 `BAD_COMMAND`，不得重新分配。
- Bootloader：保留 8 KiB Bootloader 与无有效 Application 时的 DFU 防砖回退；Application 的 software-triggered DFU 与 SRAM magic handoff 均已删除。

冻结标签为 `software-dfu-removed-v1`：

| 镜像 | SHA-256 |
|---|---|
| Application | `3f920fc169dc59fea000ff06525a528b7bfaf5c76e57374ede7e4cb34caa0efb` |
| Bootloader deployment（8 KiB） | `a5bb37828e0a57d9f17f49681acd661f715dbe0d99efc7818134d70cccbcee44` |

## 快速入口

- 构建、测试、WCH-LinkE 调试与只读 target 检查：[docs/BUILD.md](docs/BUILD.md)
- 功能和硬件接口：[docs/FUNCTIONS.md](docs/FUNCTIONS.md)
- 系统与内存架构：[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- 冻结的 Protocol V1：[docs/USB_PROTOCOL.md](docs/USB_PROTOCOL.md)
- Bootloader 使用边界：[docs/BOOTLOADER.md](docs/BOOTLOADER.md)

正常开发/恢复优先使用 WCH-LinkE；`wchisp` 可用于已验证的 USB ISP 场景。Application 不会通过 HID 请求 DFU，也不要把已运行的 HID Application 当作 DFU 设备。

## 许可证与第三方代码

CheeseCool 原创代码采用 [MIT License](LICENSE)。仓库还包含或构建时使用第三方组件；其原始版权和许可证不被 MIT 覆盖，详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。中文说明仅供参考，法律正文以英文 `LICENSE` 为准。
