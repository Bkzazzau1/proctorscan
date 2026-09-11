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
{"protocol":1,"type":"storage_status","component":"microsd","mode":"new_file_only","state":"DETECTED","capacity_bytes":32000000000}
```

Valid states are `DETECTED`, `MOUNT_ERROR`, and `INIT_ERROR`. Formatting remains
disabled.

Firmware `0.5.0` reports the controlled local-log test separately:

```json
{"protocol":1,"type":"log_status","component":"microsd","state":"VERIFIED","path":"/sdcard/proctorscan/bringup-e3ac0e-000.log"}
```

The logging stage mounts without formatting and only creates a new file using
exclusive creation. Existing files are never overwritten or deleted.

Firmware `0.6.0` reports the passive tamper microswitch connected between GPIO5
(physical header pin 13) and GND (physical header pin 14):

```json
{"protocol":1,"type":"tamper_switch","gpio":5,"physical_pin":13,"level":0,"contact":"CLOSED"}
```

The GPIO uses its internal pull-up. `CLOSED` means the two selected switch
terminals are electrically connected; `OPEN` means they are disconnected. The
firmware deliberately does not interpret either state as tampering until the
enclosure's normal resting position is established.

The ADXL345-specific simulation and hardware-probe messages have been removed
because the ADXL345 is not part of the prototype. MPU6050 support will be added as
a separately reviewed protocol extension only after its exact hardware is verified.

Firmware `0.7.0` adds the LD2420's 3.3 V `OT2` presence output on GPIO4
(physical header pin 16). UART remains disconnected during this stage:

```json
{"protocol":1,"type":"radar_presence","component":"hlk-ld2420-v2.1","signal":"OT2","gpio":4,"physical_pin":16,"level":1,"presence":true}
```
