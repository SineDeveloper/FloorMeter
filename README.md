# Super Flat Floor Meter

An ESP32-based portable device that measures floor flatness/levelness for **forklift safety monitoring**. The unit is driven (or rolled) across a floor, continuously tracking pitch and roll angles plus travel distance, and flags sections where the floor slope exceeds forklift mast safety limits.

## Features

- **Real-time telemetry** — pitch, roll, distance, and 3-point wheel heights (Z_L, Z_R, Z_Rear) updated live on a built-in web dashboard
- **3-point wheel-height calculation** — pitch/roll angles converted to front-left, front-right and rear wheel heights using configurable geometry
- **Forklift safety monitoring** — automatic NORMAL / WARNING / DANGER classification against configurable safety limits
- **Section summaries** — rolling statistics (average + min/max range) per configurable travel section, written as `SECTION` rows alongside per-sample rows
- **Configurable geometry** — front track width, rear arm distance and section length settable from the web dashboard and persisted across reboots
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

1. Install the **Arduino IDE** with the **esp32** board package (v4.0.0), or use the **Arduino CLI** (`arduino-cli core install esp32:esp32`).
2. Required libraries (all bundled with the esp32 core): `WiFi`, `WebServer`, `LittleFS`, `Wire`.
3. **Arduino IDE**: select your ESP32 board and COM port, open `FloorMeter.ino`, and click **Upload**.
4. **Arduino CLI**:

   ```bat
   arduino-cli compile --fqbn esp32:esp32:esp32 .
   arduino-cli upload --fqbn esp32:esp32:esp32 --port COMx .
   ```

   Verified result: sketch uses 1,097,789 bytes (83%) of flash, 52,120 bytes (15%) of RAM.

> **Windows note:** if your user profile path contains a space (e.g. `C:\Users\Some Name`), the ESP32 RISC-V toolchain invocation fails with `'<user>' is not recognized as an internal or external command`. Fix by pointing the CLI data directory at a path without spaces, e.g. add to the arduino-cli YAML config (`%LOCALAPPDATA%\Arduino15\arduino-cli.yaml`):
>
> ```yaml
> directories:
>     data: C:\Arduino15
>     staging: C:\Arduino15\staging
> ```
>
> and move the `packages`/`libraries` folders into `C:\Arduino15`.

### Web Dashboard

1. Power on the device and connect to the Wi-Fi access point **`SuperFlat_FloorMeter`** (password: `123456789`).
2. Open the IP address printed on the Serial console (typically `192.168.4.1`) in a browser.
3. Use the dashboard to:
   - **Start / Stop** a measurement session (Start clears the log and resets the odometer)
   - **Re-Calibrate** the gyro (keep the device still on the floor first)
   - **Download CSV** — stream the logged data file from the device
   - Watch the live pitch/roll chart with safety-limit lines
   - Read the live **Z_L / Z_R / Z_Rear** wheel-height cards (whole mm, signed)
   - **Geometry** panel — set the front track width, rear arm distance and section length (all in **cm**), then **Save Geometry** to persist them to flash

### Serial Simulation Mode

To test without the physical wheel, drive the Serial monitor (115200 baud):

1. Send a speed value in **m/s** (e.g. `1.5`) — this auto-initializes a new session and simulates 10 seconds of travel at that speed, logging as it goes.
2. After 10 seconds the run stops automatically.
3. Real encoder readings are always summed with the simulated distance, so a stationary encoder + simulated speed = a clean simulated run.

## CSV Output

Logged to LittleFS at `/floor_data.csv`. Distance is reported in **centimeters** as a non-negative whole number, and wheel heights are rounded to **whole mm** (signed) to hide sub-millimeter sensor jitter. While measuring, a `SAMPLE` row is appended every **300 ms**, and a `SECTION` summary row is appended each time the traveled distance crosses a section boundary (default 30 cm):

```
Row_Type,Section,Timestamp(ms),Distance(cm),Pitch(deg),Roll(deg),Z_Left(mm),Z_Right(mm),Z_Rear(mm),Samples,Range_Left(mm),Range_Right(mm),Range_Rear(mm),Safety_Status
SAMPLE,0,0,0,0.0,0.0,0,0,0,,,,,NORMAL
SAMPLE,0,300,5,0.1,-0.1,4,-4,1,,,,NORMAL
SECTION,0,,30,0.05,-0.05,2,-2,1,4,8,8,2,,
...
```

- `Row_Type` — `SAMPLE` (one per log interval) or `SECTION` (rolling summary per section)
- `Timestamp` — milliseconds since session start (relative), `SAMPLE` rows only
- `Z_Left` / `Z_Right` / `Z_Rear` — instantaneous wheel heights in whole mm relative to the body center, signed (see below)
- `Samples` — number of samples accumulated in the section (`SECTION` rows only)
- `Range_Left/Right/Rear` — max − min wheel height within the section (`SECTION` rows only)
- `Safety_Status` — `NORMAL` / `WARNING` / `DANGER` per the thresholds above (`SAMPLE` rows only)

### How to read REAR Z and SECTION rows

**REAR Z (mm)** is the estimated height of the rear wheel relative to the body center, computed from the pitch angle: `Z_Rear = SIGN_PITCH_REAR × REAR_ARM × tan(pitch)`. It is a *signed* value:

- **Magnitude** — how far the rear wheel is displaced from the body-center height, in mm (e.g. `12` = the rear wheel is 12 mm above/below center height).
- **Sign** — which end is down. With the default signs in `Config.h` (`SIGN_PITCH_REAR = -1`): **negative** = rear wheel low (nose up), **positive** = rear wheel high (nose down). Flip the sign constant if your mounting differs.
- `Z_Left` / `Z_Right` work the same way from the roll angle and always carry opposite signs — the negative one is the wheel that is down.
- On the dashboard these cards show whole mm values so the ±0.5 mm sensor jitter is not visible.

**SECTION rows** group the log into fixed travel buckets of the configured section length (default **30 cm**). Section N covers the stretch from N×30 cm to (N+1)×30 cm of travel:

- While traveling, each `SAMPLE` row carries the index of the section it belongs to in the `Section` column (0, then 1, 2, … as the odometer advances).
- The moment the odometer crosses the next boundary (30 cm, 60 cm, 90 cm, …), the device writes one `SECTION` row for the section that just finished. Its `Distance(cm)` is the distance where that stretch ended.
- A `SECTION` row summarizes that stretch: average pitch / roll / wheel heights (same columns as SAMPLE rows), the `Samples` count (how many 300 ms samples were collected in the stretch — fewer when moving faster), and the `Range_*` values = max − min wheel height within the stretch.
- The **`Range_*` columns are the flatness indicator for that stretch**: a small range means the wheel stayed at nearly the same height over that 30 cm (flat floor), a large range means the body pitched/rolled a lot while crossing it. Compare `Range_*` across sections to find the rough parts of the floor.

## Wheel Heights (3-Point Geometry)

Pitch and roll are converted to the heights of the three wheel contact points relative to the body center (mm):

```
Z_Left  = SIGN_ROLL_LEFT  * (FRONT_TRACK_MM / 2) * tan(roll)
Z_Right = SIGN_ROLL_RIGHT * (FRONT_TRACK_MM / 2) * tan(roll)
Z_Rear  = SIGN_PITCH_REAR * REAR_ARM_MM          * tan(pitch)
```

The geometry values are configurable at runtime from the dashboard's Geometry panel (or via `/config`) and are persisted to LittleFS (`/config.json`); defaults and the `SIGN_*` sign conventions live in `Config.h`. Flip any `SIGN_*` constant to `-1.0` if a wheel's "up is positive" direction differs for your mounting.

## HTTP API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Dashboard (HTML) |
| `/getdata` | GET | Telemetry JSON: `{"dist":… (cm, whole number, never negative),"pitch":…,"roll":…,"z_left":… (whole mm, signed),"z_right":…,"z_rear":…,"measuring":true/false}` |
| `/action?cmd=start` | GET | Reset log + odometer, start measuring. Returns `{"measuring":true}` |
| `/action?cmd=stop` | GET | Stop measuring. Returns `{"measuring":false}` |
| `/action?cmd=recal` | GET | Re-calibrate gyro offsets (device must be still) |
| `/download` | GET | Download `/floor_data.csv` |
| `/config` | GET | Geometry JSON: `{"front_track_mm":…,"rear_arm_mm":…,"section_mm":…}` |
| `/config` | POST | Set geometry (form fields `front_track_mm`, `rear_arm_mm`, `section_mm` in mm); persists to `/config.json` and returns the new geometry JSON. The dashboard panel edits these values in **cm** and converts them to mm when saving |

## Project Structure

| File | Role |
|------|------|
| `FloorMeter.ino` | Main entry point — wires modules together, measurement loop, wheel-height calculation, section tracking, CSV logging, serial simulation mode |
| `Config.h` | Wi-Fi AP credentials, file names, wheel diameter, geometry defaults, wheel-height sign conventions |
| `SensorEncoder.h` | AS5600 angle encoder driver — 12-bit angle deltas with wrap-around, distance conversion, simulated-speed support |
| `SensorIMU.h` | MPU9250 driver — gyro calibration, complementary filter (0.998 gyro / 0.002 accel), moving-average smoothing, deadband |
| `StorageManager.h` | LittleFS CSV logging (SAMPLE + SECTION rows) and geometry config persistence (`/config.json`) |
| `HttpServerManager.h` | ESP32 Wi-Fi access point + WebServer (dashboard, actions, telemetry, config, download) |
| `WebHtml.h` | Dashboard HTML/CSS (PROGMEM) |
| `WebScript.h` | Dashboard JS — live canvas chart, wheel-height cards, safety panel, geometry panel, button state |

## Configuration

All tunable values live in `Config.h`:

| Constant | Default | Description |
|----------|---------|-------------|
| `AP_SSID` | `SuperFlat_FloorMeter` | Wi-Fi access point name |
| `AP_PASS` | `123456789` | Wi-Fi access point password |
| `DATA_FILE` | `/floor_data.csv` | LittleFS log file path |
| `CONFIG_FILE` | `/config.json` | LittleFS geometry config path |
| `WHEEL_DIAMETER_MM` | `100.0` | Actual wheel diameter in millimeters |
| `DEFAULT_FRONT_TRACK_MM` | `800.0` | Default distance between the two front wheels (axle width), mm |
| `DEFAULT_REAR_ARM_MM` | `600.0` | Default longitudinal distance from front axle to rear wheel, mm |
| `DEFAULT_SECTION_MM` | `300.0` | Travel distance per measured section, mm (minimum 10) |
| `SIGN_ROLL_LEFT` / `SIGN_ROLL_RIGHT` / `SIGN_PITCH_REAR` | `-1.0` / `+1.0` / `-1.0` | Sign conventions for wheel-height calculation; flip to change "up is positive" direction |

`DEFAULT_*` values are only initial values — the live geometry can be changed at runtime from the dashboard or via `/config` and is persisted in `CONFIG_FILE`. The dashboard's Geometry panel shows and edits these values in **centimeters**; internally they are stored and transmitted in millimeters (an existing `config.json` written in mm is still read correctly).

Safety thresholds (pitch 1.0° / roll 0.5°) are defined in both `FloorMeter.ino` (firmware logging) and `WebScript.h` (dashboard); update both together if you change them.

## Notes & Limitations

- **Gyro-only calibration** — `calibrate()` samples gyro offsets only; it does not zero the accelerometer pitch/roll. If the device boots tilted, the complementary filter slowly pulls the angle back toward the accel reference over time.
- **Deadband** — pitch/roll readings with magnitude < 0.1° are clamped to 0 to suppress jitter.
- **10-sample moving average** — applied to pitch/roll before display/logging.
- **Web server is single-threaded** — the dashboard polls `/getdata` every 200 ms; heavy CSV download traffic may briefly block other requests.
