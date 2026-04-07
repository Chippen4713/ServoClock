#include "storage.h"

bool saveCalibration() {
  DynamicJsonDocument doc(4096);

  // Servo calibration
  JsonArray arr = doc.createNestedArray("servos");
  for (int i = 0; i < 28; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["open"]   = cal[i].open;
    obj["closed"] = cal[i].closed;
    obj["inv"]    = cal[i].inverted;
  }

  // Smooth settings
  doc["step"]         = servoStepSize;
  doc["delayMs"]      = servoStepDelayMs;
  doc["powerHoldMs"]  = servoPowerHoldMs;

  // WiFi credentials
  doc["ssid"] = wifiSSID;
  doc["pass"] = wifiPASS;

  // Clock / timezone
  doc["timezone"] = currentTimezoneName;

  File f = LittleFS.open(CAL_FILE, "w");
  if (!f) return false;

  size_t written = serializeJsonPretty(doc, f);
  f.close();
  return written > 0;
}

bool loadCalibration() {
  if (!LittleFS.exists(CAL_FILE)) return false;

  File f = LittleFS.open(CAL_FILE, "r");
  if (!f) return false;

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  // Servo calibration
  JsonArray arr = doc["servos"];
  if (arr && arr.size() == 28) {
    for (int i = 0; i < 28; i++) {
      cal[i].open     = arr[i]["open"]   | 340;
      cal[i].closed   = arr[i]["closed"] | 300;
      cal[i].inverted = arr[i]["inv"]    | false;
    }
  }

  // Smooth settings
  if (doc.containsKey("step"))        servoStepSize    = doc["step"];
  if (doc.containsKey("delayMs"))     servoStepDelayMs = doc["delayMs"];
  if (doc.containsKey("powerHoldMs")) servoPowerHoldMs = doc["powerHoldMs"];

  // WiFi credentials
  if (doc.containsKey("ssid")) wifiSSID = doc["ssid"].as<String>();
  if (doc.containsKey("pass")) wifiPASS = doc["pass"].as<String>();

  // Timezone
  String tzName = doc["timezone"] | "Europe/Copenhagen";
  setTimezoneByName(tzName);

  return true;
}

bool setTimezoneByName(const String& name) {
  for (int i = 0; i < TZ_COUNT; i++) {
    if (name == tzOptions[i].name) {
      currentTimezoneName = tzOptions[i].name;
      currentTzValue      = tzOptions[i].tz;
      return true;
    }
  }
  return false;
}

// Both settings functions now delegate to the unified calibration file.
bool saveClockSettings() {
  return saveCalibration();
}

bool loadClockSettings() {
  return loadCalibration();
}
