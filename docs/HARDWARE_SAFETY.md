# Hardware safety gate

## Approved stage-one connection

Only the Waveshare ESP32-P4-WIFI6-DEV-KIT may be connected to the development
computer, using the board vendor's documented USB programming/debug port and a
known-good data cable.

## Required verification before any peripheral connection

For each physical board, record and review:

1. Exact manufacturer, model, and board revision.
2. An authoritative schematic, datasheet, or board manual.
3. Supply-voltage range and expected current.
4. I/O voltage levels and whether level shifting is present.
5. Exact connector/pin orientation; never infer it from a similar module.
6. Signal range into every ESP32-P4 input, including ADC limits.
7. A current-limited first-power procedure and common-ground plan.
8. Power-off wiring inspection before energizing the assembly.

## Explicitly disconnected

| Component | Interface expected later | Stage-one status | Verification still required |
|---|---|---|---|
| AD8318-style RF detector | Analog | Disconnected | Exact module IC/revision, supply, output range, ADC protection |
| ADXL345 breakout | I2C or SPI | Disconnected | Exact breakout schematic, regulator/level shifting, pin order |
| HLK-LD2420 V2.1 | Likely UART/GPIO | Disconnected | Manufacturer pinout, supply and logic levels, connector orientation |
| OV5647 / Pi Camera Rev 1.3 | MIPI-CSI | Disconnected | Waveshare cable/adapter compatibility and contact orientation |

No pin mapping is defined in firmware until these gates are complete.

