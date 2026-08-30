# Hardware test checklist

For every item record measured value and mark Pass/Fail.

| ID | Purpose / operation | Expected result |
|---|---|---|
| TEST-01 | Measure 3V3 | About 3.3 V |
| TEST-02 | Measure boost output | About 12.18 V |
| TEST-03 | Run `wchisp info` | CH32X035F8U6, 64 KiB, bootloader 02.60 |
| TEST-04 | Set PA0 low | Q401 off, fan full speed |
| TEST-05 | Scope PWM | About 25 kHz |
| TEST-06..10 | Set duty 0/25/50/75/100% | Measured fan duty follows command despite inversion |
| TEST-11 | Observe PA1 tach | Clean fan pulses |
| TEST-12 | Compare RPM to tach frequency | Formula uses 2 pulses/rev |
| TEST-13 | Connect USB host | Device enumerates after HID transport is integrated |
| TEST-14..20 | Exercise PING, GET_INFO, SET_FAN_DUTY, GET_FAN_STATUS, ENABLE, DISABLE, KEEPALIVE | Valid versioned response and sequence echo |
| TEST-21 | Stop host packets for 30 s after valid activity | Fan returns to configured 50% fail-safe |
| TEST-22 | Reset MCU | Hardware pull-down gives full speed; firmware remains fail-safe |
| TEST-23 | Disconnect host | Timeout returns to fail-safe |
| TEST-24 | Apply confirmed PWR_FAULT input after enabling feature | Active level is reported as fault |
| TEST-25 | Run continuously | No blocking, overflow, or unexpected duty change |

## PWM routing validation

The original TIM1_CH1/PA0 assignment failed because CH32X035F8U6 maps PA0 to TIM2_CH1, not TIM1_CH1. The standalone `tim2_pwm_diag` application verified the complete PA0 -> TIM2_CH1 -> Q401 -> FAN PWM path: both test rounds showed a monotonic slow-to-full-speed trend and the application then entered the verified DFU Bootloader. Exact 25 kHz frequency and duty values remain pending oscilloscope measurement.
