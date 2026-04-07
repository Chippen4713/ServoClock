# Servo Clock

A 7-segment clock built from 28 hobby servos — each segment is a physical flap that pivots between open and closed to form digits.

> **Based on the original design by [OTVINTA.com](https://www.otvinta.com/download14.html)**
> The mechanical design, 3D-printed parts and segment layout are from their excellent Servo-Driven Digital Clock project. This repository is an ESP8266 reimplementation of the control electronics and software, replacing the original Raspberry Pi + Pololu Maestro + Windows IoT stack with a single low-cost WiFi microcontroller and full Home Assistant integration.

---

## What's different in this build

| | Original (OTVINTA) | This build |
|---|---|---|
| Controller | Raspberry Pi 3 | ESP8266 NodeMCU |
| Servo drivers | 2× Pololu Maestro | 2× PCA9685 (I²C) |
| Software | UWP app on Windows IoT | Arduino firmware |
| Connectivity | Local only | WiFi + MQTT |
| Home automation | None | Home Assistant auto-discovery |
| Time sync | Manual | NTP over WiFi |
| RTC backup | None | DS1302 |
| Terminal | Windows app | Serial + TCP on port 23 |
| Settings storage | Config file on Pi | LittleFS on ESP8266 |

---

## Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP8266 NodeMCU v2 |
| Servo drivers | 2× PCA9685 (I²C, 0x40 and 0x41) |
| Servos | 28× SG90 hobby servos |
| RTC | DS1302 (D0=CLK, D3=DAT, D4=CE) |
| Relay boards | 2× relay modules for servo power (D5, D6) |
| PIR sensor | Motion sensor on D8 |
| Status LED | WiFi/MQTT activity LED on D7 |
| Board LEDs | PCA9685 channel 14 on each board (lights when servos on that board are moving) |

### Wiring

```
ESP8266 D1 → I²C SCL  (both PCA9685 boards share the bus)
ESP8266 D2 → I²C SDA
ESP8266 D0 → DS1302 CLK
ESP8266 D3 → DS1302 DAT
ESP8266 D4 → DS1302 CE
ESP8266 D5 → Relay board 1  (controls power to servos 0–13)
ESP8266 D6 → Relay board 2  (controls power to servos 14–27)
ESP8266 D7 → Status LED
ESP8266 D8 → PIR sensor

PCA9685 #1 (0x40) → Servos 0–13  + activity LED on channel 14
PCA9685 #2 (0x41) → Servos 14–27 + activity LED on channel 14
```

---

## Features

- **Auto time display** — syncs from NTP, drives servos to show HH:MM
- **RTC backup** — DS1302 keeps time when WiFi is unavailable
- **Smooth servo motion** — configurable step size and step delay
- **Auto servo power** — relay cuts power after servos finish moving (saves heat and power)
- **Home Assistant integration** — MQTT auto-discovery, full control from HA dashboard
- **TCP terminal** — connect on port 23 (`telnet <ip> 23`) for a full interactive menu matching the serial output
- **Persistent settings** — calibration, smooth settings, WiFi credentials and timezone all saved to LittleFS
- **PIR motion sensor** — ready for wake-on-motion logic

---

## Getting started

### 1. Print the parts

Download the STL files from the original project page:
**https://www.otvinta.com/download14.html**

The `.stl` files are also included in this repository.

### 2. Install Arduino dependencies

| Library | Notes |
|---|---|
| `esp8266` board package | via Board Manager |
| `Adafruit PWM Servo Driver` | for PCA9685 |
| `PubSubClient` | MQTT |
| `ArduinoJson` 6.x | settings storage |
| `virtuabotixRTC` | DS1302 RTC |

### 3. Create your secrets file

Copy `ServoClock/secrets.h.example` to `ServoClock/secrets.h` and fill in your values:

```cpp
#define SECRET_WIFI_SSID  "your-wifi-ssid"
#define SECRET_WIFI_PASS  "your-wifi-password"

#define SECRET_MQTT_HOST  "192.168.1.x"
#define SECRET_MQTT_PORT  1883
#define SECRET_MQTT_USER  "your-mqtt-user"
#define SECRET_MQTT_PASS  "your-mqtt-password"
```

`secrets.h` is gitignored and will never be committed.

### 4. Upload

Board: **NodeMCU 1.0 (ESP-12E Module)**

Compile and upload via Arduino IDE or `arduino-cli`.

---

## Calibration

Each servo needs an open and closed PWM value saved per-servo. On first boot defaults are used (open=530, closed=300) — calibrate from the terminal:

1. Connect via serial or `telnet <ip> 23`
2. Enter `1` → Servo calibration
3. `s 0` → select servo 0
4. `+` / `-` move ±10, `++` / `--` move ±2
5. When the flap is fully open: `o` to save
6. When fully closed: `c` to save
7. Repeat for all 28 servos (the order matches the OTVINTA segment labeling A–G per digit)
8. `w` to write — saves everything to `/calibration.json` on the ESP

---

## TCP terminal

```
telnet <esp-ip> 23
```

The menu is identical to the serial terminal:

```
MAIN MENU
├── 1  Servo calibration   — set open/closed per servo, invert, save/load
├── 2  Smooth settings     — step size, step delay, power hold time
├── 3  RTC menu            — show time, set from NTP or fixed
├── 4  Motion sensor       — read / watch PIR
├── 5  WiFi menu           — status, scan, change credentials
├── 6  Run clock mode      — start / stop
└── 7  Show calibration    — dump all 28 servo positions
```

---

## Settings file

All settings are stored in `/calibration.json` on LittleFS:

- 28× servo open / closed / inverted values
- Smooth step size and delay
- Servo power hold time
- WiFi SSID and password
- Timezone

Use `w` to save and `l` to load from any menu.

---

## Home Assistant

MQTT auto-discovery publishes on boot. Entities created automatically:

| Entity | Topic |
|---|---|
| Switch: Running | `servo_clock/cmd/running` |
| Button: Sync RTC | `servo_clock/cmd/sync_rtc` |
| Select: Timezone | `servo_clock/cmd/timezone` |
| Sensor: Time | `servo_clock/state/time` |
| Sensor: RTC time | `servo_clock/state/rtc_time` |
| Sensor: WiFi RSSI | `servo_clock/state/wifi_rssi` |
| Sensor: IP address | `servo_clock/state/ip` |
| Sensor: Last NTP sync | `servo_clock/state/last_sync` |

---

## Project structure

```
ServoClock/
├── ServoClock.ino         Main sketch — setup + loop
├── globals.h/.cpp         Shared variables, pin definitions
├── secrets.h.example      Credentials template (copy to secrets.h)
├── servo_control.h/.cpp   Servo movement, smooth stepping, calibration
├── clock_control.h/.cpp   Time display, RTC read/write
├── connectivity.h/.cpp    WiFi, MQTT, NTP, Home Assistant discovery
├── menu_control.h/.cpp    Interactive menu system (serial + TCP)
├── web_interface.h/.cpp   TCP terminal server on port 23
└── storage.h/.cpp         LittleFS save/load for all settings
home_assistant/            Example HA dashboard YAML
```

---

## Credits

Mechanical design and 3D models by **[OTVINTA.com](https://www.otvinta.com/download14.html)** — a beautifully engineered project.

This ESP8266 firmware and HA integration built on top of that foundation.

---

## License

MIT — use freely, attribution appreciated.
