# Bootloader entry policy

Application 不会调用 WCH ROM ISP 或 CheeseCool Bootloader；对应的软件 reset-to-Bootloader 路径已删除。

CheeseCool Bootloader 的 USB DFU transport 保持不变，但只能在既有非软件入口已经进入 Bootloader 后使用。
无有效 Application 时，`DFU_ENTER_IF_NO_APP=1` 保留为防砖回退。
