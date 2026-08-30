#!/usr/bin/env python3
"""Decode a raw dump of the 24-byte RAM USB diagnostic trace records."""

import argparse
import struct

ENTRY = struct.Struct("<I6BHH2B3H8s16s2x")
CAPACITY = 64
EVENTS = {
    1: "BOOT",
    2: "USB_INIT_BEGIN",
    3: "CLOCK_READY",
    4: "ENDPOINT_INIT",
    5: "FLAGS_CLEARED",
    6: "PULLUP_ENABLED",
    7: "BUS_RST",
    8: "SET_ADDRESS_SETUP",
    9: "SET_ADDRESS_STATUS",
    10: "SET_ADDRESS_COMMIT",
    11: "GET_DESCRIPTOR_SETUP",
    12: "GET_DESCRIPTOR_TX",
    13: "EP0_IN",
    14: "EP0_OUT",
    15: "OTHER_TRANSFER",
}
TOKENS = {0x00: "OUT", 0x10: "SOF", 0x20: "IN", 0x30: "SETUP"}


def setup_text(setup):
    if not any(setup):
        return ""
    bm, req = setup[0], setup[1]
    value = setup[2] | (setup[3] << 8)
    index = setup[4] | (setup[5] << 8)
    length = setup[6] | (setup[7] << 8)
    descriptor = {1: "DEVICE", 2: "CONFIGURATION", 3: "STRING",
                  0x21: "HID", 0x22: "REPORT"}.get(value >> 8)
    if req == 0x06:
        name = f"GET_DESCRIPTOR {descriptor or f'0x{value >> 8:02x}'}"
    elif req == 0x05:
        name = "SET_ADDRESS"
    elif req == 0x09:
        name = "SET_CONFIGURATION"
    else:
        name = ""
    suffix = f" [{name}]" if name else ""
    return (f" bmRequestType=0x{bm:02x} bRequest=0x{req:02x}"
            f" wValue=0x{value:04x} wIndex=0x{index:04x} wLength={length}{suffix}")


def decode(path, show_all=False, count=None):
    data = open(path, "rb").read()
    if len(data) % ENTRY.size:
        raise SystemExit(f"input size {len(data)} is not a multiple of {ENTRY.size}")
    total = len(data) // ENTRY.size
    limit = total if count is None else min(count, total)
    for ordinal in range(limit):
        sequence, event, int_fg, int_st, mis_st, dev_addr, ctrl, tx_len, dma16, \
            request_type, request_code, request_value, request_length, remaining, setup, tx_data = \
            ENTRY.unpack_from(data, ordinal * ENTRY.size)
        if not show_all and event == 0:
            continue
        label = EVENTS.get(event, f"EVENT_{event}")
        token = TOKENS.get(int_st & 0x30, f"OTHER(0x{int_st & 0x30:02x})")
        flags = []
        if int_fg & 0x02: flags.append("TRANSFER")
        if int_fg & 0x01: flags.append("BUS_RST")
        if int_fg & 0x04: flags.append("SUSPEND")
        line = (f"#{sequence:02d} {label:<26} INT_FG=0x{int_fg:02x} "
                f"INT_ST=0x{int_st:02x} MIS_ST=0x{mis_st:02x} "
                f"DEV_ADDR=0x{dev_addr:02x} EP0_CTRL=0x{ctrl:02x} "
                f"T_LEN={tx_len} DMA=0x{dma16:04x} TOKEN={token} "
                f"ENDP={int_st & 0x0F} SETUP_ACT={(int_st >> 7) & 1} "
                f"TOG_OK={(int_st >> 6) & 1}")
        if flags:
            line += " FLAGS=" + ",".join(flags)
        if event in (8, 11) and (int_st & 0x30) == 0x30 and (int_st & 0x0F) == 0:
            line += setup_text(setup)
        if event == 12:
            n = min(tx_len, len(tx_data))
            line += f" TX_DATA={tx_data[:n].hex(' ').upper()}"
        print(line)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", help="raw usb_trace_entry_t dump")
    parser.add_argument("--all", action="store_true", help="show empty ring slots too")
    parser.add_argument("--count", type=int, default=None,
                        help="number of records to decode")
    args = parser.parse_args()
    decode(args.trace, args.all, args.count)


if __name__ == "__main__":
    main()
