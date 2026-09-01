# 第三方组件与许可证审计

本审计针对冻结标签 `software-dfu-removed-v1` 的受版本控制文件，以及本机构建所解析的 PlatformIO 包。根目录 [LICENSE](LICENSE) 只授予 CheeseCool 原创代码；它不删除、不替代或覆盖任何第三方版权、许可证或使用条件。

## 审计原则

- 保留每个第三方目录中的原始 `COPYING`、`AUTHORS`、版权头和许可证文本。
- WCH SDK 的文件不复制入本仓库；项目通过 PlatformIO 安装的头文件/库进行编译。任何未来复制入仓库的 WCH 文件必须原样保留其版权头和 MCU 使用条件。
- `src/`、`include/`、`bootloader/`、`ld/`、`reference_min_app/`、`diagnostics/`、`scripts/` 和 `test/` 已检查版权/许可证标头：当前受控文件没有额外被识别出的第三方版权头。它们调用 WCH API 并不使 WCH 头文件成为仓库文件；若后续确认其中某文件由上游示例派生，必须先补充准确来源和原条款再公开发布。

| 组件 | 文件/目录 | 来源与上游 | 原版权/许可证 | 是否修改 | CheeseCool MIT 是否适用 |
|---|---|---|---|---|---|
| WCH CH32X035 NoneOS SDK | 构建时安装的 `framework-wch-noneos-sdk` 2.30000.0；项目仅 `#include` 其头文件 | [Community-PIO-CH32V framework](https://github.com/Community-PIO-CH32V/framework-wch-noneos-sdk)，WCH 下载页 | 头文件标注 Copyright (c) 2021 Nanjing Qinheng Microelectronics；软件/二进制仅用于 Nanjing Qinheng MCU 的条件 | 未纳入、未修改 | 否；遵从 WCH 原条款 |
| CH32V Platform | 构建时安装的 `ch32v` 1.1.0 | [platform-ch32v](https://github.com/Community-PIO-CH32V/platform-ch32v) | Apache-2.0（平台元数据） | 未纳入、未修改 | 否 |
| GNU RISC-V toolchain | 构建时安装的 `toolchain-riscv` 1.120200.220829 / GCC 12.2.0 | [xPack RISC-V GCC](https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack) | GPL-2.0-or-later（工具包元数据） | 未纳入、未修改 | 否；构建工具 |
| OpenOCD WCH tool | 构建时安装的 `tool-openocd-riscv-wch` 2.1100.260228 | [riscv-openocd](https://github.com/riscv-collab/riscv-openocd) | GPL-2.0-or-later（工具包元数据） | 未纳入、未修改 | 否；构建/调试工具 |
| wchisp | 构建时安装的 `tool-wchisp` 0.23.240914 | [wchisp](https://github.com/ch32-rs/wchisp) | GPL-2.0-only（工具包元数据） | 未纳入、未修改 | 否；外部 ISP 工具 |
| wlink | 构建时安装的 `tool-wlink` 0.23.241116 | [wlink](https://github.com/ch32-rs/wlink) | MIT（工具包元数据） | 未纳入、未修改 | 否；外部调试工具 |
| dfu-util | `tools/dfu-util-cheesecool/`，含 `COPYING`、`AUTHORS` | [dfu-util](https://github.com/dfu-util/dfu-util) | 目录内 `COPYING`：GNU GPL v2；保留上游版权/文本 | 包含本项目的 `cheesecool-skip-set-interface.patch` | 上游源文件否；本项目 patch 仅在适用法律范围内受 MIT 覆盖，不改变 GPL 义务 |
| libusb 1.0.29 | `tools/host_usb_diag/libusb_ab/libusb-1.0.29/`，含 `COPYING`、`AUTHORS` | [libusb](https://github.com/libusb/libusb) | 目录内 `COPYING`：GNU LGPL v2.1 | 未发现 CheeseCool 对 vendored libusb 源的修改 | 否 |

`tools/host_usb_diag/` 中 CheeseCool 编写的诊断包装代码遵从根 MIT；其使用的 libusb 仍按 LGPL-2.1 单独许可。上表的外部工具并不因被列出而被重新分发或改变许可证。

## 发布前复核

发布者应再次检查新加入文件的版权头、依赖锁定版本和二进制再分发义务。对于 WCH 官方示例，不能仅假定为 BSD-3-Clause：必须以该文件随附的实际声明为准；本次使用的 CH32X035 头文件带有 WCH 自身版权和 MCU 使用条件。不得用 CheeseCool 的 MIT 文本覆盖它们。
