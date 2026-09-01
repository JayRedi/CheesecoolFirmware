# CheeseCool USB DFU Bootloader

CheeseCool Bootloader 保留标准 USB DFU 1.1 transport、Application 校验、Application Flash 映射和
`DFU_ENTER_IF_NO_APP=1` 防砖回退。它不再检查 SRAM magic，Application 也不再能请求 Bootloader。

## Flash 与 RAM 布局

```text
0x00000000  Bootloader       8 KiB
0x00002000  Application     54 KiB
0x0000F800  end of user flash

0x20000000  RAM
0x20005000  RAM end
```

`0x20004FF0..0x20004FFF` 已归还为普通 SRAM；没有 DFU handoff word、magic value 或特殊 linker
reservation。Application 与 Bootloader 的 linker RAM 均为完整 20 KiB (`0x5000`)。

## Bootloader 决策

若 Application 首 word 有效，Bootloader 跳转至 `0x2000`。若 Application 缺失或无效，
`DFU_ENTER_IF_NO_APP=1` 使 Bootloader 保持在 DFU，作为防砖回退。该回退不是 Application 可触发的功能。

既有官方/非软件 Bootloader entry 及其 USB D+/D- 行为不在本次修改范围内；本项目没有新增 GPIO、按钮、
strap 或 Application-side entry mechanism。

## DFU 下载

在既有非软件入口已经使 DFU 设备 `1a86:8035` 枚举后，使用：

```text
dfu-util -a 0 -d 1a86:8035 -D .pio/build/ch32x035f8u6_evt_r0/application.bin -R
```

Bootloader 将 DFU block 0 映射到 Application 区域 `0x08002000..0x0800F800`；不要传入物理地址 override。
`scripts/upload_dfu.py` 只接受已存在的 DFU device，绝不会向 Application HID 发送 `0x08` 或 `0x0D`。
