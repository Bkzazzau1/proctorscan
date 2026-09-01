# Diagnostics serial protocol

The stage-one protocol is newline-delimited JSON (NDJSON), UTF-8, at 115200 baud.
Each message is one JSON object followed by `\n`. Version 1 currently supports only
the controller identity, heartbeat, and read-only storage-status messages below.

Identity is emitted at startup and periodically so a monitor connected after boot
can still learn the device identity:

```json
{"protocol":1,"type":"identity","device_id":"proctorscan-0123456789ab","board":"waveshare-esp32-p4-wifi6-dev-kit","firmware":"0.1.0"}
```

Heartbeat is emitted every two seconds:

```json
{"protocol":1,"type":"heartbeat","sequence":4,"uptime_ms":8000}
```

The device ID is derived locally from the ESP32 base MAC address. It is an identifier,
not an authentication secret. Consumers must ignore malformed lines and unknown
protocol versions. Stage one accepts no commands and performs no actuation.

The built-in microSD slot can report an identification-only result:

```json
{"protocol":1,"type":"storage_status","component":"microsd","mode":"identification_only","state":"DETECTED","capacity_bytes":32000000000}
```

Valid states are `DETECTED`, `NOT_DETECTED`, and `INIT_ERROR`. The firmware reads
card metadata only; it does not mount, format, or modify the card filesystem.

The ADXL345-specific simulation and hardware-probe messages have been removed
because the ADXL345 is not part of the prototype. MPU6050 support will be added as
a separately reviewed protocol extension only after its exact hardware is verified.
