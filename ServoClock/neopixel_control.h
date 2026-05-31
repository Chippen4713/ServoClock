#ifndef NEOPIXEL_CONTROL_H
#define NEOPIXEL_CONTROL_H

#include <Arduino.h>

enum LedEffect { EFFECT_SOLID, EFFECT_BREATHING, EFFECT_RAINBOW, EFFECT_SPARKLE };

void setupNeoPixel();
void setLedsOn(bool on);
void setLedColor(uint8_t r, uint8_t g, uint8_t b);
void setLedBrightness(uint8_t brightness);
void setLedEffect(LedEffect effect);
void updateLedAnimation();
void publishLedState();
void handleLedCommand(const String& json);

#endif
