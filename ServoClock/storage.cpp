#include "storage.h"

bool saveCalibration() {
  DynamicJsonDocument doc(6144);
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < 28; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["open"] = cal[i].open;
    obj["closed"] = cal[i].closed;
    obj["inv"] = cal[i].inverted;
  }

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

  DynamicJsonDocument doc(6144);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  JsonArray arr = doc.as<JsonArray>();
  if (arr.size() != 28) return false;

  for (int i = 0; i < 28; i++) {
    cal[i].open = arr[i]["open"] | 340;
    cal[i].closed = arr[i]["closed"] | 300;
    cal[i].inverted = arr[i]["inv"] | false;
  }

  return true;
}

bool setTimezoneByName(const String& name) {
  for (int i = 0; i < TZ_COUNT; i++) {
    if (name == tzOptions[i].name) {
      currentTimezoneName = tzOptions[i].name;
      currentTzValue = tzOptions[i].tz;
      return true;
    }
  }
  return false;
}

bool saveClockSettings() {
  DynamicJsonDocument doc(512);
  doc["timezone"] = currentTimezoneName;

  File f = LittleFS.open(SETTINGS_FILE, "w");
  if (!f) return false;

  size_t written = serializeJsonPretty(doc, f);
  f.close();
  return written > 0;
}

bool loadClockSettings() {
  if (!LittleFS.exists(SETTINGS_FILE)) return false;

  File f = LittleFS.open(SETTINGS_FILE, "r");
  if (!f) return false;

  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  String tzName = doc["timezone"] | "Europe/Copenhagen";
  return setTimezoneByName(tzName);
}