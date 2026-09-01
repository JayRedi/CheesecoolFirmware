# 最终系统架构

```text
macOS/Linux 客户端
        │ USB HID（64-byte Protocol V1）
        ▼
CH32X035F8U6 Application ──► Fan Controller ──► PA0 / TIM2_CH1
        ▲                         │                    │
        │                         │                    ▼
GET_STATUS ◄── System Status ◄────┘              Q401 开漏反相级 ──► 4 针 PWM 风扇
        │
        └──────── PA1 / EXTI TACH ◄──────────────────────────────── 风扇转速脉冲

主机不可用 / 无有效 V1 活动
        │
        ▼
MCU timeout ──► failsafe（默认 50% duty）
```

`main.c` 运行非阻塞调度器。USB ISR 只处理端点与控制状态，Protocol V1 在主循环中解析；`fan_controller` 是风扇输出的业务入口；`fan_tach` 统计 PA1 下降沿并按 1 秒窗口换算 RPM；`system_status` 是 `GET_STATUS` 的状态来源。`power_monitor` 的真实 MCU 引脚尚未确认，当前在编译期禁用。

`failsafe` 的状态为 `BOOT_WAIT`、`HOST_ACTIVE`、`FAILSAFE`。上电后若尚未见过主机活动，首次超时为 5 分钟；一旦已有有效已知 Protocol V1 命令，后续无活动超时固定为约 30 秒。两种情况均输出配置的安全占空比，当前默认 50%。

## Flash 与 SRAM

| 区域 | 地址 | 用途 |
|---|---|---|
| Bootloader | `0x08000000..0x08001FFF` | 8 KiB |
| Application | 自 `0x08002000` 起 | 由 Bootloader 跳转运行 |
| SRAM | `0x20000000..0x20004FFF` | 完整 20 KiB，Application 与 Bootloader 均无保留 magic 区 |

不存在 software DFU magic、SRAM handoff word 或对应 linker reservation。PA0 的正确定时器路径是 `TIM2_CH1`，不是 TIM1。
