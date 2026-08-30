#!/usr/bin/env python3
"""Small macOS/Linux hidapi client for CheeseCool V1."""
import argparse
import subprocess
import sys
import time

VID = 0x1A86
PID = 0xFE01
REPORT_SIZE = 64

def checksum(report):
    value = 0
    for byte in report[:63]:
        value ^= byte
    return value

def make_request(command, sequence=0, payload=b""):
    report = bytearray(REPORT_SIZE)
    report[0] = 1
    report[1] = command
    report[2] = sequence & 0xFF
    report[3] = len(payload)
    report[4:4 + len(payload)] = payload
    report[63] = checksum(report)
    return report

def open_device():
    try:
        import hid
    except ImportError as exc:
        raise RuntimeError("Install hidapi first: python3 -m pip install hidapi") from exc
    devices = hid.enumerate(VID, PID)
    if not devices:
        raise RuntimeError("CheeseCool HID device not found (VID:PID 1a86:fe01)")
    device = hid.device()
    device.open_path(devices[0]["path"])
    device.set_nonblocking(False)
    return device

def transact(command, payload=b""):
    device = open_device()
    try:
        request = make_request(command, payload=payload)
        # hidapi expects a leading report-ID byte when the descriptor has no ID.
        device.write(bytes([0]) + request)
        response = bytes(device.read(REPORT_SIZE, 2000))
        if len(response) != REPORT_SIZE:
            raise RuntimeError("short HID response")
        if response[63] != checksum(response):
            raise RuntimeError("HID response checksum mismatch")
        return response
    finally:
        device.close()

def print_status(response):
    if response[4] != 0:
        raise RuntimeError("GET_STATUS failed with status %d" % response[4])
    length = response[3]
    if length < 17:
        raise RuntimeError("short GET_STATUS payload")
    payload = response[5:5 + length]
    modes = {0: "HOST_CONTROLLED", 1: "MAX"}
    print("Mode: %s" % modes.get(payload[0], "N/A"))
    print("Target duty: %s" % ("%d %%" % payload[1] if payload[1] <= 100 else "N/A"))
    print("PWM duty: %s" % ("%d %%" % payload[2] if payload[2] <= 100 else "N/A"))
    print("Fan RPM: %d rpm" % int.from_bytes(payload[3:7], "little"))
    print("USB connected: %s" % ("yes" if payload[7] else "no"))
    print("Failsafe: %s" % ("yes" if payload[8] else "no"))
    print("Power fault: %s" % ("yes" if payload[9] else "no"))
    print("Uptime: %d ms" % int.from_bytes(payload[10:14], "little"))
    print("Firmware: %d.%d.%d" % (payload[14], payload[15], payload[16]))

def make_curve_payload(arguments):
    if not arguments or len(arguments) > 29:
        raise ValueError("curve requires 1 to 29 points")
    payload = bytearray([len(arguments)])
    previous_temperature = None
    for item in arguments:
        try:
            temperature, duty = item.split(":", 1)
            temperature, duty = int(temperature), int(duty)
        except (AttributeError, ValueError):
            raise ValueError("curve points must use temperature:duty")
        if temperature < 0 or temperature > 125 or duty < 0 or duty > 100:
            raise ValueError("temperature must be 0..125 and duty must be 0..100")
        if previous_temperature is not None and temperature <= previous_temperature:
            raise ValueError("curve temperatures must be strictly increasing")
        payload.extend((temperature, duty))
        previous_temperature = temperature
    return payload

def wait_for_dfu(timeout_seconds=8):
    deadline = time.time() + timeout_seconds
    while time.time() < deadline:
        result = subprocess.run(["dfu-util", "-l"], capture_output=True, text=True, timeout=2)
        if "1a86:8035" in result.stdout.lower():
            return True
        time.sleep(0.2)
    return False

def main():
    parser = argparse.ArgumentParser(prog="cheesecoolctl")
    parser.add_argument("command", choices=("ping", "info", "status", "mode", "duty", "curve", "dfu", "enter-dfu"))
    parser.add_argument("argument", nargs="*")
    args = parser.parse_args()
    if args.command == "mode":
        if len(args.argument) != 1 or args.argument[0] not in ("host", "max"):
            parser.error("mode requires host or max")
        response = transact(10, {"host": bytes([0]), "max": bytes([1])}[args.argument[0]])
        if response[4] != 0:
            raise RuntimeError("SET_MODE failed with status %d" % response[4])
        print("Mode set to %s" % ("HOST_CONTROLLED" if args.argument[0] == "host" else "MAX"))
        return 0
    if args.command == "curve":
        try:
            payload = make_curve_payload(args.argument)
        except ValueError as exc:
            parser.error(str(exc))
        response = transact(12, payload)
        if response[4] != 0:
            raise RuntimeError("SET_CURVE failed with status %d" % response[4])
        accepted = response[5:5 + response[3]]
        print("Fan curve updated:")
        for index in range(accepted[0]):
            print("%d C -> %d %%" % (accepted[1 + 2 * index], accepted[2 + 2 * index]))
        return 0
    if args.command == "dfu":
        response = transact(13)
        if response[4] != 0:
            raise RuntimeError("ENTER_DFU failed with status %d" % response[4])
        print("Entering DFU...")
        print("Application disconnected.")
        if not wait_for_dfu():
            raise RuntimeError("ENTER_DFU accepted but DFU device was not detected")
        print("DFU device detected: 1A86:8035")
        return 0
    if args.command == "duty":
        try:
            duty = int(args.argument[0])
        except (IndexError, TypeError, ValueError):
            parser.error("duty requires an integer from 0 to 100")
        if duty < 0 or duty > 100:
            parser.error("duty requires an integer from 0 to 100")
        response = transact(11, bytes([duty]))
        if response[4] != 0:
            raise RuntimeError("SET_DUTY failed with status %d" % response[4])
        print("Duty set to %d %%" % duty)
        return 0
    response = transact({"ping": 1, "info": 2, "status": 9, "enter-dfu": 8}[args.command])
    if args.command == "status":
        print_status(response)
        return 0
    print("response: status=%d sequence=%d payload=%s" %
          (response[4], response[2], response[4:4 + response[3]].hex()))
    if args.command == "enter-dfu":
        print("DFU request accepted; waiting for USB DFU enumeration")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        raise SystemExit(1)
