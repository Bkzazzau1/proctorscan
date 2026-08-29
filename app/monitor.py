"""Console hardware status screen for an attached ProctorScan controller."""

from __future__ import annotations

import argparse
import json
import logging
import sys
import time

from .protocol import DeviceStatus, ProtocolError, parse_line


LOG = logging.getLogger("proctorscan.hardware")


def print_ports() -> int:
    try:
        from serial.tools import list_ports
    except ImportError:
        LOG.error("Serial support is not installed. Run: py -m pip install -r app\\requirements.txt")
        return 2

    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return 1
    for port in ports:
        print(f"{port.device}: {port.description}")
    return 0


def print_status(status: DeviceStatus) -> None:
    print(f"Device: {status.device_id or 'waiting for identity'}")
    print(f"Board: {status.board or 'unknown'}")
    print(f"Firmware: {status.firmware or 'unknown'}")
    print(f"Status: {status.connection()}")
    if status.heartbeat_sequence is not None:
        print(
            f"Heartbeat: {status.heartbeat_sequence}  "
            f"uptime={status.uptime_ms // 1000}s"
        )
    print("-")


def monitor(port: str, baud: int) -> int:
    try:
        import serial
    except ImportError:
        LOG.error("Serial support is not installed. Run: py -m pip install -r app\\requirements.txt")
        return 2

    status = DeviceStatus()
    print("ProctorScan hardware monitor")
    print(f"Opening {port} at {baud} baud. Press Ctrl+C to stop.")
    try:
        with serial.Serial(port, baudrate=baud, timeout=1) as connection:
            while True:
                line = connection.readline()
                if not line:
                    if status.connection() == "STALE":
                        LOG.warning("Device heartbeat is stale")
                    continue
                try:
                    message = parse_line(line)
                    status.update(message)
                except ProtocolError as exc:
                    LOG.debug("Ignoring non-protocol serial line: %s", exc)
                    continue
                print_status(status)
    except serial.SerialException as exc:
        LOG.error("Unable to use serial port %s: %s", port, exc)
        return 2
    except KeyboardInterrupt:
        print("\nMonitor stopped.")
        return 0


def simulate(count: int, interval: float) -> int:
    """Exercise the real parser/status path without attached hardware."""
    status = DeviceStatus()
    identity = {
        "protocol": 1,
        "type": "identity",
        "device_id": "proctorscan-simulator",
        "board": "waveshare-esp32-p4-wifi6-dev-kit",
        "firmware": "0.1.0-simulated",
    }
    status.update(parse_line(json.dumps(identity)))
    print("ProctorScan hardware monitor (simulation)")
    print_status(status)

    for sequence in range(1, count + 1):
        if interval:
            time.sleep(interval)
        heartbeat = {
            "protocol": 1,
            "type": "heartbeat",
            "sequence": sequence,
            "uptime_ms": sequence * 2000,
        }
        status.update(parse_line(json.dumps(heartbeat)))
        print_status(status)
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true", help="list serial ports")
    parser.add_argument("--port", help="serial port, for example COM7")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--simulate", action="store_true", help="run without hardware")
    parser.add_argument("--count", type=int, default=3, help="simulated heartbeat count")
    parser.add_argument("--interval", type=float, default=0.5, help="simulation delay in seconds")
    parser.add_argument("--verbose", action="store_true")
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
    )
    if args.list:
        return print_ports()
    if args.simulate:
        if args.count < 0 or args.interval < 0:
            parser.error("--count and --interval must not be negative")
        return simulate(args.count, args.interval)
    if not args.port:
        print("Specify --port COM_PORT or use --list.", file=sys.stderr)
        return 2
    return monitor(args.port, args.baud)


if __name__ == "__main__":
    raise SystemExit(main())
