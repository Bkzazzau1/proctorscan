"""Console hardware status screen for an attached ProctorScan controller."""

from __future__ import annotations

import argparse
import logging
import sys
import time

import serial
from serial.tools import list_ports

from .protocol import DeviceStatus, ProtocolError, parse_line


LOG = logging.getLogger("proctorscan.hardware")


def print_ports() -> int:
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


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true", help="list serial ports")
    parser.add_argument("--port", help="serial port, for example COM7")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--verbose", action="store_true")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
    )
    if args.list:
        return print_ports()
    if not args.port:
        print("Specify --port COM_PORT or use --list.", file=sys.stderr)
        return 2
    return monitor(args.port, args.baud)


if __name__ == "__main__":
    raise SystemExit(main())

