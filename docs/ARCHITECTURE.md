# 系统架构

```text
Mac Host -- USB HID (transport abstraction) --> USB Protocol --> Fan Controller
                                                        |          |-- Fail-safe
                                                        |          |-- PWM (TIM2_CH1/PA0)
                                                        |          `-- TACH (EXTI/PA1)
                                                        `--> Device status
```

`main.c` 只初始化模块并运行非阻塞调度器。`fan_pwm` 负责 PA0 上的 TIM2 通道 1 和硬件反相。`fan_tach` 在 ISR 中统计下降沿，并按一秒窗口计算 RPM。`fan_controller` 是唯一面向业务的风扇控制 API。`failsafe` 独立于风扇模式跟踪 `BOOT_WAIT`、`HOST_ACTIVE` 和 `FAILSAFE`：上电从 0% 占空比开始，第一次有效的已知协议请求启动 Host activity，配置的仅 RAM fail-safe 占空比（默认 50%）在相应超时后生效。真实引脚确定前，`power_monitor` 在编译期禁用。`system_status` 是 USB 响应的唯一状态来源。

原 TIM1/PA0 配对不适用于 CH32X035F8U6：PA0 是 TIM2_CH1。独立的 `tim2_pwm_diag` 测试已在硬件上验证 PA0 的 TIM2_CH1；精确 25 kHz 频率和占空比精度仍待示波器测量。
