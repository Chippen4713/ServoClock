# Servo Clock

A 7-segment clock built from 28 hobby servos, controlled by an ESP8266 with full Home Assistant integration.

Each digit is made of 7 servo-driven segments — the servo rotates a flap between an open and closed position to form the digit shape. The clock shows HH:MM across 4 digits (28 servos total).

![Clock concept: 4 digits × 7 servo segments]

---

## Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP8266 NodeMCU v2 |
| Servo drivers | 2× PCA9685 (I²C, 0x40 and 0x41) |
| Servos | 28× standard hobby servos |
| RTC | DS1302 (D0=CLK, D3=DAT, D4=CE) |
| Relay boards | 2× relay modules for servo power (D5, D6) |
| PIR sensor | Motion sensor on D8 |
| Status LED | WiFi/MQTT activity LED on D7 |
| Board LEDs | PCA9685 channel 14 on each board (lights when servos on that board are moving) |

### Wiring summary

```
ESP8266 D1 → I²C SCL (both PCA9685 boards)
ESP8266 D2 → I²C SDA (both PCA9685 boards)
ESP8266 D0 → DS1302 CLK
ESP8266 D3 → DS1302 DAT
ESP8266 D4 → DS1302 CE
ESP8266 D5 → Relay board 1 (servos 0–13)
ESP8266 D6 → Relay board 2 (servos 14–27)
ESP8266 D7 → Status LED
ESP8266 D8 → PIR sensor
PCA9685 #1  → Servos 0–13  + LED on channel 14
PCA9685 #2  → Servos 14–27 + LED on channel 14
```

---

## Features

- **Auto time display** — syncs from NTP, drives servos to show HH:MM
- **RTC backup** — DS1302 keeps time when WiFi is unavailable
- **Smooth servo motion** — configurable step size and step delay
- **Auto servo power** — relay cuts power after servos finish moving (saves heat and power)
- **Home Assistant integration** — MQTT auto-discovery, control from HA dashboard
- **TCP terminal** — connect on port 23 (e.g. `telnet <ip> 23`) for a full interactive menu
- **Persistent settings** — all calibration, smooth settings, WiFi credentials and timezone saved to LittleFS
- **PIR motion sensor** — ready for wake-on-motion logic

---

## Getting started

### 1. Install dependencies (Arduino IDE / arduino-cli)

| Library | Version tested |
|---|---|
| esp8266 board package | 3.x |
| Adafruit PWM Servo Driver | 3.x |
| PubSubClient | 2.8 |
| ArduinoJson | 6.x |
| virtuabotixRTC | any |

### 2. Create your secrets file

Copy `ServoClock/secrets.h.example` to `ServoClock/secrets.h` and fill in your credentials:

```cpp
#define SECRET_WIFI_SSID  "your-wifi-ssid"
#define SECRET_WIFI_PASS  "your-wifi-password"
#define SECRET_MQTT_HOST  "192.168.1.x"
#define SECRET_MQTT_PORT  1883
#define SECRET_MQTT_USER  "your-mqtt-user"
#define SECRET_MQTT_PASS  "your-mqtt-password"
```

`secrets.h` is gitignored and will never be committed.

### 3. Upload

Select board: **NodeMCU 1.0 (ESP-12E Module)**, then compile and upload.

---

## TCP terminal

Connect with any Telnet client:

```
telnet <esp-ip> 23
```

Or on Windows:
```
putty.exe -telnet <esp-ip> 23
```

Type `h` for the menu. The terminal mirrors the serial output exactly.

### Menu structure

```
MAIN MENU
├── 1  Servo calibration   — set open/closed positions per servo, save/load
├── 2  Smooth settings     — tune step size, step delay, power hold time
├── 3  RTC menu            — show time, set from NTP or fixed
├── 4  Motion sensor       — read/watch PIR sensor
├── 5  WiFi menu           — status, scan, change SSID/password
├── 6  Run clock mode      — start/stop the clock
└── 7  Show calibration    — dump all 28 servo positions
```

---

## Servo calibration

Each servo needs an **open** and **closed** PWM value stored in `/calibration.json` on LittleFS.

1. Connect via serial or TCP terminal
2. Enter `1` (Servo calibration)
3. Select a servo: `s 0`
4. Move with `+` / `-` (±10) or `++` / `--` (±2)
5. When the flap is fully open: `o` to save open position
6. When fully closed: `c` to save closed position
7. Repeat for all 28 servos
8. `w` to write calibration file (saves all settings including smooth/WiFi/timezone)

---

## Settings file

All settings are stored in `/calibration.json` on the ESP's LittleFS:

- 28× servo open/closed/inverted values
- Smooth step size and delay
- Servo power hold time
- WiFi SSID and password
- Timezone

Use `w` (write) and `l` (load) in the serial/TCP menu to save and restore.

---

## Home Assistant

The device publishes MQTT auto-discovery on boot. Entities created automatically:

| Entity | Description |
|---|---|
| Switch: Running | Start/stop the clock |
| Button: Sync RTC | Trigger NTP sync |
| Select: Timezone | Change timezone |
| Sensor: Time | Current displayed time |
| Sensor: RTC Time | Raw RTC time |
| Sensor: WiFi RSSI | Signal strength |
| Sensor: IP | Device IP address |
| Sensor: Last NTP sync | Timestamp of last sync |

MQTT topics are all under `servo_clock/`.

---

## Project structure

```
ServoClock/
├── ServoClock.ino       Main sketch, setup + loop
├── globals.h/.cpp       All shared variables and pin definitions
├── secrets.h.example    Credentials template (copy to secrets.h)
├── servo_control.h/.cpp Servo movement, calibration, smooth stepping
├── clock_control.h/.cpp Time display logic, RTC read/write
├── connectivity.h/.cpp  WiFi, MQTT, NTP, Home Assistant discovery
├── menu_control.h/.cpp  Serial + TCP interactive menu system
├── web_interface.h/.cpp TCP terminal server (port 23)
└── storage.h/.cpp       LittleFS save/load for all settings
home_assistant/          Example HA automation/dashboard YAML
```

---

## License

MIT — do whatever you want, attribution appreciated.
