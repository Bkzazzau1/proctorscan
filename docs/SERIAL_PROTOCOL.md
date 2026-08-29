# Diagnostics serial protocol

The stage-one protocol is newline-delimited JSON (NDJSON), UTF-8, at 115200 baud.
Each message is one JSON object followed by `\n`. Version 1 has two message types.

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

