# 仅 RAM USB Trace 安装载荷

此目录是临时诊断产物，不是 PlatformIO 环境，不得烧录。

trampoline 会在 RAM 中记录一次 IRQ 入口快照，然后 tail-jump 到现有 Safety
`USBFS_IRQHandler`（`0x2778`）。不得调用该 handler，因为原 handler 以
`mret` 结束。
