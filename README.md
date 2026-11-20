# Super Flat Floor Meter

An ESP32-based portable device that measures floor flatness/levelness for **forklift safety monitoring**. The unit is driven (or rolled) across a floor, continuously tracking pitch and roll angles plus travel distance, and flags sections where the floor slope exceeds forklift mast safety limits.

## Features

- **Real-time telemetry** — pitch, roll, and distance updated live on a built-in web dashboard
- **Forklift safety monitoring** — automatic NORMAL / WARNING / DANGER classification against configurable safety limits
- **CSV data logging** — every sample written to LittleFS for later export
- **Web dashboard** — served from an ESP32 Wi-Fi access point (no router needed), with a live canvas chart of pitch/roll plus safety-limit lines and limit-exceedance markers
- **Serial simulation mode** — test the full pipeline (logging, dashboard, safety logic) without the wheel hardware
- **On-the-fly gyro re-calibration** — trigger from the dashboard without reflashing

## Hardware

| Component | Details |
|-----------|---------|
| MCU | ESP32 (custom I2C pin mapping: SDA = 21, SCL = 22, 400 kHz) |
| IMU | MPU9250 (I2C address `0x68`) — 9-axis, used for gyro + accelerometer |
| Angle encoder | AS5600 magnetic rotary encoder (I2C address `0x36`) — 12-bit raw angle, 12-bit mode |
| Wheel | 100 mm diameter (configurable in `Config.h`) |
| Storage | LittleFS (on-board flash) |

## Safety Thresholds

| Status | Pitch (longitudinal) | Roll (transverse) |
|--------|----------------------|-------------------|
| DANGER | ≥ 1.0° | ≥ 0.5° |
| WARNING | ≥ 0.6° | ≥ 0.3° |
| NORMAL | < 0.6° | < 0.3° |

The same limits are enforced in both the firmware (CSV log `Safety_Status` column) and the web UI (safety panel + chart markers).

## Getting Started

### Build & Flash

1. Install the **Arduino IDE** with the **esp32** board package, or use the **Arduino CLI**.
2. Required libraries (all come with the esp32 core except as noted): `WiFi`, `WebServer`, `LittleFS`, `Wire`.
3. Select your ESP32 board and COM port.
4. Open `FloorMeter.ino` and click **Upload**.

### Web Dashboard

1. Power on the device and connect to the Wi-Fi access point **`SuperFlat_FloorMeter`** (password: `123456789`).
2. Open the IP address printed on the Serial console (typically `192.168.4.1`) in a browser.
3. Use the dashboard to:
   - **Start / Stop** a measurement session (Start clears the log and resets the odometer)
   - **Re-Calibrate** the gyro (keep the device still on the floor first)
   - **Download CSV** — stream the logged data file from the device
   - Watch the live pitch/roll chart with safety-limit lines

### Serial Simulation Mode

To test without the physical wheel, drive the Serial monitor (115200 baud):

1. Send a speed value in **m/s** (e.g. `1.5`) — this auto-initializes a new session and simulates 10 seconds of travel at that speed, logging as it goes.
2. After 10 seconds the run stops automatically.
3. Real encoder readings are always summed with the simulated distance, so a stationary encoder + simulated speed = a clean simulated run.

## CSV Output

Logged to LittleFS at `/floor_data.csv`, one row every **300 ms** while measuring:

```
Timestamp(ms),Distance(m),Pitch(deg),Roll(deg),Safety_Status
0,0.000,0.0,0.0,NORMAL
300,0.045,0.1,-0.1,NORMAL
...
```

- `Timestamp` — milliseconds since session start (relative)
- `Safety_Status` — `NORMAL` / `WARNING` / `DANGER` per the thresholds above

## HTTP API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Dashboard (HTML) |
| `/getdata` | GET | Telemetry JSON: `{"dist":…,"pitch":…,"roll":…,"measuring":true/false}` |
| `/action?cmd=start` | GET | Reset log + odometer, start measuring. Returns `{"measuring":true}` |
| `/action?cmd=stop` | GET | Stop measuring. Returns `{"measuring":false}` |
| `/action?cmd=recal` | GET | Re-calibrate gyro offsets (device must be still) |
| `/download` | GET | Download `/floor_data.csv` |

## Project Structure

| File | Role |
|------|------|
| `FloorMeter.ino` | Main entry point — wires modules together, measurement loop, CSV logging, serial simulation mode |
| `Config.h` | Wi-Fi AP credentials, CSV filename, wheel diameter |
| `SensorEncoder.h` | AS5600 angle encoder driver — 12-bit angle deltas with wrap-around, distance conversion, simulated-speed support |
| `SensorIMU.h` | MPU9250 driver — gyro calibration, complementary filter (0.998 gyro / 0.002 accel), moving-average smoothing, deadband |
| `StorageManager.h` | LittleFS CSV logging — init, append, clear |
| `HttpServerManager.h` | ESP32 Wi-Fi access point + WebServer (dashboard, actions, telemetry, download) |
| `WebHtml.h` | Dashboard HTML/CSS (PROGMEM) |
| `WebScript.h` | Dashboard JS — live canvas chart, safety panel, button state |

## Configuration

All tunable values live in `Config.h`:

| Constant | Default | Description |
|----------|---------|-------------|
| `AP_SSID` | `SuperFlat_FloorMeter` | Wi-Fi access point name |
| `AP_PASS` | `123456789` | Wi-Fi access point password |
| `DATA_FILE` | `/floor_data.csv` | LittleFS log file path |
| `WHEEL_DIAMETER_MM` | `100.0` | Actual wheel diameter in millimeters |

Safety thresholds (pitch 1.0° / roll 0.5°) are defined in both `FloorMeter.ino` (firmware logging) and `WebScript.h` (dashboard); update both together if you change them.

## Notes & Limitations

- **Gyro-only calibration** — `calibrate()` samples gyro offsets only; it does not zero the accelerometer pitch/roll. If the device boots tilted, the complementary filter slowly pulls the angle back toward the accel reference over time.
- **Deadband** — pitch/roll readings with magnitude < 0.1° are clamped to 0 to suppress jitter.
- **10-sample moving average** — applied to pitch/roll before display/logging.
- **Web server is single-threaded** — the dashboard polls `/getdata` every 200 ms; heavy CSV download traffic may briefly block other requests.
