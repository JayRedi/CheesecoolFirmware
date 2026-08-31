# USB EP0 DMA 与 TRANSFER 元数据关联诊断 V3

该诊断只记录 RAM 快照，不改变 USB 描述符、端点响应、IRQ 优先级、标志清除顺序或安全控制逻辑。

## 格式

- Magic：`0x33435045`（RAM 字节为 `45 50 43 33`，即 `EPC3`）
- Version：`3`
- Header：24 字节
- Entry：32 字节
- Capacity：64 条
- 总长度：2072 字节
- 策略：保留最早事件；第 64 条写完后设置 `overflow=1`、`frozen=1`，不覆盖旧记录

Header 字段依次为：`magic, version, header_size, entry_size, capacity, count, next_sequence, frozen, overflow, reserved[6]`。

Entry 字段依次为：

`sequence, event, dispatch, INT_FG, INT_ST, MIS_ST, DEV_ADDR, RX_LEN, usb_configuration, decoded_flags, TOKEN, ENDP, usb_setup_count, usb_bus_reset_count, ep0[8], reserved[6]`。

`decoded_flags` 位定义：bit0 TRANSFER、bit1 BUS_RST、bit2 SUSPEND、bit3 TOG_OK、bit4 SETUP_ACT、bit5 FIFO_OV。

## 事件

1. `IRQ_ENTRY`：读取 `INT_FG/INT_ST` 后、任何分支与清除之前
2. `TRANSFER_BRANCH_ENTERED`
3. `SETUP_TOKEN_BRANCH_ENTERED`
4. `SETUP_EP0_ACCEPTED`
5. `SETUP_COUNT_INCREMENTED`：原计数递增后立即记录
6. `CONTROL_SETUP_ENTRY`：`control_setup()` 第一处可执行观测点
7. `BUS_RST_BRANCH_ENTERED`

## 离线解码

从 ELF 符号 `usb_ep0_corr_trace_v3` 的地址开始导出 2072 字节，执行：

```sh
python3 tools/usb_ep0_correlation_trace_v3_decode.py usb_ep0_corr_trace_v3.bin
```

解码器严格校验 magic、version、header/entry 尺寸和 dump 长度，并给出以下派生计数：

- `DMA_SETUP_WITH_TRANSFER`
- `DMA_SETUP_WITHOUT_TRANSFER`
- `DMA_SETUP_METADATA_ENDPOINT_MISMATCH`
- `DMA_SETUP_WITH_INT_ST_0xB8`

派生判断只针对 `IRQ_ENTRY`，避免把同一硬件事件的多个软件路径 marker 重复计数。标准 SETUP 语义采用保守识别；无法可靠识别的 8 字节仍原样输出，但不标记为有效标准请求。
