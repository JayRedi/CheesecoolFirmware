# USB 协议 V1

Report 固定为 64 字节。Byte 0 是协议版本（1），byte 1 是 command，byte 2 是 sequence，byte 3 是
payload length，bytes 4.. 是 payload，byte 63 是 bytes 0..62 的 XOR checksum。Response 会回显 version、
command 和 sequence；byte 4 是 status code。

对于 request，byte 4 是第一个 payload byte。对于 response，byte 4 是 status，byte 5 是第一个 payload byte。
`payload length` 不包含 status byte，最大为 59 字节。未使用的 report byte 以零填充。checksum 是 report
bytes 0 到 62 的 XOR。

Commands：`1 PING`、`2 GET_INFO`、`3 SET_FAN_DUTY`（legacy，payload byte 0）、`4 GET_FAN_STATUS`、
`5 FAN_ENABLE`、`6 FAN_DISABLE`、`7 KEEPALIVE`、`8 RESERVED`、`9 GET_STATUS`、`10 SET_MODE`、
`11 SET_DUTY`、`12 SET_CURVE` 和 `13 RESERVED`。命令 8 与 13 永久保留，均返回 `BAD_COMMAND`；它们不写 RAM、
不刷新 Host activity、不改变风扇状态，也不会复位或进入 DFU。Status code 为 `0 OK`、`1 BAD_PACKET`、`2 BAD_COMMAND`、
`3 NOT_SUPPORTED` 和 `4 BAD_PARAMETER`。

## SET_MODE (command 10)

Request 在 payload offset 0 包含一个 payload byte。Response 没有 payload，并使用通用 status 和 sequence 字段。

| Payload value | Mode | 行为 |
|---:|---|---|
| 0 | `HOST_CONTROLLED` | Host-controlled mode; does not change the current duty. |
| 1 | `MAX` | Enables the fan and commands 100% duty through `fan_controller`. |

其他值，或 payload length 不等于 1 的 request，返回 `BAD_PARAMETER`，且不改变 mode。CLI 接受
`mode host` 和 `mode max`；由于当前固件没有对应的控制策略，未提供 AUTO 和 QUIET。Fail-safe 仍在
现有控制路径中处理，并可覆盖普通输出命令。

## SET_DUTY (command 11)

Request 在 payload offset 0 包含一个 payload byte：

| Offset | Size | Type / unit | Range | 含义 |
|---:|---:|---|---:|---|
| 0 | 1 | `uint8_t` / percent | 0–100 | Requested fan duty |

Response 没有 payload。在 `HOST_CONTROLLED` mode 中，有效 request 调用正式的
`fan_controller_set_duty()` API 并返回 `OK`。101–255 的值以及 payload length 不等于 1 的 request
返回 `BAD_PARAMETER`。在 `MAX` mode 中，request 返回 `NOT_SUPPORTED`，不会退出 MAX 或改变 100% 输出。
Fail-safe 或 power-fault active state 也会以 `NOT_SUPPORTED` 拒绝 request。Legacy command 3 仍通过相同
的校验和控制路径支持。

Safety layer 独立地将每个结构有效且使用已知 command ID 的 request 视为 Host activity，即使该命令返回
`BAD_PARAMETER` 或 `NOT_SUPPORTED`。无效 packet、不支持的 command ID 以及仅 USB configuration 变化，
都不会刷新 Host activity。复位后 Application 以 0% duty 进入 `BOOT_WAIT`；五分钟内没有任何有效已知
request 时进入 `FAILSAFE`，Host activity 首次出现后，连续 30 秒没有有效 request 时也进入 `FAILSAFE`。
当前仅 RAM 的 fail-safe duty 默认为 50%，限制范围为 20–100%。

## GET_STATUS (command 9)

Request 没有 payload。Response 保留通用 header，并返回 17 字节 payload。所有多字节值均为 little-endian。
下表 offset 相对于 payload（report byte 5）；status byte 是 report byte 4。

| Payload offset | Size | Type / unit | 含义 |
|---:|---:|---|---|
| 0 | 1 | `uint8_t` | `mode`: 0 HOST_CONTROLLED, 1 MAX; sourced from `fan_controller` |
| 1 | 1 | `uint8_t` / percent | `target_duty`: current formal fan-controller duty |
| 2 | 1 | `uint8_t` / percent | `actual_pwm_duty`: same applied PWM duty; no separate hardware readback exists |
| 3 | 4 | `uint32_t` / rpm | `tach_rpm` |
| 7 | 1 | `uint8_t` / boolean | `usb_connected`: 1 when USB is configured |
| 8 | 1 | `uint8_t` / boolean | `failsafe_active` |
| 9 | 1 | `uint8_t` / boolean | `power_fault` |
| 10 | 4 | `uint32_t` / ms | `uptime_ms` |
| 14 | 1 | `uint8_t` | Firmware major version |
| 15 | 1 | `uint8_t` | Firmware minor version |
| 16 | 1 | `uint8_t` | Firmware patch version |

由于当前固件没有独立 target register，也没有硬件 PWM readback API，`target_duty` 和 `actual_pwm_duty`
都从同一个 `fan_controller_get_duty()` 值序列化。没有伪造任何 status value。

## SET_CURVE (command 12)

Request payload 为：

| Offset | Size | Type / unit | Range | 含义 |
|---:|---:|---|---:|---|
| 0 | 1 | `uint8_t` / count | 1–29 | Number of curve points |
| 1 + 2×i | 1 | `uint8_t` / °C | 0–125 | Point `i` temperature |
| 2 + 2×i | 1 | `uint8_t` / percent | 0–100 | Point `i` duty |

温度点必须严格递增。由于 HID payload 容量为 59 字节，最多支持 29 个点。设备在临时 RAM array 中完整
校验 request，包括精确 payload length、点数、温度范围、duty 范围和顺序，然后以原子方式提交到正式
fan-controller curve configuration。无效输入返回 `BAD_PARAMETER`，并保持上一次有效曲线不变。

Response 返回 `OK`，并使用相同 payload layout 回显已接受的曲线。目前没有 `GET_CURVE` command。该曲线
仅存于 RAM；由于 AUTO mode 和温度感测尚未实现，目前不用于控制。`SET_CURVE` 不改变当前 mode 或 duty。
CLI 示例：`curve 40:30 50:45 60:65 70:80 80:100`。

## Reserved commands (8 and 13)

`0x08` 与 `0x0D` 是永久保留的协议 ID，不能重新分配。任意 payload length 均返回 `BAD_COMMAND`。
它们不触发 reset、DFU request、RAM handoff、Bootloader jump 或持久状态变化。`cheesecoolctl.py reserved-test`
可在已枚举的 HID Application 上验证两个响应。
