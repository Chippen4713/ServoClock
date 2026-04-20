#include "globals.h"
#include "servo_control.h"
#include "clock_control.h"
#include "storage.h"
#include "connectivity.h"
#include "menu_control.h"
#include "web_interface.h"

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);

  Serial.println();
  Serial.println("====================================");
  Serial.println("Servo Clock Program");
  Serial.println("====================================");

  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_BOARD1_PIN, OUTPUT);
  pinMode(RELAY_BOARD2_PIN, OUTPUT);
  setServoPower(false);
  dhtSensor.begin();
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

  // Set current to open so smooth motion has distance to travel to closed
  for (int i = 0; i < 28; i++) {
    currentPos[i]     = cal[i].open;
    servoTargetPos[i] = cal[i].open;
  }

  setupConnectivity();

  startWiFiConnect();
  if (WiFi.status() == WL_CONNECTED) {
    syncRtcFromNtp();
    connectMqtt();
  }

  moveAllToClosed();  // triggers relay + sets all targets to closed

  setMode(MODE_MAIN);
  printServo(selectedServo);
}

void loop() {
  static String inputBuffer = "";

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      inputBuffer.trim();
      if (inputBuffer.length() > 0) {
        routeCommand(inputBuffer);
      }
      inputBuffer = "";
    } else {
      inputBuffer += c;
    }
  }

  maintainConnectivity();
  updateSensors();
  updateWebInterface();
  updateServoSmooth();
  updateServoPower();

  if (clockRunning) {
    updateClock();
  }
}