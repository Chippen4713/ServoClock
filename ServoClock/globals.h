#ifndef GLOBALS_H
#define GLOBALS_H

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <virtuabotixRTC.h>
#include <time.h>
#include <TZ.h>
#include <DHT.h>

struct ServoCal {
  uint16_t open;
  uint16_t closed;
  bool inverted;
};

extern const int RELAY_BOARD1_PIN;
extern const int RELAY_BOARD2_PIN;
extern const int DHT_PIN;
extern const int LDR_PIN;
extern const int LED_GREEN_CH;
extern const int LED_BLUE_CH;

extern DHT dhtSensor;
extern float sensorTemp;
extern float sensorHumidity;
extern int   sensorLight;
extern unsigned long lastSensorReadMs;
extern bool servoPowerEnabled;
extern unsigned long servoPowerOffAtMs;
extern unsigned long servoPowerHoldMs;

extern uint16_t servoStepSize;
extern uint16_t servoStepDelayMs;
extern uint16_t servoTargetPos[28];
extern unsigned long servoLastStepMs[28];

struct TimezoneOption {
  const char* name;
  const char* tz;
};

extern TimezoneOption tzOptions[];
extern int TZ_COUNT;

extern String currentTimezoneName;
extern const char* currentTzValue;

enum AppMode {
  MODE_MAIN,
  MODE_SERVO_CAL,
  MODE_SMOOTH,
  MODE_RTC,
  MODE_MOTION,
  MODE_WIFI,
  MODE_RUN
};

extern Adafruit_PWMServoDriver board1;
extern Adafruit_PWMServoDriver board2;
extern virtuabotixRTC clockRTC;

extern ServoCal cal[28];
extern uint16_t currentPos[28];

extern int selectedServo;
extern const char* CAL_FILE;
extern const char* SETTINGS_FILE;

extern int lastHour;
extern int lastMinute;
extern bool clockRunning;
extern bool ntpSynced;

extern unsigned long lastMqttReconnectAttempt;
extern unsigned long lastWifiStatePublish;
extern unsigned long lastClockCheck;

extern int p0[7];
extern int p1[7];
extern int p2[7];
extern int p3[7];
extern int p4[7];
extern int p5[7];
extern int p6[7];
extern int p7[7];
extern int p8[7];
extern int p9[7];
extern int* digits[10];

extern const int PIR_PIN;

extern bool wifiConnecting;
extern bool ntpSyncInProgress;

extern unsigned long lastWiFiAttempt;
extern unsigned long ntpStartMs;

extern String wifiSSID;
extern String wifiPASS;

extern const char* WIFI_HOSTNAME;
extern const char* MQTT_HOST;
extern const uint16_t MQTT_PORT;
extern const char* MQTT_USER;
extern const char* MQTT_PASS;

extern WiFiClient wifiClient;
extern PubSubClient mqtt;

extern const char* TOPIC_STATUS;
extern const char* TOPIC_STATE_RUNNING;
extern const char* TOPIC_STATE_TIME;
extern const char* TOPIC_STATE_TIMEZONE;
extern const char* TOPIC_STATE_LAST_SYNC;
extern const char* TOPIC_STATE_WIFI_RSSI;
extern const char* TOPIC_STATE_IP;
extern const char* TOPIC_STATE_RTC_TIME;
extern const char* TOPIC_CMD_RUNNING;
extern const char* TOPIC_CMD_SYNC_RTC;
extern const char* TOPIC_CMD_TIMEZONE;

extern const char* TOPIC_STATE_TEMP;
extern const char* TOPIC_STATE_HUMIDITY;
extern const char* TOPIC_STATE_LIGHT;
extern const char* TOPIC_STATE_MOTION;

extern AppMode currentMode;



#endif