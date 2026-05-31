#include "neopixel_control.h"
#include "globals.h"
#include "connectivity.h"
#include "web_interface.h"
#include <ArduinoJson.h>

static uint8_t   ledR = 255, ledG = 200, ledB = 120;  // warm white default
static uint8_t   ledBrightness = 200;
static LedEffect ledEffect = EFFECT_SOLID;

static void hsvToRgb(uint8_t h, uint8_t s, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
  if (s == 0) { r = g = b = v; return; }
  uint8_t region    = h / 43;
  uint8_t remainder = (h - region * 43) * 6;
  uint8_t p = (uint16_t)v * (255 - s) >> 8;
  uint8_t q = (uint16_t)v * (255 - ((uint16_t)s * remainder >> 8)) >> 8;
  uint8_t t = (uint16_t)v * (255 - ((uint16_t)s * (255 - remainder) >> 8)) >> 8;
  switch (region) {
    case 0: r=v; g=t; b=p; break;
    case 1: r=q; g=v; b=p; break;
    case 2: r=p; g=v; b=t; break;
    case 3: r=p; g=q; b=v; break;
    case 4: r=t; g=p; b=v; break;
    default: r=v; g=p; b=q; break;
  }
}

void setupNeoPixel() {
  strip.begin();
  strip.clear();
  strip.show();
}

void setLedsOn(bool on) {
  ledsOn = on;
  if (!on) {
    strip.clear();
    strip.show();
  }
  publishLedState();
}

void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  ledR = r; ledG = g; ledB = b;
}

void setLedBrightness(uint8_t brightness) {
  ledBrightness = brightness;
}

void setLedEffect(LedEffect effect) {
  ledEffect = effect;
}

void updateLedAnimation() {
  if (!ledsOn) return;

  static unsigned long lastFrame = 0;
  unsigned long now = millis();
  if (now - lastFrame < 30) return;
  lastFrame = now;

  switch (ledEffect) {

    case EFFECT_SOLID: {
      strip.setBrightness(ledBrightness);
      for (int i = 0; i < strip.numPixels(); i++)
        strip.setPixelColor(i, ledR, ledG, ledB);
      strip.show();
      break;
    }

    case EFFECT_BREATHING: {
      uint32_t t = now % 3000;
      uint8_t b = t < 1500 ? map(t, 0, 1500, 8, ledBrightness)
                           : map(t, 1500, 3000, ledBrightness, 8);
      strip.setBrightness(b);
      for (int i = 0; i < strip.numPixels(); i++)
        strip.setPixelColor(i, ledR, ledG, ledB);
      strip.show();
      break;
    }

    case EFFECT_RAINBOW: {
      strip.setBrightness(ledBrightness);
      uint8_t offset = (now / 15) & 0xFF;
      for (int i = 0; i < strip.numPixels(); i++) {
        uint8_t hue = offset + (uint8_t)(i * 256 / strip.numPixels());
        uint8_t r, g, b;
        hsvToRgb(hue, 220, 255, r, g, b);
        strip.setPixelColor(i, r, g, b);
      }
      strip.show();
      break;
    }

    case EFFECT_SPARKLE: {
      static uint8_t spark[NEOPIXEL_COUNT] = {0};
      strip.setBrightness(ledBrightness);
      if (random(8) < 3)
        spark[random(strip.numPixels())] = 255;
      for (int i = 0; i < strip.numPixels(); i++) {
        if (spark[i] > 25) spark[i] -= 25;
        else spark[i] = 0;
        uint8_t sv = spark[i];
        strip.setPixelColor(i,
          (uint16_t)ledR * sv / 255,
          (uint16_t)ledG * sv / 255,
          (uint16_t)ledB * sv / 255);
      }
      strip.show();
      break;
    }
  }
}

void publishLedState() {
  if (!mqtt.connected()) return;

  StaticJsonDocument<200> doc;
  doc["state"] = ledsOn ? "ON" : "OFF";
  doc["color_mode"] = "rgb";
  doc["brightness"] = ledBrightness;
  doc["effect"] = ledEffect == EFFECT_BREATHING ? "breathing"
                : ledEffect == EFFECT_RAINBOW   ? "rainbow"
                : ledEffect == EFFECT_SPARKLE   ? "sparkle"
                                                : "solid";
  JsonObject color = doc.createNestedObject("color");
  color["r"] = ledR;
  color["g"] = ledG;
  color["b"] = ledB;

  char buf[200];
  serializeJson(doc, buf);
  mqttPublishRetained(TOPIC_LIGHT_STATE, String(buf));
}

void handleLedCommand(const String& json) {
  Serial.print("[LED] CMD: ");
  Serial.println(json);
  webLogf("[LED] CMD: %s", json.c_str());

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err != DeserializationError::Ok) {
    Serial.printf("[LED] JSON error: %s\n", err.c_str());
    webLogf("[LED] JSON error: %s", err.c_str());
    return;
  }

  if (doc.containsKey("color")) {
    setLedColor(doc["color"]["r"], doc["color"]["g"], doc["color"]["b"]);
  }
  if (doc.containsKey("brightness")) {
    setLedBrightness(doc["brightness"]);
  }
  if (doc.containsKey("effect")) {
    String e = doc["effect"].as<String>();
    if      (e == "breathing") setLedEffect(EFFECT_BREATHING);
    else if (e == "rainbow")   setLedEffect(EFFECT_RAINBOW);
    else if (e == "sparkle")   setLedEffect(EFFECT_SPARKLE);
    else                       setLedEffect(EFFECT_SOLID);
  }
  if (doc.containsKey("state")) {
    bool on = String(doc["state"].as<const char*>()) == "ON";
    setLedsOn(on);
    Serial.printf("[LED] state=%s ledsOn=%d\n", on ? "ON" : "OFF", ledsOn);
  } else {
    publishLedState();
  }
}
