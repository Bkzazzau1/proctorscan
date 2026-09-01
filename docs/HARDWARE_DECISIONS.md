# Hardware decisions

## Motion/tamper sensor

The MPU6050 is the selected motion/tamper sensor for the ProctorScan Halo
prototype. The ADXL345 is excluded and must not be connected, probed, or developed
as part of the prototype.

The earlier ADXL345 read-only experiment returned `NOT_DETECTED`; its implementation
has been removed. This historical result does not affect the ESP32-P4 identity and
heartbeat diagnostics.

The MPU6050 remains disconnected. No pin mapping or driver is approved until the
exact received breakout, pinout, supply voltage, logic levels, and I2C requirements
are verified from authoritative documentation.

## MicroSD storage

Firmware `0.4.0` performed a metadata-only SDMMC initialization using the official
Waveshare ESP32-P4 pin mapping. It did not mount or modify the filesystem. The first
physical test returned `NOT_DETECTED` while USB identity and heartbeat diagnostics
remained healthy. Review of Waveshare's official example identified the missing
board-specific power step: the SD host must use internal LDO channel 4. Firmware
`0.4.1` adds that power-control handle while retaining metadata-only operation.
The subsequent physical test detected the card successfully at 16.36 GB and
maintained stable controller heartbeats for more than one minute.

Firmware `0.5.0` then mounted the existing filesystem with formatting disabled,
created a new file using exclusive creation, wrote device identity and three
heartbeats, and read the file back successfully. The verified physical-test file
was `/sdcard/proctorscan/bringup-e3ac0e-001.log`. Existing files were neither
overwritten nor deleted.
