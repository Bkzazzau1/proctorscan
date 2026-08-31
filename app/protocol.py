"""Parser and state model for the ProctorScan diagnostics serial protocol."""

from __future__ import annotations

from dataclasses import dataclass
import json
import time
from typing import Any


PROTOCOL_VERSION = 1


class ProtocolError(ValueError):
    """Raised when a serial line is not a valid diagnostics message."""


def parse_line(line: bytes | str) -> dict[str, Any]:
    if isinstance(line, bytes):
        try:
            line = line.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise ProtocolError("message is not valid UTF-8") from exc

    try:
        message = json.loads(line)
    except json.JSONDecodeError as exc:
        raise ProtocolError("message is not valid JSON") from exc

    if not isinstance(message, dict):
        raise ProtocolError("message must be a JSON object")
    if message.get("protocol") != PROTOCOL_VERSION:
        raise ProtocolError("unsupported protocol version")
    if message.get("type") not in {"identity", "heartbeat"}:
        raise ProtocolError("unsupported message type")
    return message


@dataclass
class DeviceStatus:
    device_id: str | None = None
    board: str | None = None
    firmware: str | None = None
    heartbeat_sequence: int | None = None
    uptime_ms: int | None = None
    last_seen_monotonic: float | None = None

    def update(self, message: dict[str, Any], now: float | None = None) -> None:
        now = time.monotonic() if now is None else now
        if message["type"] == "identity":
            required = ("device_id", "board", "firmware")
            if not all(isinstance(message.get(field), str) for field in required):
                raise ProtocolError("identity message is missing string fields")
            self.device_id = message["device_id"]
            self.board = message["board"]
            self.firmware = message["firmware"]
        elif message["type"] == "heartbeat":
            if not isinstance(message.get("sequence"), int):
                raise ProtocolError("heartbeat sequence must be an integer")
            if not isinstance(message.get("uptime_ms"), int):
                raise ProtocolError("heartbeat uptime_ms must be an integer")
            self.heartbeat_sequence = message["sequence"]
            self.uptime_ms = message["uptime_ms"]
        self.last_seen_monotonic = now

    def connection(self, now: float | None = None, stale_after: float = 5.0) -> str:
        if self.last_seen_monotonic is None:
            return "WAITING"
        now = time.monotonic() if now is None else now
        return "ONLINE" if now - self.last_seen_monotonic <= stale_after else "STALE"
