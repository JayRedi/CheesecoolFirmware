# 贡献指南

欢迎对 CheeseCool Firmware 提交可审查的改进。提交前请先确认改动不改变冻结 V1 的 USB、Protocol、failsafe、Bootloader 或 Flash layout，除非维护者明确开启新版本设计。

## 环境、构建与测试

- 使用 [docs/BUILD.md](docs/BUILD.md) 所列的 PlatformIO 环境。
- 至少构建 Application 环境 `ch32x035f8u6_evt_r0` 和 Bootloader 环境 `cheesecool_bootloader`。
- 运行 `make -C test/host reserved` 与 `git diff --check`。
- 固件改动需要在 PR 中说明实际硬件、工具版本、构建输出 SHA-256 和风险评估。

## 代码与协议要求

- 保持 C 代码简单、可审查，避免在 USB ISR 中执行阻塞或风扇业务逻辑。
- Protocol ID 是公开接口。`0x08` 与 `0x0D` 永久保留，**绝不能**重新用于 DFU、reset、Bootloader 或其他功能。
- 修改 USB、PWM、TACH、failsafe、Bootloader 或链接布局的 PR，至少要包含对应单元/主机测试和受控硬件回归结果；USB 改动还需记录枚举、PING、`GET_STATUS`、模式、duty、failsafe 与多次重新连接回归。
- 不要通过故意损坏有效 Application 测试防砖 DFU fallback。

## 仓库卫生

不提交 `.pio/`、临时 BIN/ELF/MAP、日志、目标读回 dump、调试采集、私钥、token、证书或个人 IDE 状态。正式发布 artifact 只能放入明确审核的 `artifacts/release/`，并随 PR 说明来源、SHA-256、许可证和保留理由。

第三方文件必须保留其原版权与许可证；新增/更新第三方依赖时同步更新 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
