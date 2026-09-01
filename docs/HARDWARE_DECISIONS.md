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
remained healthy. Inspect the card type, orientation, and full insertion before
repeating the test.
