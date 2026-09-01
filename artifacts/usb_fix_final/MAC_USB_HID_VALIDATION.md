# Mac USB HID Reliability + Protocol V1 Hardware Validation

**Date:** 2026-09-01
**Target:** CH32X035F8U6, normal application (`1A86:FE01`)
**Method:** macOS native IOKit (`IOHIDManager` / `IOHIDDevice`) using the existing
`cheesecool-macos` transport and frozen 64-byte Protocol V1 framing.

## A. Task type

PCB / MCU Firmware — hardware validation only.

## B. Bootloader hash

| Check | Range | SHA-256 | Result |
|---|---|---|---|
| Before validation | `0x08000000..0x08002000` (8192 B) | `6a3dac5176b28ae0aeb924d528e179c0c07307c8114bcdec101a10bd49c5c71a` | PASS |
| After validation | `0x08000000..0x08002000` (8192 B) | `6a3dac5176b28ae0aeb924d528e179c0c07307c8114bcdec101a10bd49c5c71a` | PASS; byte-identical to pre-read |

## C. Application hash

| Check | Range | SHA-256 | Result |
|---|---|---|---|
| Before validation | `0x08002000..0x080034f4` (5364 B) | `f25d2554099e920e97db45838d699efa3f586dc261fd75ac409e648787d9f39d` | PASS |
| After validation | `0x08002000..0x080034f4` (5364 B) | `f25d2554099e920e97db45838d699efa3f586dc261fd75ac409e648787d9f39d` | PASS; byte-identical to pre-read |

## D. Baseline macOS enumeration

macOS IORegistry after the physical tests reports an active `CheeseCool USB HID`
device with VID:PID `1A86:FE01`, serial `CC-USB-001`, USB address `8`,
configuration `1`, endpoint-0 maximum packet size `64`, and a 12 Mbps link.

## E. 20/20 passive reconnect reliability

No Protocol V1 command was issued during the 20 physical reconnects.  The table
is reconstructed from macOS kernel `IOUSBHostFamily` remove/enumerate/configure
events. Every enumerated device was `1a86:fe01`, address `8`, 12 Mbps, then
selected configuration `1`.

| Cycle | Removal → enumeration time | Present / active | Address | Result |
|---:|---:|---|---:|---|
| 1 | 6.129 s | yes / yes | 8 | PASS |
| 2 | 3.917 s | yes / yes | 8 | PASS |
| 3 | 27.582 s | yes / yes | 8 | PASS |
| 4 | 2.345 s | yes / yes | 8 | PASS |
| 5 | 2.406 s | yes / yes | 8 | PASS |
| 6 | 1.745 s | yes / yes | 8 | PASS |
| 7 | 1.655 s | yes / yes | 8 | PASS |
| 8 | 1.964 s | yes / yes | 8 | PASS |
| 9 | 1.323 s | yes / yes | 8 | PASS |
| 10 | 1.741 s | yes / yes | 8 | PASS |
| 11 | 1.762 s | yes / yes | 8 | PASS |
| 12 | 1.300 s | yes / yes | 8 | PASS |
| 13 | 1.709 s | yes / yes | 8 | PASS |
| 14 | 1.898 s | yes / yes | 8 | PASS |
| 15 | 3.032 s | yes / yes | 8 | PASS |
| 16 | 1.523 s | yes / yes | 8 | PASS |
| 17 | 1.638 s | yes / yes | 8 | PASS |
| 18 | 2.549 s | yes / yes | 8 | PASS |
| 19 | 1.452 s | yes / yes | 8 | PASS |
| 20 | 2.593 s | yes / yes | 8 | PASS |

**Result: 20/20 PASS.**

## F. Native IOHID discovery/open

The existing native transport matched exactly `VID=0x1A86`, `PID=0xFE01`, opened
the device successfully, and used report ID `0` with exactly 64-byte reports.
No Python or hidapi transport was used.

## G. Single PING transaction

The qualified suite single PING completed in 1.911 ms with a valid response,
checksum, command, and sequence correlation. A subsequent native confirmation
validated the exact hardware response payload `50 4F 4E 47 00` (`PONG\0`).

## H. PING reliability

| Sent | Successful | Timeout | Checksum error | Sequence mismatch | Unexpected report |
|---:|---:|---:|---:|---:|---:|
| 100 | 100 | 0 | 0 | 0 | 0 |

The interval was 100 ms between transactions.

## I. GET_STATUS

`GET_STATUS` returned and decoded the required 17-byte payload. Observed fields
included `HOST_CONTROLLED`, USB configured, no power fault, firmware `1.0.0`,
and valid duty/RPM values.

## J. SET_MODE hardware test

`SET_MODE(0)` reported `HOST_CONTROLLED`; `SET_MODE(1)` reported `MAX` with
target/actual duty `100`; the final `SET_MODE(0)` returned to
`HOST_CONTROLLED`. All command responses were valid.

## K. SET_DUTY / RPM characterization

| Requested duty | Reported target / actual | RPM | Result |
|---:|---|---:|---|
| 0 | 0 / 0 | 1290 | PASS (minimum-speed semantic) |
| 20 | 20 / 20 | 600 | PASS |
| 40 | 40 / 40 | 1140 | PASS |
| 60 | 60 / 60 | 1740 | PASS |
| 80 | 80 / 80 | 2250 | PASS |
| 100 | 100 / 100 | 2640 | PASS |

RPM values are characterization observations, not hard thresholds.

## L. MCU host-activity / failsafe

After `HOST_CONTROLLED` and duty `20`, the native client sent no Protocol V1
traffic for 31 seconds. The first post-timeout `GET_STATUS` reported
`failsafe=true`, target/actual duty `50`, and RPM `660`.

**Result: PASS.**

## M. 5/5 reconnect after HID traffic

Each physical reconnect was detected by the native IOKit client; it rediscovered,
opened, PINGed, and read status successfully.

| Cycle | Re-enumeration time | Result |
|---:|---:|---|
| 1 | 3.145 s | PASS |
| 2 | 2.542 s | PASS |
| 3 | 2.832 s | PASS |
| 4 | 1.994 s | PASS |
| 5 | 1.891 s | PASS |

## N. App/client restart without USB replug

Two separately launched native client processes, without any USB replug between
them, independently rediscovered/opened the same device and completed PING plus
`GET_STATUS` (PING RTT 2.161 ms and 1.718 ms respectively).

**Result: PASS.**

## O. Sleep / wake

**BLOCKED — non-firmware environment limitation.** Triggering system sleep from
this active hardware/debug session cannot reliably wake the Mac or preserve the
automation channel. This is not a device failure; all other required hardware
tests passed.

## P. USB counter check

The post-test SRAM read was supporting evidence for the latest physical attach:

| Counter / state | Value |
|---|---:|
| BUS_RST | 1 |
| SETUP | 16 |
| SET_ADDRESS | 1 |
| SET_CONFIGURATION | 1 |
| configured | 1 |
| USB address | 8 |

The WCH-Link read temporarily halted the MCU; it was explicitly resumed without
reset, and `allrunning=true` was confirmed.

## Q. Error review

| Observation | Classification | Impact |
|---|---|---|
| 100-PING suite | No timeout/checksum/sequence/unexpected-report error | none |
| 20 reconnects | All enumerated and selected configuration 1 | none |
| Endpoint 0 string-descriptor-index-4 STALL in macOS kernel log | Non-fatal optional host descriptor query; subsequent configuration and HID discovery succeeded every time | no firmware change warranted |
| Endpoint `0x81` transaction error immediately before physical removal | Expected cancellation from hardware disconnect | none |
| One post-debug PING timeout | Debugger-induced: WCH-Link status showed `allhalted=true`; recovered immediately with `resume`, then PING/GET_STATUS passed | not a USB/Protocol failure |

## R. Persistent operation counters

| Operation | Count |
|---|---:|
| Bootloader write | 0 |
| Application write | 0 |
| Flash erase | 0 |
| Option-byte / protection write | 0 |
| Mass erase | 0 |
| DFU / ENTER_DFU | 0 |
| Debug reset | 0 |

Only read-only WCH-Link dumps/status operations were used. `resume` restored an
in-memory debug halt and made no persistent change.

## S. Verdict

**MAC USB HID VALIDATION = PASS**

The sleep/wake item is clearly non-firmware BLOCKED; every other PASS criterion
is met.

## T. Freeze recommendation

**MAC USB ENUMERATION = PASS**
**MAC HID TRANSPORT = PASS**
**PROTOCOL V1 HARDWARE = PASS**
**CURRENT USB FIX = READY TO FREEZE**

No firmware source or binary was modified. Software DFU was not removed or
invoked, and no commit was created.
