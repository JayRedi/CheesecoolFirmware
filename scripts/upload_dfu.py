#!/usr/bin/env python3
"""Upload an application only after non-software DFU entry is already active."""
import argparse
import subprocess
import sys

DFU_ID = "1a86:8035"

def dfu_present():
    result = subprocess.run(["dfu-util", "-l"], text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, check=False)
    return DFU_ID in result.stdout

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--application", required=True)
    args = parser.parse_args()
    if not dfu_present():
        raise RuntimeError(
            "CheeseCool DFU device 1a86:8035 is not present; use the existing "
            "non-software bootloader-entry procedure first. This uploader never sends HID 0x08 or 0x0D."
        )
    print("DFU device already present")
    command = ["dfu-util", "-a", "0", "-d", DFU_ID, "-D", args.application, "-R"]
    print(" ".join(command))
    subprocess.run(command, check=True)
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, OSError, subprocess.CalledProcessError) as exc:
        print("upload error: %s" % exc, file=sys.stderr)
        raise SystemExit(1)
