# Staged integration plan

## Stage 1 — controller bring-up (implemented)

- Build and flash minimal ESP-IDF firmware.
- Report identity, firmware version, and heartbeat over USB serial.
- Parse and display status locally.
- Keep all peripherals disconnected.

Exit criterion: repeatable `ONLINE` status and stable heartbeats through reconnects.

## Stage 2 — hardware verification (documentation only)

- Gather authoritative documents for each exact board revision.
- Create reviewed power budgets, pin maps, and signal-level checks.
- Bench-verify supplies before attaching signals.

Exit criterion: signed-off wiring sheet for one peripheral at a time.

## Stage 3 — one-peripheral drivers

- Add the lowest-risk verified digital sensor first.
- Use a simulator/test fixture before physical connection where practical.
- Extend the serial protocol with health and raw diagnostic values.
- Do not add detection judgments yet.

## Stage 4 — camera and local edge processing

- Validate the exact MIPI-CSI cable and orientation.
- Add local capture diagnostics, then bounded on-device/local inference.
- Do not upload frames or inference data to cloud services.

## Stage 5 — calibrated sensor fusion

- Establish baselines and false-positive tests.
- Combine independent signals into explainable confidence indicators.
- Treat output as an aid for review, not an automatic accusation.

