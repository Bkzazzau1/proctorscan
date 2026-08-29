# ProctorScan

ProctorScan is starting with one deliberately narrow hardware milestone:

> ESP32-P4 → USB serial → local diagnostics monitor → device online

No external sensors or camera are connected by this first stage. The AD8318-style
RF detector, ADXL345, HLK-LD2420 V2.1, and OV5647 camera remain electrically
disconnected until the exact board revisions, pinouts, signal levels, connector
orientation, and supply requirements are verified from authoritative documentation.

All processing is intended to remain local/edge. This repository currently contains
no cloud service, telemetry, account, or remote inference dependency.

## Repository layout

- `firmware/esp32p4-diagnostics/` — ESP-IDF firmware that emits newline-delimited
  JSON identity and heartbeat messages over the configured console/USB serial port.
- `app/` — local, command-line hardware status monitor.
- `tests/` — protocol/parser tests that require no attached hardware.
- `docs/` — staged integration plan, safety gate, and serial protocol.

There is no desktop/mobile application or backend yet; the local monitor is the
smallest safe application surface for initial bring-up.

## Stage 1 setup

### Verified bring-up

Stage 1 was verified on a physical Waveshare ESP32-P4-WIFI6-DEV-KIT using
ESP-IDF v6.1 and the board's native USB Serial/JTAG interface. The firmware
build, flash verification, device identity message, and repeating two-second
heartbeats all completed successfully.

The serial port name is assigned by Windows and may differ between computers.
Always identify the Espressif USB Serial/JTAG port with the command below; do
not copy a COM port number from another machine.

### Try the status screen without hardware

The simulator uses the same parser and status model as a real board and requires no
extra Python packages:

```powershell
py -m app.monitor --simulate
```

Include clearly labeled simulated ADXL345 readings (no GPIO or sensor required):

```powershell
py -m app.monitor --simulate --simulate-adxl
```

Show the current controller and peripheral safety/readiness states:

```powershell
py -m app.monitor --hardware-status
```

This is the quickest way to confirm the local diagnostics feature before installing
ESP-IDF or connecting the development board.

### 1. Install development tools

- Install ESP-IDF v6.1 (the version used for the verified bring-up).
- Install Python 3.10 or newer.
- Create a Python virtual environment, then install the local monitor dependency:

```powershell
py -m venv .venv
.venv\Scripts\Activate.ps1
py -m pip install -r app\requirements.txt
```

### 2. Build and flash firmware

Open an ESP-IDF terminal:

```powershell
cd firmware\esp32p4-diagnostics
idf.py set-target esp32p4
idf.py build
idf.py -p COM_PORT flash
```

Replace `COM_PORT` with the board's programming/debug serial port, for example
`COM7`. Use only the Waveshare-documented USB port and boot/flash procedure for the
exact ESP32-P4-WIFI6-DEV-KIT revision. Do not connect any peripheral modules.

### 3. Run the local status monitor

List candidate serial ports:

```powershell
py -m app.monitor --list
```

Start monitoring:

```powershell
py -m app.monitor --port COM_PORT
```

Expected output resembles:

```text
ProctorScan hardware monitor
Device: proctorscan-0123456789ab
Board: waveshare-esp32-p4-wifi6-dev-kit
Firmware: 0.1.0
Status: ONLINE
Heartbeat: 4  uptime=12s
```

The monitor changes the status to `STALE` if no valid heartbeat arrives for more
than five seconds. Press Ctrl+C to stop it.

## Verify without hardware

```powershell
py -m unittest discover -s tests -v
```

## Hardware safety gate

For this stage, connect only the ESP32-P4 development board to the computer with
the correct USB cable. Leave these disconnected:

- AD8318-style RF detector module
- ADXL345 breakout
- HLK-LD2420 V2.1 radar
- OV5647 / Raspberry Pi Camera Rev 1.3

See [docs/HARDWARE_SAFETY.md](docs/HARDWARE_SAFETY.md) before expanding the build.
The first peripheral verification worksheet is
[docs/ADXL345_VERIFICATION.md](docs/ADXL345_VERIFICATION.md).
