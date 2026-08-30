# Diagnostics serial protocol

The stage-one protocol is newline-delimited JSON (NDJSON), UTF-8, at 115200 baud.
Each message is one JSON object followed by `\n`. Version 1 supports the controller
messages below plus an explicitly simulated accelerometer message.

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

An ADXL345 simulation can exercise the future display path without GPIO access:

```json
{"protocol":1,"type":"accelerometer","source":"simulation","x_g":0.01,"y_g":-0.005,"z_g":1.0}
```

The version-one monitor rejects accelerometer messages unless `source` is exactly
`simulation`. Real sensor data remains disabled until the physical connection gate
and firmware driver review are complete.

The firmware contains the build option `PROCTORSCAN_ADXL345_SIMULATOR`, which
defaults to off. Even when enabled it performs no I2C or GPIO access.

The separately gated physical probe reports whether register `0x00` contains the
ADXL345 device ID `0xE5`:

```json
{"protocol":1,"type":"peripheral_status","component":"adxl345","source":"hardware","state":"DETECTED","address":83,"device_id":229}
```

Possible states are `DETECTED`, `NOT_DETECTED`, `ID_MISMATCH`, and `BUS_ERROR`.
`PROCTORSCAN_ADXL345_PROBE` defaults to off and performs identification only; it
does not place the accelerometer into measurement mode.
