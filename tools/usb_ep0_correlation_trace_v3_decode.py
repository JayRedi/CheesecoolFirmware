#!/usr/bin/env python3
"""解码 CheeseCool EP0 DMA/TRANSFER 关联诊断 Trace V3。"""

import argparse
import struct

MAGIC = 0x33435045
VERSION = 3
HEADER = struct.Struct("<I6H2B6s")
ENTRY = struct.Struct("<H6BH4BHH8s6s")

EVENTS = {
    1: "IRQ_ENTRY",
    2: "TRANSFER_BRANCH_ENTERED",
    3: "SETUP_TOKEN_BRANCH_ENTERED",
    4: "SETUP_EP0_ACCEPTED",
    5: "SETUP_COUNT_INCREMENTED",
    6: "CONTROL_SETUP_ENTRY",
    7: "BUS_RST_BRANCH_ENTERED",
}
DISPATCHES = {0: "NONE", 1: "TRANSFER", 2: "BUS_RST", 3: "SUSPEND", 4: "FALLBACK"}
TOKENS = {0x00: "OUT", 0x10: "SOF", 0x20: "IN", 0x30: "SETUP"}


def standard_setup_name(packet):
    bm, request = packet[0], packet[1]
    value = packet[2] | (packet[3] << 8)
    index = packet[4] | (packet[5] << 8)
    length = packet[6] | (packet[7] << 8)
    if bm & 0x60:
        return None
    if request == 0x05 and bm == 0x00 and value <= 0x7f and index == 0 and length == 0:
        return f"SET_ADDRESS({value})"
    if request == 0x06 and (bm & 0x80) and length > 0:
        descriptor = {1: "Device", 2: "Configuration", 3: "String", 0x21: "HID", 0x22: "Report"}.get(value >> 8, f"0x{value >> 8:02X}")
        return f"GET_DESCRIPTOR({descriptor}, index={value & 0xff})"
    if request == 0x09 and bm == 0x00 and index == 0 and length == 0:
        return f"SET_CONFIGURATION({value & 0xff})"
    if request == 0x08 and bm == 0x80 and value == 0 and index == 0 and length >= 1:
        return "GET_CONFIGURATION"
    if request == 0x00 and (bm & 0x80) and value == 0 and length == 2:
        return "GET_STATUS"
    if request in (0x01, 0x03, 0x07, 0x0a, 0x0b, 0x0c):
        return f"STANDARD_REQUEST_0x{request:02X}"
    return None


def setup_fields(packet):
    return {
        "bmRequestType": packet[0],
        "bRequest": packet[1],
        "wValue": packet[2] | (packet[3] << 8),
        "wIndex": packet[4] | (packet[5] << 8),
        "wLength": packet[6] | (packet[7] << 8),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", help="从 usb_ep0_corr_trace_v3 起始地址导出的完整二进制 RAM dump")
    args = parser.parse_args()
    data = open(args.trace, "rb").read()
    if len(data) < HEADER.size:
        raise SystemExit("文件短于 V3 header")

    magic, version, header_size, entry_size, capacity, count, next_sequence, frozen, overflow, _ = HEADER.unpack_from(data)
    if magic != MAGIC or version != VERSION:
        raise SystemExit(f"magic/version 不匹配: magic=0x{magic:08X}, version={version}")
    if header_size != HEADER.size or entry_size != ENTRY.size:
        raise SystemExit(f"结构尺寸不匹配: header={header_size}, entry={entry_size}")
    expected = header_size + capacity * entry_size
    if len(data) != expected:
        raise SystemExit(f"dump 大小错误: actual={len(data)}, expected={expected}")
    if count > capacity:
        raise SystemExit(f"count {count} 超过 capacity {capacity}")

    print(f"magic=0x{magic:08X} version={version} count={count}/{capacity} next={next_sequence} frozen={frozen} overflow={overflow}")
    derived = {"DMA_SETUP_WITH_TRANSFER": 0, "DMA_SETUP_WITHOUT_TRANSFER": 0,
               "DMA_SETUP_METADATA_ENDPOINT_MISMATCH": 0, "DMA_SETUP_WITH_INT_ST_0xB8": 0}

    for ordinal in range(count):
        values = ENTRY.unpack_from(data, header_size + ordinal * entry_size)
        sequence, event, dispatch, int_fg, int_st, mis_st, dev_addr, rx_len, configuration, decoded, token, endpoint, setup_count, reset_count, packet, _ = values
        name = standard_setup_name(packet)
        flags = []
        if decoded & 0x01: flags.append("TRANSFER")
        if decoded & 0x02: flags.append("BUS_RST")
        if decoded & 0x04: flags.append("SUSPEND")
        if decoded & 0x20: flags.append("FIFO_OV")
        print(f"#{sequence:03d} {EVENTS.get(event, f'EVENT_{event}'):<30} dispatch={DISPATCHES.get(dispatch, dispatch)} "
              f"INT_FG=0x{int_fg:02X} INT_ST=0x{int_st:02X} MIS_ST=0x{mis_st:02X} DEV_ADDR=0x{dev_addr:02X} "
              f"RX_LEN={rx_len} CFG={configuration} TOKEN={TOKENS.get(token, f'0x{token:02X}')} ENDP={endpoint} "
              f"TOG_OK={1 if decoded & 0x08 else 0} SETUP_ACT={1 if decoded & 0x10 else 0} "
              f"setup_count={setup_count} reset_count={reset_count} FLAGS={','.join(flags) or '-'} "
              f"EP0={packet.hex(' ').upper()}" + (f" [{name}]" if name else ""))
        if event == 1 and name:
            if int_fg & 0x02:
                derived["DMA_SETUP_WITH_TRANSFER"] += 1
            else:
                derived["DMA_SETUP_WITHOUT_TRANSFER"] += 1
            if endpoint != 0:
                derived["DMA_SETUP_METADATA_ENDPOINT_MISMATCH"] += 1
            if int_st == 0xB8:
                derived["DMA_SETUP_WITH_INT_ST_0xB8"] += 1
            fields = setup_fields(packet)
            print("  SETUP " + " ".join(f"{key}=0x{value:04X}" if key.startswith("w") else f"{key}=0x{value:02X}" for key, value in fields.items()))

    print("DERIVED")
    for label, value in derived.items():
        print(f"  {label}={value}")


if __name__ == "__main__":
    main()
