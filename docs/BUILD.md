# 构建、调试与硬件兼容性

本文对应冻结标签 `software-dfu-removed-v1`。构建和测试不会写入 MCU Flash；任何 `flash`、`erase`、`reset` 或 DFU 上传命令都应在明确的硬件操作流程中单独执行。

## 受支持的构建环境

| 项目 | 当前验证/版本 |
|---|---|
| MCU / board | `CH32X035F8U6` / `ch32x035f8u6_evt_r0` |
| 生产 Application 环境 | `ch32x035f8u6_evt_r0` |
| Bootloader 环境 | `cheesecool_bootloader` |
| PlatformIO Core | 6.1.19 |
| Platform | `ch32v` 1.1.0 |
| Framework | `noneos-sdk`，`framework-wch-noneos-sdk` 2.30000.0 |
| 编译器 | `riscv-wch-elf-gcc` 12.2.0（toolchain-riscv 1.120200.220829） |

macOS 是本项目已实际使用的环境。Linux/Ubuntu 使用相同的 PlatformIO 配置进行本机构建；在新的主机或 USB/调试器组合上，应先完成本地工具与硬件回归，不把它视为已完成的硬件兼容认证。VS Code 用户安装 PlatformIO IDE 后打开仓库即可选择以上环境；命令行用户使用 `pio`，若不在 `PATH`，使用 PlatformIO 自己虚拟环境中的 `pio` 可执行文件。

## 构建

在仓库根目录运行：

```sh
pio run -e ch32x035f8u6_evt_r0 -t clean
pio run -e ch32x035f8u6_evt_r0
pio run -e cheesecool_bootloader -t clean
pio run -e cheesecool_bootloader
```

Application 的输出位于 `.pio/build/ch32x035f8u6_evt_r0/`：

- `firmware.bin`：链接器生成的原始镜像；
- `application.bin`：构建脚本复制出的 Application 镜像，链接起点为 `0x08002000`；
- `firmware.elf`：调试符号；
- `firmware.map`：链接 map。

Bootloader 的输出位于 `.pio/build/cheesecool_bootloader/`：

- `bootloader.bin`：实际 Bootloader payload；
- `bootloader-deployment.bin`：填充到完整 8 KiB 的部署镜像；
- `cheesecool-bootloader-only-test.bin`：Application 区全为 `0xFF` 的测试镜像；
- `firmware.elf` 和 `firmware.map`：调试/链接信息。

Application 不是地址零镜像，不能作为 `0x08000000` 的 Bootloader 镜像写入。常规开发/恢复不以 software DFU 为入口；只有设备已经通过既有非软件方式枚举为 DFU 时，才可使用 `pio run -e ch32x035f8u6_evt_r0 -t upload`。

## 测试与静态检查

```sh
make -C test/host reserved
git diff --check
```

前一命令编译并执行 Protocol V1 保留命令测试；它验证 `0x08` 与 `0x0D` 返回 `BAD_COMMAND` 且不刷新 host activity。不要提交 `.pio/`、ELF、MAP、BIN 或主机临时输出。

## WCH-LinkE、只读检查与 ISP

WCH-LinkE 是推荐的开发/恢复调试器。先确认连接，再执行只读操作：

```sh
wlink list
wlink --chip CH32X035 status
wlink --chip CH32X035 regs
wlink --chip CH32X035 dump 0x08000000 0xF800 --out target-readback.bin
```

最后一条会读取整段用户 Flash；输出包含设备固件，应保留在本机受控目录且不能提交。`wlink halt`、`resume`、`reset`、`erase`、`flash`、`write-reg`、`write-mem` 都会改变执行状态或内容，不属于只读检查。

`wchisp` 仍是已验证 USB ISP 开发工具，可先使用 `wchisp info` 查看已连接芯片。其 `flash`、`erase`、`reset` 命令会操作目标，必须在独立编程流程中使用；Application 正常运行时的 HID 设备不是 ISP/DFU 入口。

## PCB 与风扇兼容规格

以下结论严格区分实测、设计资料和未验证项。没有 PCB 原理图、BOM、器件降额与满载记录时，不得把设计推断写成额定能力。

### A. 已实机验证

| 项目 | 结论 |
|---|---|
| 风扇类型 | 标准 PC 4-pin PWM 风扇，带 PWM 控制和 TACH 反馈 |
| 已测试参考风扇 | TL9015，90 × 90 × 15 mm，12 V，0.13 A，约 1.56 W |
| 供电输出 | `FAN_12V` 约 12.18 V（历史硬件测试记录） |
| PWM 路径 | PA0 → R403 100 Ω → Q401 2N7002K → Pin 4，开漏且反相 |
| TACH 路径 | Pin 3 → 10 kΩ 上拉至 3.3 V → PA1，默认 2 pulses/rev |
| 基本电源 | 3V3 约 3.3 V |

因此当前**已实机验证规格**仅为 12 V / 0.13 A / 约 1.56 W 的上述参考风扇与已记录控制路径；它不是对任意 4 针风扇的电流保证。

### B. PCB 设计链与设计目标

项目硬件定义表明的链路为：USB-C 输入 → TPS2553D 保护/限流相关信号 → 5 V 至 12 V 的升压电源 → 风扇接口。升压链应包含电感、肖特基/整流元件、开关 MOS 与输出电容；PWM Q401 是控制 Pin 4 的开漏 MOS，不能据此推断升压级器件额定值。连接器 Pin 2 提供 `FAN_12V`，PCB 走线与连接器额定电流必须以发布的原理图/BOM/PCB 层叠和制造资料复核。

曾作为设计讨论的 12 V / 0.3 A / 3.6 W 如需要采用，只能标作“设计目标，尚未完成满载实机验证，不作为保证规格”。当前仓库没有足以确认该电流、功率、升压器件额定值、热性能或走线能力的公开 BOM/原理图证据。

### C. 未验证/不保证

- 不保证 0.3 A、3.6 W 或更高负载连续工作；不保证浪涌、堵转、热稳态或线缆压降能力。
- 未完成示波器频率/占空比精度、不同厂商风扇兼容性和满载电源验证。
- `PWR_FAULT` 的 MCU 管脚未确认，固件当前禁用该输入；不能把它当作已启用的运行时保护。

### 4 针风扇接口

| Pin | 信号 | 说明 |
|---:|---|---|
| 1 | GND | 风扇地 |
| 2 | `FAN_12V` | 12 V 风扇供电 |
| 3 | `TACH` | 转速反馈至 PA1 |
| 4 | `PWM` | Q401 开漏控制线 |

`0%` duty 只代表最小 PWM 控制值，不代表关闭 `FAN_12V`，也不承诺风扇物理停转。
