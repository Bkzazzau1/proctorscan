"""Declared hardware readiness state; this module never accesses GPIO."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class HardwareItem:
    name: str
    state: str
    next_gate: str


HARDWARE_ITEMS = (
    HardwareItem("ESP32-P4 controller", "VERIFIED / USB ONLY", "Keep diagnostics operational"),
    HardwareItem("ADXL345 accelerometer", "DISCONNECTED / UNVERIFIED", "Confirm exact breakout and markings"),
    HardwareItem("AD8318-style RF detector", "DISCONNECTED / UNVERIFIED", "Confirm exact module and output range"),
    HardwareItem("HLK-LD2420 V2.1 radar", "DISCONNECTED / UNVERIFIED", "Confirm manufacturer pinout and levels"),
    HardwareItem("OV5647 / Pi Camera V1.3", "DISCONNECTED / UNVERIFIED", "Confirm cable, adapter, and orientation"),
)


def hardware_status_text() -> str:
    lines = ["ProctorScan hardware readiness", ""]
    for item in HARDWARE_ITEMS:
        lines.extend((item.name, f"  Status: {item.state}", f"  Next: {item.next_gate}"))
    lines.extend(("", "No peripheral pin mapping is approved."))
    return "\n".join(lines)
