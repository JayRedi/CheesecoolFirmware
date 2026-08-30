#!/usr/bin/env python3
"""Golden DFU PASS/FAIL reproducibility harness.

This harness only uses OpenOCD telnet RAM/debug operations and host-side
read-only USB inspection. It never sends a Flash command.
The operator performs the physical USB-C unplug/plug between prompts.
"""

from __future__ import annotations

import csv
import re
import socket
import subprocess
import sys
import time
from pathlib import Path

HOST = "127.0.0.1"
PORT = 4444
MAGIC_ADDR = 0x20004FF0
MAGIC = 0xB0071DF0
ROUNDS = 10


class OpenOCD:
    def __init__(self) -> None:
        self.sock = socket.create_connection((HOST, PORT), timeout=3)
        self.sock.settimeout(3)
        self.sock.recv(4096)

    def command(self, command: str) -> str:
        # Explicit allow-list: no Flash, program, erase, load, or write other
        # than the documented DFU handoff RAM magic.
        allowed = (
            command in {"resume", "halt", "reset run", "reg pc", "reg sp", "reg gp", "reg mcause", "reg mepc"}
            or command.startswith("mww 0x20004ff0 0xb0071df0")
            or command.startswith("mdw ")
            or command.startswith("mdb ")
        )
        if not allowed:
            raise ValueError(f"refusing non-read-only/approved command: {command}")
        self.sock.sendall((command + "\n").encode())
        data = bytearray()
        deadline = time.monotonic() + 3
        while time.monotonic() < deadline:
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                break
            data.extend(chunk)
            if b"> " in data:
                break
        return data.decode(errors="replace")

    def close(self) -> None:
        self.sock.close()


def value(text: str, pattern: str) -> str:
    match = re.search(pattern, text, re.IGNORECASE)
    return match.group(1) if match else "UNKNOWN"


def snapshot(oocd: OpenOCD) -> dict[str, str]:
    commands = {
        "pc": "reg pc",
        "sp": "reg sp",
        "mcause": "reg mcause",
        "mepc": "reg mepc",
        "afio_ctlr": "mdb 0x40010018 1",
        "base_ctrl": "mdb 0x40023400 1",
        "udev_ctrl": "mdb 0x40023401 1",
        "int_en": "mdb 0x40023402 1",
        "dev_addr": "mdb 0x40023403 1",
        "mis_st": "mdb 0x40023405 1",
        "int_fg": "mdb 0x40023406 1",
        "int_st": "mdb 0x40023407 1",
        "uep0_dma": "mdw 0x40023410 1",
        "uep0_tx_len": "mdb 0x40023420 1",
        "uep0_ctrl_h": "mdb 0x40023422 1",
        "magic": "mdw 0x20004ff0 1",
    }
    result: dict[str, str] = {}
    patterns = {
        "pc": r"pc.*0x([0-9a-f]+)", "sp": r"sp.*0x([0-9a-f]+)",
        "mcause": r"mcause.*0x([0-9a-f]+)", "mepc": r"mepc.*0x([0-9a-f]+)",
    }
    for key, command in commands.items():
        output = oocd.command(command)
        if key in patterns:
            result[key] = value(output, patterns[key])
        elif key == "uep0_dma":
            result[key] = value(output, r"0x40023410:\s*([0-9a-f]+)")
        elif key == "magic":
            result[key] = value(output, r"0x20004ff0:\s*([0-9a-f]+)")
        else:
            address = {"afio_ctlr": "40010018", "base_ctrl": "40023400", "udev_ctrl": "40023401", "int_en": "40023402", "dev_addr": "40023403", "mis_st": "40023405", "int_fg": "40023406", "int_st": "40023407", "uep0_tx_len": "40023420", "uep0_ctrl_h": "40023422"}[key]
            result[key] = value(output, rf"0x{address}:\s*([0-9a-f]+)")
    return result


def host_result() -> tuple[str, str, str]:
    result = subprocess.run(["dfu-util", "-l"], capture_output=True, text=True, timeout=5)
    text = result.stdout + result.stderr
    match = re.search(r"Found DFU: \[1a86:8035\].*", text)
    if match:
        return "PASS", "1a86:8035", match.group(0).strip()
    return "FAIL", "NONE", "NONE"


def main() -> int:
    output_path = Path("artifacts/golden_repro_results.csv")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    fields = ["round", "host_result", "host_detail", "attach_delay_s", "pre_" + "dummy"]
    fields = ["round", "host_result", "host_detail", "attach_delay_s", "pre_json", "post_json"]
    print("Operator protocol: keep USB-C unplugged at startup; press Enter only after plugging it back in.")
    oocd = OpenOCD()
    try:
        with output_path.open("w", newline="") as file:
            writer = csv.DictWriter(file, fieldnames=fields)
            writer.writeheader()
            for round_no in range(1, ROUNDS + 1):
                input(f"Round {round_no}: unplugged and ready? Press Enter to establish DFU start...")
                oocd.command(f"mww 0x{MAGIC_ADDR:08x} 0x{MAGIC:08x}")
                oocd.command("reset run")
                oocd.command("resume")
                time.sleep(0.25)
                oocd.command("halt")
                pre = snapshot(oocd)
                oocd.command("resume")
                input(f"Round {round_no}: now plug USB-C, then press Enter after attach...")
                time.sleep(3)
                oocd.command("halt")
                post = snapshot(oocd)
                result, device, detail = host_result()
                writer.writerow({"round": round_no, "host_result": result, "host_detail": detail, "attach_delay_s": "3", "pre_json": pre, "post_json": post})
                file.flush()
                print(f"Round {round_no}: {result} ({device}); saved to {output_path}")
    finally:
        oocd.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
