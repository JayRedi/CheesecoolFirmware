# USB Fix Final Freeze — macOS USB HID validated baseline

## A. Task type

PCB / MCU Firmware. This is a source and artifact freeze; no target write was
performed.

## B. Repository / branch / starting HEAD

The authoritative validated worktree was
`/private/tmp/cheesecool-fw-production.JThLiS`, on branch
`mac-usb-enumeration-debug`. It started at the specified production baseline
`1d6183db3d85749ecd2622840dfff1b0a41240cf`. The user's primary worktree on
branch `product-core-simulation` contained unrelated changes and was not used
or modified by this freeze.

## C. Baseline commit

`1d6183db3d85749ecd2622840dfff1b0a41240cf`

## D. Exact changed files

Relative to the baseline, the validated source change is only:

```
src/usb_device.c
```

No bootloader, descriptor, VID/PID, Protocol V1, PWM/TACH, failsafe, RAM-layout,
or macOS-client source changed.

## E. Exact source diff summary

Before, `USBFS_IRQHandler()` recognized SETUP only when both the SETUP token and
the `INT_ST` endpoint nibble were zero:

```c
if (token==USBFS_UIS_TOKEN_SETUP && ep==0)
```

After, it requires CH32X035 `USBFS_SETUP_ACT` plus the SETUP token:

```c
if ((status&USBFS_SETUP_ACT) && token==USBFS_UIS_TOKEN_SETUP)
```

This accepts a valid EP0 SETUP delivered into UEP0 DMA even when unrelated
endpoint-nibble bits are non-zero (captured example `INT_ST=0xB4`). It does not
broaden handling beyond a SETUP-active SETUP token.

The complete, reviewed 6-addition/1-deletion patch is
[`validated-usb-setup-fix.patch`](validated-usb-setup-fix.patch).

## F. Build environment

| Field | Value |
|---|---|
| PlatformIO | Core 6.1.19 |
| Production environment | `ch32x035f8u6_evt_r0` |
| Platform / framework | `ch32v 1.1.0+sha.499971a` / `noneos-sdk 2.30000.0+sha.34b1b78` |
| Compiler | `riscv-wch-elf-gcc` 12.2.0 |
| Command | `/Users/tangyujie/.platformio/penv/bin/pio run -e ch32x035f8u6_evt_r0` |
| Flash / RAM accounting | 5360 / 63488 B; 2520 / 20480 B |

The build followed an explicit clean of the environment's generated build
directory. Full metadata is in [`BUILD_METADATA.md`](BUILD_METADATA.md).

## G. Rebuilt application

| Field | Value |
|---|---|
| File | [`application.bin`](application.bin) |
| Size | 5364 B |
| SHA-256 | `f25d2554099e920e97db45838d699efa3f586dc261fd75ac409e648787d9f39d` |
| ELF | [`application.elf`](application.elf), 123636 B, SHA-256 `61ea6607c4f7aefbc1a1638baeddd36ae6804e8cf5a0f550348d91b39fa8ab39` |

## H. Validated target application

| Field | Value |
|---|---|
| Preserved target image | [`validated_target_application.bin`](validated_target_application.bin) |
| Size | 5364 B |
| SHA-256 | `f25d2554099e920e97db45838d699efa3f586dc261fd75ac409e648787d9f39d` |

## I. Byte-for-byte comparison

`cmp -s application.bin validated_target_application.bin` returned success.
The rebuilt application and validated physical target are byte-identical.

## J. Golden bootloader hash

The physical region `0x08000000..0x08001fff` remains the 8192-byte Golden
Bootloader:

```
6a3dac5176b28ae0aeb924d528e179c0c07307c8114bcdec101a10bd49c5c71a
```

## K. Static / regression checks

| Check | Result |
|---|---|
| Clean production PlatformIO build | PASS |
| Rebuilt BIN size/hash | PASS |
| Rebuilt BIN vs forensic target `cmp` | PASS |
| Source diff scope (`git diff --check`) | PASS |
| Existing candidate-worktree unit/protocol tests | No test sources present; not applicable |

The production build emitted existing unused-variable warnings in diagnostics
compiled out by the production configuration; no warning was introduced by the
single USB fix.

## L. Hardware validation evidence summary

The archived [macOS hardware validation report](MAC_USB_HID_VALIDATION.md)
records:

- 20/20 passive physical reconnects PASS;
- native IOHID transport PASS;
- PING 100/100, valid `PONG\0`, and valid 17-byte `GET_STATUS`;
- `SET_MODE` and 0/20/40/60/80/100 duty/RPM sweep PASS;
- 31-second MCU failsafe PASS;
- 5/5 reconnect-after-HID-traffic PASS; and
- native client restart without USB replug PASS.

Sleep/wake remains BLOCKED only because the active automation environment cannot
reliably wake the Mac; it is not a firmware failure. The one observed timeout
was debug-induced: WCH-Link had halted the MCU, and HID immediately recovered
after a non-reset `resume`.

## M. Frozen artifact paths

```
artifacts/usb_fix_final/application.bin
artifacts/usb_fix_final/application.elf
artifacts/usb_fix_final/validated_target_application.bin
artifacts/usb_fix_final/validated-usb-setup-fix.patch
artifacts/usb_fix_final/BUILD_METADATA.md
artifacts/usb_fix_final/SHA256SUMS
artifacts/usb_fix_final/MAC_USB_HID_VALIDATION.md
artifacts/usb_fix_final/USB_FIX_FINAL_FREEZE.md
```

## N. Commit hash

Validated source freeze commit:

```
1ba4f3ffcdcb142c4a7320cb3a520a7c635eb06f
fix(usb): finalize CH32X035 EP0 SETUP handling
```

## O. Tag / baseline identifier

Annotated local tag `usb-hid-validated-v1` points to the validated source freeze
commit `1ba4f3ffcdcb142c4a7320cb3a520a7c635eb06f`.

## P. Persistent target operation counters

| Operation | Count |
|---|---:|
| Bootloader write | 0 |
| Application write | 0 |
| Flash erase | 0 |
| Option-byte / protection write | 0 |
| Mass erase | 0 |
| DFU | 0 |

## Q. Verdict

**USB FIX FINAL FREEZE = PASS**

**MAC USB HID BASELINE = FROZEN**

The committed source, rebuilt application, and validated target all share the
exact application SHA-256
`f25d2554099e920e97db45838d699efa3f586dc261fd75ac409e648787d9f39d`.

**NEXT DEVELOPMENT STAGE: SOFTWARE DFU REMOVAL**

That next stage has not been started by this freeze.
