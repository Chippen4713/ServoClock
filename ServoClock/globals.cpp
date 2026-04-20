#include "globals.h"
#include "secrets.h"

// Define Pins locations
Adafruit_PWMServoDriver board1(0x41);
Adafruit_PWMServoDriver board2(0x40);
virtuabotixRTC clockRTC(D0, D3, D4);
const int RELAY_BOARD1_PIN = D5;
const int RELAY_BOARD2_PIN = D6;
const int LED_ESP_PINT = D7;
const int LED_BOARD1_PIN = 14;
const int LED_BOARD2_PIN = 14;
const int PIR_PIN = D8;




bool servoPowerEnabled = false;
unsigned long servoPowerOffAtMs = 0;
unsigned long servoPowerHoldMs = 3000;

ServoCal cal[28];
uint16_t currentPos[28];
int selectedServo = 0;

uint16_t servoTargetPos[28];
unsigned long servoLastStepMs[28];

uint16_t servoStepSize = 2;
uint16_t servoStepDelayMs = 10;

const char* CAL_FILE = "/calibration.json";
const char* SETTINGS_FILE = "/clock_settings.json";

int lastHour = -1;
int lastMinute = -1;
bool clockRunning = false;
bool ntpSynced = false;

unsigned long lastMqttReconnectAttempt = 0;
unsigned long lastWifiStatePublish = 0;
unsigned long lastClockCheck = 0;

int p0[7] = {1, 1, 1, 0, 1, 1, 1};
int p1[7] = {0, 0, 1, 0, 0, 1, 0};
int p2[7] = {1, 0, 1, 1, 1, 0, 1};
int p3[7] = {1, 0, 1, 1, 0, 1, 1};
int p4[7] = {0, 1, 1, 1, 0, 1, 0};
int p5[7] = {1, 1, 0, 1, 0, 1, 1};
int p6[7] = {1, 1, 0, 1, 1, 1, 1};
int p7[7] = {1, 0, 1, 0, 0, 1, 0};
int p8[7] = {1, 1, 1, 1, 1, 1, 1};
int p9[7] = {1, 1, 1, 1, 0, 1, 0};

int* digits[10] = {p0, p1, p2, p3, p4, p5, p6, p7, p8, p9};



String wifiSSID = SECRET_WIFI_SSID;
String wifiPASS = SECRET_WIFI_PASS;

const char* WIFI_HOSTNAME = "servo-clock";
const char* MQTT_HOST     = SECRET_MQTT_HOST;
const uint16_t MQTT_PORT  = SECRET_MQTT_PORT;
const char* MQTT_USER     = SECRET_MQTT_USER;
const char* MQTT_PASS     = SECRET_MQTT_PASS;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool wifiConnecting = false;
bool ntpSyncInProgress = false;

unsigned long lastWiFiAttempt = 0;
unsigned long ntpStartMs = 0;

const char* TOPIC_STATUS          = "servo_clock/status";
const char* TOPIC_STATE_RUNNING   = "servo_clock/state/running";
const char* TOPIC_STATE_TIME      = "servo_clock/state/time";
const char* TOPIC_STATE_TIMEZONE  = "servo_clock/state/timezone";
const char* TOPIC_STATE_LAST_SYNC = "servo_clock/state/last_sync";
const char* TOPIC_STATE_WIFI_RSSI = "servo_clock/state/wifi_rssi";
const char* TOPIC_STATE_IP        = "servo_clock/state/ip";
const char* TOPIC_STATE_RTC_TIME  = "servo_clock/state/rtc_time";

const char* TOPIC_CMD_RUNNING     = "servo_clock/cmd/running";
const char* TOPIC_CMD_SYNC_RTC    = "servo_clock/cmd/sync_rtc";
const char* TOPIC_CMD_TIMEZONE    = "servo_clock/cmd/timezone";

TimezoneOption tzOptions[] = {
  {"Europe/Copenhagen", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"UTC", "UTC0"},
  {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0"},
  {"Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
  {"America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0"},
  {"Asia/Tokyo", "JST-9"},
  {"Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3"}
};

int TZ_COUNT = sizeof(tzOptions) / sizeof(tzOptions[0]);

String currentTimezoneName = "Europe/Copenhagen";
const char* currentTzValue = "CET-1CEST,M3.5.0,M10.5.0/3";

AppMode currentMode = MODE_MAIN;