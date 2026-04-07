#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver board1(0x40);
Adafruit_PWMServoDriver board2(0x41);

void scanI2C() {
  Serial.println("Scanning I2C...");
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(D2, D1);

  scanI2C();

  board1.begin();
  board2.begin();

  board1.setPWMFreq(50);
  board2.setPWMFreq(50);

  Serial.println("Test starting...");
}

void loop() {
   scanI2C();
  delay(2000);


 
}