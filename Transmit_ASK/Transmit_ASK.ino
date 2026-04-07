#include "globals.h"
#include "servo_control.h"
#include "clock_control.h"
#include "storage.h"
#include "connectivity.h"
#include "menu_control.h"

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("====================================");
  Serial.println("Servo Clock Program");
  Serial.println("====================================");

  pinMode(PIR_PIN, INPUT);
  Wire.begin(D2, D1);

  board1.begin();
  board2.begin();
  board1.setPWMFreq(50);
  board2.setPWMFreq(50);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
  }

  setDefaults();
  loadCalibration();
  loadClockSettings();

  for (int i = 0; i < 28; i++) {
    currentPos[i] = cal[i].closed;
  }

  setupConnectivity();

  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    syncRtcFromNtp();
    connectMqtt();
  }

  setMode(MODE_MAIN);
  printServo(selectedServo);
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    routeCommand(cmd);
  }

  maintainConnectivity();

  if (clockRunning) {
    updateClock();
  }
}