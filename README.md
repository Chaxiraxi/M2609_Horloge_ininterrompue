# M2609 – Uninterrupted Clock (DAB + NTP + GPS)

[Version française](README-fr.md)

## Overview

This project implements a resilient clock system on Arduino with **multiple time sources** and a clear priority strategy:

Priority order: **DAB+ > NTP > GPS**.

The system continuously evaluates sources, rejects incoherent data (±10 seconds), keeps time running through a software clock, and exposes control through:

- an **8x2 LCD + buttons + rotary encoder** user interface,
- an embedded **HTTP web interface / REST API**,
- and generated **Doxygen documentation**.

## Functional scope (cahier des charges)

Implemented objectives:

- Retrieve date/time from **NTP** (Wi-Fi)
- Retrieve date/time from **DAB+**
- Retrieve date/time from **GPS**
- Source priority: **DAB > NTP > GPS**
- Ignore incoherent source when deviation exceeds **±10 s**
- Display date/time on **LCD 8x2**
- Display synchronization errors on **LCD 8x2**
- Acknowledge errors with **SET**
- Auto-acknowledge errors after **60 s** when at least one source is valid
- **Source selection mode** with CFG/SET/encoder
- **Manual configuration mode** with long press behavior and save/cancel logic

## Main features

- **Coordinator-based synchronization** (`TimeCoordinator`)
  - Polls sources periodically
  - Checks cross-source coherence
  - Selects the best valid source by priority
  - Maintains an internal software clock between sync cycles
- **Source abstraction** (`TimeSource`)
  - `DABTimeSource`
  - `NTPTimeSource`
  - `GPSTimeSource`
- **Error model and handling** (`SyncErrors`)
  - Per-source and global errors
  - Manual acknowledge and timeout-based auto-ack
- **Dual control UX**
  - Local hardware UI (`UiController`)
  - Remote control through `RestApiServer`

## Hardware and platform

- Primary board target: **Arduino UNO R4 WiFi**
- Also configured in tasks: **UNO R4 Minima** build/upload tasks
- Main peripherals used in code:
  - DAB shield (SPI)
  - GPS module (SoftwareSerial)
  - LCD 8x2
  - MCP23017 I/O expander (encoder + CFG + LCD lines)
  - SET button (GPIO)

Pin mapping is centralized in `src/platform/PinDefinitions.hpp`.

## Software architecture

Top-level structure:

- `DabGps.ino`: composition root (`setup()` / `loop()`)
- `src/time/`: source implementations + coordination logic
- `src/ui/`: state machine for LCD and controls
- `src/network/`: Wi-Fi manager + embedded web server / REST
- `src/core/`: errors and notification/logging
- `src/platform/`: pin definitions
- `src/config/`: local secrets/config headers

## REST API and web interface

The embedded server (default port `80`) serves:

- `GET /` → embedded web UI
- `GET /status` → JSON status (source states + time set state)
- `GET /logs` → latest 20 in-memory logs (newest first)
- `POST /toggle-source` → enable/disable one source
- `POST /set-time` → set manual date/time
- `POST /set-timezone` → set timezone offset (applied to NTP and GPS sources)

The web UI allows source toggling and manual time setting from a browser on the same network.
It also includes a logs panel with:

- level threshold filter (`DEBUG`, `INFO`, `WARNING`, `ERROR`) with default `INFO`,
- configurable logs polling interval in seconds (minimum `1` second),
- display format: `timestamp + level + message`.

## Build and upload

### VS Code tasks (already configured)

- `Compile Arduino Code (WiFi)`
- `Upload Arduino Code (WiFi)`
- `Compile Arduino Code (minima)`
- `Upload Arduino Code (minima)`

### Arduino CLI (equivalent)

Compile for UNO R4 WiFi:

```bash
arduino-cli compile -b arduino:renesas_uno:unor4wifi --build-path ./.build
```

Upload for UNO R4 WiFi (example port):

```bash
arduino-cli upload -p COM11 -b arduino:renesas_uno:unor4wifi --input-dir ./.build
```

## Configuration

Create `src/config/SECRETS.hpp` with your local credentials:

```cpp
constexpr char WIFI_SSID[] = "your-ssid";
constexpr char WIFI_PASSWORD[] = "your-password";
```

`src/config/SECRETS.hpp` is ignored by Git (`.gitignore`).

## Doxygen documentation

Generate docs with:

```bash
doxygen Doxyfile
```

Optional PDF (Windows batch in this project):

```bash
./docs/latex/make.bat
```

The main Doxygen front page is mapped to this `README.md`.

## Possible future improvements

- Captive portal for runtime configuration:
  - Timezone
  - Wi-Fi credentials
  - NTP server address
  - Source enable/disable
- Alarm-clock extension using DAB audio station playback.
- Inter-Arduino synchronization network over Bluetooth.

## Author

David GOLETTA  
CPNV – M2609
