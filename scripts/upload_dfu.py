#!/usr/bin/env python3
"""Upload an application through CheeseCool HID -> DFU, without ROM ISP."""
import argparse
import subprocess
import sys
import time

DFU_ID = "1a86:8035"
HID_VID = 0x1A86
HID_PID = 0xFE01

def dfu_present():
    result = subprocess.run(["dfu-util", "-l"], text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, check=False)
    return DFU_ID in result.stdout

def hid_present():
    try:
        import hid
        return bool(hid.enumerate(HID_VID, HID_PID))
    except ImportError:
        return False

def request_dfu():
    from pathlib import Path
    cli = Path(__file__).resolve().parents[1] / "tools" / "cheesecoolctl" / "cheesecoolctl.py"
    result = subprocess.run([sys.executable, str(cli), "enter-dfu"], check=False)
    if result.returncode:
        raise RuntimeError("HID ENTER_DFU request failed")

def wait_for_dfu(expected):
    deadline = time.monotonic() + 5.0
    while time.monotonic() < deadline:
        if dfu_present():
            return
        time.sleep(0.2)
    raise RuntimeError("DFU device 1a86:8035 did not appear within 5 seconds")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--application", required=True)
    args = parser.parse_args()
    if dfu_present():
        print("DFU device already present")
    elif hid_present():
        print("CheeseCool HID present; requesting DFU")
        request_dfu()
        wait_for_dfu(args.application)
    else:
        raise RuntimeError("neither CheeseCool HID nor DFU device is present")
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
