# ADXL345 verification gate

The ADXL345 is the first candidate peripheral, but it must remain disconnected.
The chip name alone is not enough to determine safe wiring because breakout boards
can differ in regulators, level shifting, pull-ups, pin order, and labeling.

## Evidence needed

- Clear, straight-on photographs of both sides of the exact breakout board.
- A readable close-up of every marking and connector label.
- Manufacturer or seller name and exact product link, if available.
- The schematic/manual for that exact board revision.

## Review before wiring

- Match the PCB layout and revision to authoritative documentation.
- Record supply range separately from I/O logic voltage.
- Confirm whether a regulator and I2C/SPI level shifting are fitted.
- Confirm connector orientation and each printed pin label.
- Select I2C or SPI only after the breakout design is known.
- Produce a reviewed ESP32-P4-to-breakout wiring table.
- Inspect the unpowered wiring before first current-limited power-up.

## Current decision

Status: **DISCONNECTED / UNPOWERED CHECK PASSED**

Photographs confirmed the eight labels and a component-free rear side. An unpowered
continuity test displayed `OL` between `VCC` and `GND`, so no direct short was
detected. The supplied header is not soldered; loose jumper contact is not approved.

No powered physical test is approved yet. The temporary wiring uses the documented
3.3 V I2C arrangement, but its unsoldered contacts still require visual inspection.

## Software probe prepared

A disabled-by-default identification probe is implemented for the documented
Waveshare I2C bus (`SDA/GPIO7`, `SCL/GPIO8`) and ADXL345 address `0x53`. It reads
only the device ID register and requires `0xE5` before reporting `DETECTED`.
