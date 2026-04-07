#include "globals.h"
#include "servo_control.h"
#include "clock_control.h"
#include "storage.h"
#include "connectivity.h"
#include "menu_control.h"

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
  pinMode(LED_ESP_PINT, OUTPUT);
  setServoPower(false);
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

  startWiFiConnect();
  if (WiFi.status() == WL_CONNECTED) {
    syncRtcFromNtp();
    connectMqtt();
  }

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
  updateServoSmooth();
  updateServoPower();

  if (clockRunning) {
    updateClock();
  }
}