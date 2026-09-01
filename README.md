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
| Factory image（62 KiB） | `36c932f34ace2771fdf9bce5cf040aa0f5a57bea9ec93e61c85fdb4da383f127` |

## 快速入口

- 构建、测试、WCH-LinkE 调试与只读 target 检查：[docs/BUILD.md](docs/BUILD.md)
- 功能和硬件接口：[docs/FUNCTIONS.md](docs/FUNCTIONS.md)
- 系统与内存架构：[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- 冻结的 Protocol V1：[docs/USB_PROTOCOL.md](docs/USB_PROTOCOL.md)
- Bootloader 使用边界：[docs/BOOTLOADER.md](docs/BOOTLOADER.md)

正常开发/恢复优先使用 WCH-LinkE；空芯片若能被 `wchisp info` 识别为 WCH ROM ISP，也可以直接通过主 USB-C 首次烧录。Application 不会通过 HID 请求 DFU，也不要把已运行的 HID Application 当作 DFU 设备。

## 空芯片新 PCB：首次烧录

空的 CH32X035F8U6 不会枚举 CheeseCool HID/DFU，但可以在硬件 BOOT 模式下通过主 USB-C 进入芯片内部的 WCH ROM ISP。若 `wchisp info` 能看到 `CH32X035F8U6`（历史验证记录为 Bootloader 02.60），可以不接 WCH-LinkE 直接完成首次烧录。

### 方式 A：USB-C + WCH ROM ISP（优先）

1. 按 PCB 上已有的 BOOT 按键/跳线进入 WCH ROM ISP，使用主 USB-C 给目标板供电和通信；不要同时接另一只 3.3 V 电源。
2. 确认 ROM ISP 已枚举：

   ```sh
   wchisp info
   ```

   如果提示找不到 `4348:55e0` 或 `1a86:55e0`，说明当前没有进入 ROM ISP；断电后重新设置 BOOT，再检查 USB-C 数据线，仍不行时改用方式 B。

3. 下载 [Release v1.0.0](https://github.com/JayRedi/Cheesecool_Firmware/releases/tag/v1.0.0) 的 `cheesecool-factory-v1.0.0.bin`，执行一次完整烧录：

   ```sh
   wchisp flash cheesecool-factory-v1.0.0.bin
   ```

   该合并镜像大小为 62 KiB，Bootloader 位于 `0x08000000`，Application 位于 `0x08002000`；`wchisp flash` 会擦除用户 Flash 并在写入后复位。此命令只适用于空板或明确授权的完整重刷，不要用于需要保留现有镜像的设备。

4. 退出 BOOT 模式并重新插拔主 USB-C。等待约 2–3 秒后，主机应看到 `1A86:FE01`（产品字符串 `CheeseCool USB HID`）。此时 `wchisp info` 不再找到 ROM ISP 是正常现象。

### 方式 B：WCH-LinkE / SDI 后备流程

如果方式 A 无法枚举 ROM ISP，断开主 USB-C 数据线，连接 WCH-LinkE 的 `DBG_DIO`、`DBG_DCK`、GND，并确认目标板有稳定 3.3 V 供电。不要让两个独立电源同时驱动同一块板。

1. 检查探针和芯片：

   ```sh
   wlink list
   wlink --chip CH32X035 status
   ```

2. 下载 [Release v1.0.0](https://github.com/JayRedi/Cheesecool_Firmware/releases/tag/v1.0.0) 的两个独立资产。先擦除并写入 8 KiB Bootloader，再在不擦除的情况下写入 Application：

   ```sh
   wlink --chip CH32X035 flash --erase --address 0x08000000 \
     cheesecool-bootloader-deployment-v1.0.0.bin
   wlink --chip CH32X035 flash --address 0x08002000 \
     cheesecool-application-v1.0.0.bin
   ```

   Bootloader 必须位于 `0x08000000..0x08001FFF`，Application 必须位于 `0x08002000` 起始的区域。不能使用 `wchisp flash application.bin` 把 raw Application 当作地址零镜像。

3. 可选地只读回读两个区域并校验：

   ```sh
   wlink --chip CH32X035 dump 0x08000000 0x2000 --out bootloader-readback.bin
   wlink --chip CH32X035 dump 0x08002000 5248 --out application-readback.bin
   shasum -a 256 bootloader-readback.bin application-readback.bin
   ```

   预期 SHA-256 分别为 `a5bb37828e0a57d9f17f49681acd661f715dbe0d99efc7818134d70cccbcee44` 和 `3f920fc169dc59fea000ff06525a528b7bfaf5c76e57374ede7e4cb34caa0efb`。回读文件只保存在本机，不要提交到 Git。

4. 让目标运行并重新连接主 USB-C：

   ```sh
   wlink --chip CH32X035 reset run
   ```

   等待约 2–3 秒后，主机应看到 `1A86:FE01`（产品字符串 `CheeseCool USB HID`）。空芯片阶段不要运行 `pio run -t upload`，也不要向 Application 发送 `0x08` 或 `0x0D`；这两个 ID 永久保留并返回 `BAD_COMMAND`。

如果 `wlink status` 找不到目标，先检查目标供电、WCH-LinkE 的 DIO/DCK/GND 接线和芯片选择；不要为了“恢复”而自动执行 `erase` 或 `unprotect`。

## 许可证与第三方代码

CheeseCool 原创代码采用 [MIT License](LICENSE)。仓库还包含或构建时使用第三方组件；其原始版权和许可证不被 MIT 覆盖，详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。中文说明仅供参考，法律正文以英文 `LICENSE` 为准。
