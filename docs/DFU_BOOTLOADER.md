# CheeseCool USB DFU Bootloader

此集成遵循 CH32X035 DFU 参考项目，并保持现有 Application 模块不变。参考实现使用与
`dfu-util` 兼容的标准 USB DFU 1.1、仅 EP0 设备。

## Flash 和 RAM 布局

```text
0x00000000  Bootloader       8 KiB
0x00002000  Application     54 KiB
0x0000F800  end of user flash

0x20000000  RAM
0x20004FF0  reserved hand-off flag word
0x20005000  RAM end
```

保留 flag 为 `0x20004FF0`；DFU request magic 为 `0xB0071DF0`。两个值均取自参考项目的
`config.h`，并同步定义在 `include/dfu_config.h` 和 `bootloader/config.h` 中。

## Application 请求

`system_request_dfu()` 首先通过 `fan_controller` 设置风扇 100% 占空比，写入 magic word，执行 RISC-V
`fence rw, rw`，然后调用 `NVIC_SystemReset()`。现有 ROM ISP `system_bootloader.c` 仍然保留，但正常
Application 流程不会调用它。为保持数据包兼容性，`CMD_ENTER_BOOTLOADER` 仍然保留，但现在请求的是
CheeseCool DFU Bootloader，而不是 WCH ROM ISP。

Application USB transport 现在提供命令 8（`CMD_ENTER_DFU`，兼容 API 别名 `CMD_ENTER_BOOTLOADER`），
并等待 HID 响应完成后再调用已验证的 `system_request_dfu()` 路径。显式的
`ch32x035f8u6_evt_r0_dfu_test` 环境仍在约 3 秒后请求 DFU；默认环境保留现有 PWM debug test。

## Bootloader 决策与保护

复位时 Bootloader 消费 magic word。如果 magic 不存在，且 Application 第一个 word 非零/非空白，
则跳转到 `0x2000`。如果 Application 为空或无效，则留在 DFU。DFU 写入使用 Application alias 范围
`0x08002000..0x0800F800`，并拒绝超出 54 KiB Application 区域的写入；8 KiB Bootloader 永远不是
有效下载目标。

## 构建输出

`pio run` 会构建两个环境。post-build 脚本生成：

- `.pio/build/cheesecool_bootloader/bootloader.bin`
- `.pio/build/ch32x035f8u6_evt_r0/application.bin`
- `.pio/build/ch32x035f8u6_evt_r0/cheesecool-factory.bin`

factory image 恰好为 62 KiB，以 `0xFF` 填充，Bootloader 位于 offset 0，Application 位于 offset `0x2000`。

## 首次安装

使用硬件 BOOT 和 WCH ROM ISP 一次，然后将合并的 factory image 烧录到地址零：

```text
wchisp flash .pio/build/ch32x035f8u6_evt_r0/cheesecool-factory.bin
```

不要使用 `wchisp` 将 `application.bin` 烧录到地址零：它是链接地址为 `0x2000` 的 raw image，且正常
Application PlatformIO 环境刻意没有配置 ISP upload。

## 后续 DFU 更新

安装 Bootloader 和支持 DFU 的 Application 后：

```text
dfu-util -l
dfu-util -a 0 -d 1a86:8035 -D .pio/build/ch32x035f8u6_evt_r0/application.bin -R
```

正常 Application 环境使用 `scripts/upload_dfu.py` 作为自定义 PlatformIO uploader。它接受已经存在的
DFU 设备，或通过 Application HID 接口请求 DFU，然后调用 `dfu-util`。Bootloader 和诊断环境不使用此 uploader。
