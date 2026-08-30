# CheeseCool USB DFU Bootloader

This integration follows the reference CH32X035 DFU project and keeps the existing application modules intact. The reference uses a standard USB DFU 1.1 EP0-only device compatible with `dfu-util`.

## Flash and RAM layout

```text
0x00000000  Bootloader       8 KiB
0x00002000  Application     54 KiB
0x0000F800  end of user flash

0x20000000  RAM
0x20004FF0  reserved hand-off flag word
0x20005000  RAM end
```

The reserved flag is `0x20004FF0`; the DFU request magic is `0xB0071DF0`. Both values are taken from the reference project's `config.h` and are mirrored in `include/dfu_config.h` and `bootloader/config.h`.

## Application request

`system_request_dfu()` first commands 100% fan duty through `fan_controller`, writes the magic word, executes a RISC-V `fence rw, rw`, then calls `NVIC_SystemReset()`. The existing ROM ISP `system_bootloader.c` remains present but is not called by normal application flow. `CMD_ENTER_BOOTLOADER` is retained for packet compatibility and now requests the CheeseCool DFU Bootloader, not the WCH ROM ISP.

The application USB transport now delivers command 8 (`CMD_ENTER_DFU`, compatible API alias `CMD_ENTER_BOOTLOADER`) and waits until its HID response has completed before calling the verified `system_request_dfu()` path. The explicit `ch32x035f8u6_evt_r0_dfu_test` environment still requests DFU after approximately 3 seconds; the default environment keeps the existing PWM debug test.

## Bootloader decision and protection

At reset the bootloader consumes the magic word. If it is absent and the first application word is nonzero/non-blank, it jumps to `0x2000`. If the application is blank/invalid, it remains in DFU. DFU writes use the application alias range `0x08002000..0x0800F800` and reject writes beyond the 54 KiB application region; the 8 KiB bootloader is never a valid download target.

## Build outputs

`pio run` builds both environments. The post-build script creates:

- `.pio/build/cheesecool_bootloader/bootloader.bin`
- `.pio/build/ch32x035f8u6_evt_r0/application.bin`
- `.pio/build/ch32x035f8u6_evt_r0/cheesecool-factory.bin`

The factory image is exactly 62 KiB, filled with `0xFF`, with the bootloader at offset 0 and application at offset `0x2000`.

## First installation

Use hardware BOOT and WCH ROM ISP once, then flash the combined factory image at address zero:

```text
wchisp flash .pio/build/ch32x035f8u6_evt_r0/cheesecool-factory.bin
```

Do not flash `application.bin` with `wchisp` at address zero: it is a raw image whose link address is `0x2000`, and the normal application PlatformIO environment deliberately does not configure ISP upload.

## Later DFU update

After the Bootloader and a DFU-capable application are installed:

```text
dfu-util -l
dfu-util -a 0 -d 1a86:8035 -D .pio/build/ch32x035f8u6_evt_r0/application.bin -R
```

The normal application environment uses `scripts/upload_dfu.py` as its custom PlatformIO uploader. It accepts an already-present DFU device or requests DFU through the application HID interface, then invokes `dfu-util`. The bootloader and diagnostic environments are not assigned this uploader.
