# 硬件测试清单

每项记录实测值，并标记 Pass/Fail。

| ID | 目的 / 操作 | 预期结果 |
|---|---|---|
| TEST-01 | 测量 3V3 | 约 3.3 V |
| TEST-02 | 测量升压输出 | 约 12.18 V |
| TEST-03 | 执行 `wchisp info` | CH32X035F8U6，64 KiB，Bootloader 02.60 |
| TEST-04 | 将 PA0 置低 | Q401 关闭，风扇全速 |
| TEST-05 | 使用示波器测量 PWM | 约 25 kHz |
| TEST-06..10 | 设置 duty 为 0/25/50/75/100% | 即使存在反相，实测风扇 duty 仍跟随命令 |
| TEST-11 | 观察 PA1 tach | 风扇脉冲清晰 |
| TEST-12 | 将 RPM 与 tach 频率比较 | 公式使用每转 2 个脉冲 |
| TEST-13 | 连接 USB Host | 集成 HID transport 后设备完成 enumeration |
| TEST-14..20 | 测试 PING、GET_INFO、SET_FAN_DUTY、GET_FAN_STATUS、ENABLE、DISABLE、KEEPALIVE | 返回有效的带版本 response 和 sequence echo |
| TEST-21 | 有效 activity 后停止 Host packet 30 秒 | 风扇回到配置的 50% fail-safe |
| TEST-22 | 复位 MCU | 硬件下拉提供全速；固件保持 fail-safe |
| TEST-23 | 断开 Host | Timeout 后回到 fail-safe |
| TEST-24 | 启用 feature 后施加已确认的 PWR_FAULT 输入 | Active level 被报告为 fault |
| TEST-25 | 连续运行 | 无阻塞、溢出或异常 duty 变化 |

## PWM 路由验证

原 TIM1_CH1/PA0 分配失败，因为 CH32X035F8U6 将 PA0 映射到 TIM2_CH1，而不是 TIM1_CH1。独立的
`tim2_pwm_diag` Application 验证了完整的 PA0 -> TIM2_CH1 -> Q401 -> FAN PWM 路径：两轮测试都显示
转速从慢到全速单调变化。该历史诊断曾在测试结束后请求 DFU；软件触发 DFU 已移除。精确 25 kHz 频率和占空比值
仍待示波器测量。
