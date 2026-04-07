#include "clock_control.h"
#include "servo_control.h"
#include "connectivity.h"

void printRTCNow() {
  clockRTC.updateTime();
  Serial.printf("RTC: %04d-%02d-%02d  %02d:%02d:%02d\n",
                clockRTC.year, clockRTC.month, clockRTC.dayofmonth,
                clockRTC.hours, clockRTC.minutes, clockRTC.seconds);
}

String rtcTimeString() {
  clockRTC.updateTime();
  char buf[20];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
           clockRTC.hours, clockRTC.minutes, clockRTC.seconds);
  return String(buf);
}

void setRTCFromCompileTime() {
  clockRTC.setDS1302Time(0, 0, 12, 1, 6, 4, 2026);
  Serial.println("RTC set to fixed compile-time example value");
  printRTCNow();
  publishStates();
}

void setTimeFromRTC() {
  clockRTC.updateTime();

  int result[28];
  int index = 0;

  int hour = clockRTC.hours;
  int minute = clockRTC.minutes;

  int t[] = {
    hour / 10,
    hour % 10,
    minute / 10,
    minute % 10
  };

  for (int i = 0; i < 4; i++) {
    int* d = digits[t[i]];
    for (int j = 0; j < 7; j++) {
      result[index++] = d[j];
    }
  }

  applyDigitPatternToServos(result);

  lastHour = hour;
  lastMinute = minute;

  Serial.printf("Clock updated -> %02d:%02d\n", hour, minute);
  publishClockTimeState(hour, minute);
  mqttPublishRetained(TOPIC_STATE_RTC_TIME, rtcTimeString());
}

void startClock() {
  clockRunning = true;
  lastHour = -1;
  lastMinute = -1;
  setTimeFromRTC();
  mqttPublishRetained(TOPIC_STATE_RUNNING, "ON");
  Serial.println("Clock started");
}

void stopClock() {
  clockRunning = false;
  mqttPublishRetained(TOPIC_STATE_RUNNING, "OFF");
  Serial.println("Clock stopped");
}

void updateClock() {
  if (millis() - lastClockCheck < 1000) return;
  lastClockCheck = millis();

  clockRTC.updateTime();

  if (clockRTC.hours != lastHour || clockRTC.minutes != lastMinute) {
    setTimeFromRTC();
  }
}