#include "connectivity.h"
#include "clock_control.h"
#include "storage.h"
#include "menu_control.h"

static const char* TOPIC_SERIAL_IN        = "servo_clock/serial/in";
static const char* TOPIC_STATE_SERIAL_IN  = "servo_clock/state/serial_in_last";

void setupConnectivity() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setKeepAlive(60);
  mqtt.setSocketTimeout(10);
  mqtt.setBufferSize(1024);
}

void printWiFiStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
  } else {
    Serial.println("WiFi not connected");
  }
}

void startWiFiConnect() {
  if (wifiSSID.length() == 0) return;

  Serial.printf("Connecting to %s\n", wifiSSID.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.hostname(WIFI_HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());

  wifiConnecting = true;
  lastWiFiAttempt = millis();
}

void connectWiFi() {
  startWiFiConnect();
}

void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (!wifiConnecting) startWiFiConnect();
}

void updateWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    if (wifiConnecting) {
      wifiConnecting = false;
      Serial.println("WiFi connected");
      printWiFiStatus();
    }
    return;
  }

  if (!wifiConnecting || millis() - lastWiFiAttempt > 15000) {
    startWiFiConnect();
  }
}

static unsigned long ledBlinkOffUntilMs = 0;

void updateLedState() {
  if (!mqtt.connected()) {
    digitalWrite(LED_ESP_PINT, LOW);
    return;
  }
  if (millis() < ledBlinkOffUntilMs) {
    digitalWrite(LED_ESP_PINT, LOW);
  } else {
    digitalWrite(LED_ESP_PINT, HIGH);
  }
}

void mqttPublishRetained(const char* topic, const String& payload) {
  if (!mqtt.connected()) {
    Serial.print("[MQTT] Skipped publish, not connected: ");
    Serial.println(topic);
    return;
  }

  bool ok = mqtt.publish(topic, payload.c_str(), true);

  if (ok) {
    ledBlinkOffUntilMs = millis() + 80;
    digitalWrite(LED_ESP_PINT, LOW);
  }

  Serial.print("[MQTT] ");
  Serial.print(ok ? "Published " : "FAILED ");
  Serial.print(topic);
  Serial.print(" (");
  Serial.print(payload.length());
  Serial.println(" bytes)");
}

String isoTimestampNow() {
  time_t now = time(nullptr);
  if (now < 100000) return "";

  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", &timeinfo);

  String ts = String(buf);
  if (ts.length() > 5) {
    ts = ts.substring(0, ts.length() - 5)
       + ts.substring(ts.length() - 5, ts.length() - 2)
       + ":"
       + ts.substring(ts.length() - 2);
  }

  return ts;
}

void startNtpSync() {
  if (WiFi.status() != WL_CONNECTED) return;

  Serial.print("Starting NTP sync: ");
  Serial.println(currentTimezoneName);

  configTime(currentTzValue, "pool.ntp.org", "time.nist.gov");
  ntpStartMs = millis();
  ntpSyncInProgress = true;
}

void updateNtpSync() {
  if (!ntpSyncInProgress) return;

  time_t now = time(nullptr);
  if (now > 100000) {
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    int dow = timeinfo.tm_wday == 0 ? 1 : timeinfo.tm_wday + 1;

    clockRTC.setDS1302Time(
      timeinfo.tm_sec,
      timeinfo.tm_min,
      timeinfo.tm_hour,
      dow,
      timeinfo.tm_mday,
      timeinfo.tm_mon + 1,
      timeinfo.tm_year + 1900
    );

    ntpSynced = true;
    ntpSyncInProgress = false;

    Serial.println("RTC updated from NTP");
    publishStates();

    if (clockRunning) {
      setTimeFromRTC();
    }
    return;
  }

  if (millis() - ntpStartMs > 20000) {
    ntpSynced = false;
    ntpSyncInProgress = false;
    Serial.println("NTP sync timeout");
  }
}

bool syncRtcFromNtp() {
  startNtpSync();
  return true;
}

void publishClockTimeState(int hour, int minute) {
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
  mqttPublishRetained(TOPIC_STATE_TIME, String(buf));
}

void publishStates() {
  mqttPublishRetained(TOPIC_STATE_RUNNING, clockRunning ? "ON" : "OFF");
  mqttPublishRetained(TOPIC_STATE_TIMEZONE, currentTimezoneName);
  mqttPublishRetained(TOPIC_STATE_WIFI_RSSI, WiFi.status() == WL_CONNECTED ? String(WiFi.RSSI()) : "");
  mqttPublishRetained(TOPIC_STATE_IP, WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "");
  mqttPublishRetained(TOPIC_STATE_RTC_TIME, rtcTimeString());

  time_t now = time(nullptr);
  if (now > 100000) {
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    publishClockTimeState(timeinfo.tm_hour, timeinfo.tm_min);
  }

  String ts = isoTimestampNow();
  if (ts.length() > 0) {
    mqttPublishRetained(TOPIC_STATE_LAST_SYNC, ts);
  }
}

void publishDiscovery() {
  Serial.println("[HA] Publishing MQTT discovery...");

  mqttPublishRetained(
    "homeassistant/switch/servo_clock_running/config",
    "{"
    "\"name\":\"Servo Clock Running\","
    "\"unique_id\":\"servo_clock_running\","
    "\"state_topic\":\"servo_clock/state/running\","
    "\"command_topic\":\"servo_clock/cmd/running\","
    "\"payload_on\":\"ON\","
    "\"payload_off\":\"OFF\","
    "\"availability_topic\":\"servo_clock/status\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"device\":{"
      "\"identifiers\":[\"servo_clock_device\"],"
      "\"name\":\"Servo Clock\","
      "\"manufacturer\":\"Custom\","
      "\"model\":\"ESP8266 Servo Clock\""
    "}"
    "}"
  );

  mqttPublishRetained(
    "homeassistant/button/servo_clock_sync_rtc/config",
    "{"
    "\"name\":\"Servo Clock Sync RTC\","
    "\"unique_id\":\"servo_clock_sync_rtc\","
    "\"command_topic\":\"servo_clock/cmd/sync_rtc\","
    "\"payload_press\":\"PRESS\","
    "\"availability_topic\":\"servo_clock/status\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"device\":{"
      "\"identifiers\":[\"servo_clock_device\"],"
      "\"name\":\"Servo Clock\","
      "\"manufacturer\":\"Custom\","
      "\"model\":\"ESP8266 Servo Clock\""
    "}"
    "}"
  );

  mqttPublishRetained(
    "homeassistant/select/servo_clock_timezone/config",
    "{"
    "\"name\":\"Servo Clock Timezone\","
    "\"unique_id\":\"servo_clock_timezone\","
    "\"state_topic\":\"servo_clock/state/timezone\","
    "\"command_topic\":\"servo_clock/cmd/timezone\","
    "\"options\":[\"Europe/Copenhagen\",\"UTC\",\"Europe/London\",\"Europe/Berlin\",\"America/New_York\",\"America/Los_Angeles\",\"Asia/Tokyo\",\"Australia/Sydney\"],"
    "\"availability_topic\":\"servo_clock/status\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"device\":{"
      "\"identifiers\":[\"servo_clock_device\"],"
      "\"name\":\"Servo Clock\","
      "\"manufacturer\":\"Custom\","
      "\"model\":\"ESP8266 Servo Clock\""
    "}"
    "}"
  );

  mqttPublishRetained(
    "homeassistant/sensor/servo_clock_time/config",
    "{"
    "\"name\":\"Servo Clock Display Time\","
    "\"unique_id\":\"servo_clock_time\","
    "\"state_topic\":\"servo_clock/state/time\","
    "\"icon\":\"mdi:clock-digital\","
    "\"availability_topic\":\"servo_clock/status\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"device\":{"
      "\"identifiers\":[\"servo_clock_device\"],"
      "\"name\":\"Servo Clock\","
      "\"manufacturer\":\"Custom\","
      "\"model\":\"ESP8266 Servo Clock\""
    "}"
    "}"
  );

  mqttPublishRetained(
    "homeassistant/sensor/servo_clock_wifi_rssi/config",
    "{"
    "\"name\":\"Servo Clock WiFi RSSI\","
    "\"unique_id\":\"servo_clock_wifi_rssi\","
    "\"state_topic\":\"servo_clock/state/wifi_rssi\","
    "\"unit_of_measurement\":\"dBm\","
    "\"state_class\":\"measurement\","
    "\"icon\":\"mdi:wifi\","
    "\"availability_topic\":\"servo_clock/status\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"device\":{"
      "\"identifiers\":[\"servo_clock_device\"],"
      "\"name\":\"Servo Clock\","
      "\"manufacturer\":\"Custom\","
      "\"model\":\"ESP8266 Servo Clock\""
    "}"
    "}"
  );

  mqttPublishRetained(
    "homeassistant/sensor/servo_clock_ip/config",
    "{"
    "\"name\":\"Servo Clock IP\","
    "\"unique_id\":\"servo_clock_ip\","
    "\"state_topic\":\"servo_clock/state/ip\","
    "\"icon\":\"mdi:ip-network\","
    "\"availability_topic\":\"servo_clock/status\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"device\":{"
      "\"identifiers\":[\"servo_clock_device\"],"
      "\"name\":\"Servo Clock\","
      "\"manufacturer\":\"Custom\","
      "\"model\":\"ESP8266 Servo Clock\""
    "}"
    "}"
  );

  mqttPublishRetained(
    "homeassistant/sensor/servo_clock_rtc_time/config",
    "{"
    "\"name\":\"Servo Clock RTC Time\","
    "\"unique_id\":\"servo_clock_rtc_time\","
    "\"state_topic\":\"servo_clock/state/rtc_time\","
    "\"icon\":\"mdi:clock-outline\","
    "\"availability_topic\":\"servo_clock/status\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"device\":{"
      "\"identifiers\":[\"servo_clock_device\"],"
      "\"name\":\"Servo Clock\","
      "\"manufacturer\":\"Custom\","
      "\"model\":\"ESP8266 Servo Clock\""
    "}"
    "}"
  );

  mqttPublishRetained(
    "homeassistant/sensor/servo_clock_last_sync/config",
    "{"
    "\"name\":\"Servo Clock Last Sync\","
    "\"unique_id\":\"servo_clock_last_sync\","
    "\"state_topic\":\"servo_clock/state/last_sync\","
    "\"device_class\":\"timestamp\","
    "\"availability_topic\":\"servo_clock/status\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"device\":{"
      "\"identifiers\":[\"servo_clock_device\"],"
      "\"name\":\"Servo Clock\","
      "\"manufacturer\":\"Custom\","
      "\"model\":\"ESP8266 Servo Clock\""
    "}"
    "}"
  );

  mqttPublishRetained(
    "homeassistant/text/servo_clock_serial_in/config",
    "{"
    "\"name\":\"Servo Clock Serial In\","
    "\"unique_id\":\"servo_clock_serial_in\","
    "\"command_topic\":\"servo_clock/serial/in\","
    "\"mode\":\"text\","
    "\"availability_topic\":\"servo_clock/status\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"device\":{"
      "\"identifiers\":[\"servo_clock_device\"],"
      "\"name\":\"Servo Clock\","
      "\"manufacturer\":\"Custom\","
      "\"model\":\"ESP8266 Servo Clock\""
    "}"
    "}"
  );

  mqttPublishRetained(
    "homeassistant/sensor/servo_clock_serial_last/config",
    "{"
    "\"name\":\"Servo Clock Serial Last Command\","
    "\"unique_id\":\"servo_clock_serial_last\","
    "\"state_topic\":\"servo_clock/state/serial_in_last\","
    "\"icon\":\"mdi:console-line\","
    "\"availability_topic\":\"servo_clock/status\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"device\":{"
      "\"identifiers\":[\"servo_clock_device\"],"
      "\"name\":\"Servo Clock\","
      "\"manufacturer\":\"Custom\","
      "\"model\":\"ESP8266 Servo Clock\""
    "}"
    "}"
  );

  Serial.println("[HA] Discovery published");
}

bool connectMqtt() {
  String clientId = "servo-clock-" + String(ESP.getChipId(), HEX);

  Serial.print("[MQTT] Connecting to ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.println(MQTT_PORT);

  bool ok = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS, TOPIC_STATUS, 1, true, "offline");

  if (ok) {
    Serial.println("[MQTT] Connected");
    mqtt.publish(TOPIC_STATUS, "online", true);
    mqtt.subscribe(TOPIC_CMD_RUNNING);
    mqtt.subscribe(TOPIC_CMD_SYNC_RTC);
    mqtt.subscribe(TOPIC_CMD_TIMEZONE);
    mqtt.subscribe(TOPIC_SERIAL_IN);

    publishDiscovery();
    publishStates();
    return true;
  }

  Serial.print("[MQTT] Connect failed, rc=");
  Serial.println(mqtt.state());
  return false;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String msg = "";

  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  msg.trim();

  Serial.print("[MQTT] RX ");
  Serial.print(topicStr);
  Serial.print(" = ");
  Serial.println(msg);

  if (topicStr == TOPIC_CMD_RUNNING) {
    if (msg == "ON") startClock();
    else if (msg == "OFF") stopClock();

    mqttPublishRetained(TOPIC_STATE_RUNNING, clockRunning ? "ON" : "OFF");
    return;
  }

  if (topicStr == TOPIC_CMD_SYNC_RTC) {
    syncRtcFromNtp();
    if (clockRunning) setTimeFromRTC();
    return;
  }

  if (topicStr == TOPIC_CMD_TIMEZONE) {
    if (setTimezoneByName(msg)) {
      saveClockSettings();
      mqttPublishRetained(TOPIC_STATE_TIMEZONE, currentTimezoneName);
      syncRtcFromNtp();
      if (clockRunning) setTimeFromRTC();
    }
    return;
  }

  if (topicStr == TOPIC_SERIAL_IN) {
    mqttPublishRetained(TOPIC_STATE_SERIAL_IN, msg);
    routeCommand(msg);
    return;
  }
}

void maintainConnectivity() {
  updateWiFi();

  if (WiFi.status() == WL_CONNECTED && !ntpSynced && !ntpSyncInProgress) {
    startNtpSync();
  }

  updateNtpSync();

  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.connected()) {
      unsigned long now = millis();
      if (now - lastMqttReconnectAttempt > 5000) {
        lastMqttReconnectAttempt = now;
        connectMqtt();
        
      }
    } else {
      mqtt.loop();
    }
  }

  updateLedState();

  if (mqtt.connected() && millis() - lastWifiStatePublish > 60000) {
    lastWifiStatePublish = millis();
    mqttPublishRetained(TOPIC_STATE_WIFI_RSSI, String(WiFi.RSSI()));
    mqttPublishRetained(TOPIC_STATE_IP, WiFi.localIP().toString());
    mqttPublishRetained(TOPIC_STATE_RTC_TIME, rtcTimeString());

    time_t now = time(nullptr);
    if (now > 100000) {
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);
      publishClockTimeState(timeinfo.tm_hour, timeinfo.tm_min);
    }
  }
}