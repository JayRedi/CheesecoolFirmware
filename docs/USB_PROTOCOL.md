# USB 协议 V1（冻结）

Report 固定为 64 字节。Byte 0 是协议版本（1），byte 1 是 command，byte 2 是 sequence，byte 3 是 payload length，bytes 4.. 是 payload，byte 63 是 bytes 0..62 的 XOR checksum。Response 会回显 version、command 和 sequence；byte 4 是 status code。

对于 request，byte 4 是第一个 payload byte。对于 response，byte 4 是 status，byte 5 是第一个 payload byte。`payload length` 不包含 status byte，最大为 59 字节。未使用的 report byte 以零填充。checksum 是 report bytes 0 到 62 的 XOR。

常用 V1 命令是 `0x01 PING`、`0x09 GET_STATUS`、`0x0A SET_MODE`、`0x0B SET_DUTY` 与 `0x0C SET_CURVE`。为兼容早期客户端，固件仍识别 `0x02..0x07` 的 legacy 命令；新客户端应使用前述常用命令。

`0x08 RESERVED` 与 `0x0D RESERVED` **永久不得重新分配**，都返回 `BAD_COMMAND`。它们不写 RAM、不刷新 Host activity、不改变风扇状态，不会复位，也不会进入 DFU 或 Bootloader。Status code 为 `0 OK`、`1 BAD_PACKET`、`2 BAD_COMMAND`、`3 NOT_SUPPORTED` 和 `4 BAD_PARAMETER`。

## SET_MODE（`0x0A`）

payload 长度必须为 1：`0` 是 `HOST_CONTROLLED`，不会改变当前 duty；`1` 是 `MAX`，使能风扇并输出 100% duty。其他值或长度错误返回 `BAD_PARAMETER`。failsafe 或 power-fault 状态时返回 `NOT_SUPPORTED`。

## SET_DUTY（`0x0B`）

payload 长度必须为 1，取值 0–100（百分比）。只在 `HOST_CONTROLLED` 且非 failsafe 时执行；`MAX`、failsafe 或 power-fault 状态返回 `NOT_SUPPORTED`。无效参数返回 `BAD_PARAMETER`。

## SET_CURVE（`0x0C`）

payload 的第一个字节是点数，后续每点为温度（°C）和 duty（百分比）。点数必须在 1–`FAN_CURVE_MAX_POINTS`，温度严格递增且不超过 125，duty 不超过 100；否则返回 `BAD_PARAMETER`。曲线由 MCU 保存，但 `AUTO` 温度算法不在 MCU 中：macOS 客户端负责决定何时将目标 duty 下发给 MCU。

## GET_STATUS（`0x09`）

response payload 为 17 字节，所有多字节字段是 little-endian。

| payload offset | Size | 含义 |
|---:|---:|---|
| 0 | 1 | mode：0 `HOST_CONTROLLED`，1 `MAX` |
| 1 | 1 | target duty（%） |
| 2 | 1 | applied PWM duty（%） |
| 3 | 4 | TACH RPM |
| 7 | 1 | USB configured（0/1） |
| 8 | 1 | failsafe active（0/1） |
| 9 | 1 | power fault（0/1） |
| 10 | 4 | uptime（ms） |
| 14 | 3 | firmware version major/minor/patch |

每个结构有效且使用已知 command ID 的 request（包括 `GET_STATUS`）都会刷新 Host activity，即使该命令最终返回 `BAD_PARAMETER` 或 `NOT_SUPPORTED`。坏 checksum、未知 command 及 `0x08`/`0x0D` 不会刷新活动。已有主机活动后约 30 秒无有效活动会进入默认 50% failsafe；上电从未有活动时独立等待为 5 分钟。
