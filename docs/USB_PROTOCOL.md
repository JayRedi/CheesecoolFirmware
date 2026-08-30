# USB Protocol V1

Reports are exactly 64 bytes. Byte 0 is protocol version (1), byte 1 command, byte 2 sequence, byte 3 payload length, bytes 4.. payload, and byte 63 is XOR checksum over bytes 0..62. Responses echo version, command, and sequence; byte 4 is the status code.

For requests, byte 4 is the first payload byte. For responses, byte 4 is the status and byte 5 is the first payload byte. `payload length` excludes the status byte and is limited to 59 bytes. Unused report bytes are zero-filled. The checksum is the XOR of report bytes 0 through 62.

Commands: `1 PING`, `2 GET_INFO`, `3 SET_FAN_DUTY` (legacy, payload byte 0), `4 GET_FAN_STATUS`, `5 FAN_ENABLE`, `6 FAN_DISABLE`, `7 KEEPALIVE`, `8` legacy DFU command, `9 GET_STATUS`, `10 SET_MODE`, `11 SET_DUTY`, `12 SET_CURVE`, and `13 CMD_ENTER_DFU`. Commands 8 and 13 request the CheeseCool USB DFU Bootloader via the RAM hand-off flag; they do not request WCH ROM ISP. Both require a zero-length payload. The device sends the normal response first and resets after the HID IN transfer completes. Status codes are `0 OK`, `1 BAD_PACKET`, `2 BAD_COMMAND`, `3 NOT_SUPPORTED`, and `4 BAD_PARAMETER`.

## SET_MODE (command 10)

The request contains one payload byte at payload offset 0. The response has no payload and uses the common status and sequence fields.

| Payload value | Mode | Behavior |
|---:|---|---|
| 0 | `HOST_CONTROLLED` | Host-controlled mode; does not change the current duty. |
| 1 | `MAX` | Enables the fan and commands 100% duty through `fan_controller`. |

Other values, or a request payload length other than 1, return `BAD_PARAMETER` and do not change the mode. The CLI accepts `mode host` and `mode max`; AUTO and QUIET are not exposed because no corresponding control strategy exists in the current firmware. Fail-safe handling remains in its existing control path and can override ordinary output commands.

## SET_DUTY (command 11)

The request contains one payload byte at payload offset 0:

| Offset | Size | Type / unit | Range | Meaning |
|---:|---:|---|---:|---|
| 0 | 1 | `uint8_t` / percent | 0–100 | Requested fan duty |

The response has no payload. In `HOST_CONTROLLED` mode, a valid request calls the formal `fan_controller_set_duty()` API and returns `OK`. Values 101–255 and payload lengths other than 1 return `BAD_PARAMETER`. In `MAX` mode, the request returns `NOT_SUPPORTED`; it does not exit MAX or change the 100% output. Fail-safe or power-fault active state also rejects the request with `NOT_SUPPORTED`. The legacy command 3 remains supported through the same validation and control path.

The safety layer independently treats every structurally valid request using a known command ID as Host activity, even when the command returns `BAD_PARAMETER` or `NOT_SUPPORTED`. Invalid packets, unsupported command IDs, and USB configuration alone do not refresh Host activity. After reset, the application starts in `BOOT_WAIT` at 0% duty; it enters `FAILSAFE` after five minutes without any valid known request, or after 30 seconds without one after Host activity has first been seen. The current RAM-only fail-safe duty defaults to 50% and is constrained to 20–100%.

## GET_STATUS (command 9)

The request has no payload. The response keeps the common header and returns a 17-byte payload. All multi-byte values are little-endian. Offsets below are relative to the payload (report byte 5); the status byte is report byte 4.

| Payload offset | Size | Type / unit | Meaning |
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

`target_duty` and `actual_pwm_duty` are serialized from the same `fan_controller_get_duty()` value because the current firmware has no independent target register and no hardware PWM readback API. No status value is fabricated.

## SET_CURVE (command 12)

The request payload is:

| Offset | Size | Type / unit | Range | Meaning |
|---:|---:|---|---:|---|
| 0 | 1 | `uint8_t` / count | 1–29 | Number of curve points |
| 1 + 2×i | 1 | `uint8_t` / °C | 0–125 | Point `i` temperature |
| 2 + 2×i | 1 | `uint8_t` / percent | 0–100 | Point `i` duty |

Points must have strictly increasing temperatures. The maximum of 29 points is imposed by the 59-byte HID payload capacity. The device validates the complete request in a temporary RAM array, including exact payload length, point count, temperature range, duty range, and ordering, then commits it atomically to the formal fan-controller curve configuration. Invalid input returns `BAD_PARAMETER` and leaves the previous valid curve unchanged.

The response returns `OK` and echoes the accepted curve using the same payload layout. There is currently no `GET_CURVE` command. The curve is RAM-only and is not used for control yet because AUTO mode and temperature sensing are not implemented; SET_CURVE does not change the current mode or duty. The CLI accepts, for example, `curve 40:30 50:45 60:65 70:80 80:100`.

## ENTER_DFU (command 13)

The request has zero payload. A valid request returns `OK`, arms the existing pending-DFU flag, and waits until the HID response transmission completes before calling `system_request_dfu()`. The Application then disconnects and the CheeseCool DFU device appears as VID:PID `1A86:8035`, with Application target `0x2000`. Non-zero payload lengths return `BAD_PARAMETER` and do not reset. CLI usage is `cheesecoolctl.py dfu`; it waits a bounded time for DFU enumeration and does not automatically flash an Application.
