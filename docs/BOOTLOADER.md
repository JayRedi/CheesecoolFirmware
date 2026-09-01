# Bootloader（冻结 V1）

CheeseCool 保留位于 `0x08000000..0x08001FFF` 的 8 KiB Bootloader。有效 Application 从 `0x08002000` 开始；Bootloader 验证 Application 向量后跳转执行。

- Application 的 software-triggered DFU 已删除。
- `BOOT_MAGIC_DFU` / SRAM magic handoff 已删除，完整 SRAM `0x20000000..0x20004FFF` 可供正常使用。
- `0x08`、`0x0D` 是 Protocol V1 的永久保留命令，均只返回 `BAD_COMMAND`；它们不能进入 Bootloader、复位设备或改变风扇状态。
- 无有效 Application 时，`DFU_ENTER_IF_NO_APP=1` 仍会留在 Bootloader DFU transport，作为防砖回退。这不是 Application 可触发的功能。

日常开发、调试和恢复使用 WCH-LinkE。若设备已经通过既有的非软件入口枚举为 DFU（`1A86:8035`），`scripts/upload_dfu.py` 才能上传 Application；它不会向 HID Application 发送任何进入 DFU 的命令。项目没有足够的、可公开复现的证据来规定 D+/D-、按钮或 strap 的具体 Bootloader 进入步骤，因此这里不编造该流程。

历史 SRAM recovery stub 仅用于受控硬件恢复，不是用户刷机方式，也不属于本项目的常规发布流程。
