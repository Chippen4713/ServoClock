#include "servo_control.h"
#include "web_interface.h"

uint16_t constrainPulse(uint16_t pulse) {
  return constrain(pulse, 0, 600);
}

uint16_t applyServoInversion(int servoNum, uint16_t logicalPulse) {
  return constrainPulse(logicalPulse);
}

static void writeServoPwm(int servoNum, uint16_t pulse) {
  pulse = constrainPulse(pulse);

  if (!servoPowerEnabled) return;

  if (servoNum >= 0 && servoNum <= 13) {
    board1.setPWM(servoNum, 0, pulse);
  } else if (servoNum >= 14 && servoNum <= 27) {
    board2.setPWM(servoNum - 14, 0, pulse);
  }
}

void setClockServoRaw(int servoNum, uint16_t logicalPulse) {
  logicalPulse = constrainPulse(logicalPulse);
  writeServoPwm(servoNum, logicalPulse);
  Serial.printf("Servo %d -> actual=%u\n", servoNum, logicalPulse);
}

void setClockServoActual(int servoNum, uint16_t actualPulse) {
  actualPulse = constrainPulse(actualPulse);
  writeServoPwm(servoNum, actualPulse);
  Serial.printf("Servo %d -> DIRECT actual=%u\n", servoNum, actualPulse);
}

void setServoPower(bool enabled) {
  digitalWrite(RELAY_BOARD1_PIN, enabled ? LOW : HIGH);
  digitalWrite(RELAY_BOARD2_PIN, enabled ? LOW : HIGH);
  servoPowerEnabled = enabled;
  Serial.printf("Servo power %s\n", enabled ? "ON" : "OFF");
}

void requestServoPowerHold() {
  if (!servoPowerEnabled) setServoPower(true);
  servoPowerOffAtMs = millis() + servoPowerHoldMs;
}

bool anyServoMoving() {
  for (int i = 0; i < 28; i++) {
    if (currentPos[i] != servoTargetPos[i]) return true;
  }
  return false;
}

static void updateBoardLeds() {
  bool board1Moving = false;
  bool board2Moving = false;
  for (int i = 0; i <= 13; i++) {
    if (currentPos[i] != servoTargetPos[i]) { board1Moving = true; break; }
  }
  for (int i = 14; i <= 27; i++) {
    if (currentPos[i] != servoTargetPos[i]) { board2Moving = true; break; }
  }
  board1.setPWM(LED_BOARD1_PIN, 0, board1Moving ? 4095 : 0);
  board2.setPWM(LED_BOARD2_PIN, 0, board2Moving ? 4095 : 0);
}

void updateServoPower() {
  if (currentMode == MODE_SERVO_CAL) {
    if (!servoPowerEnabled) setServoPower(true);
    servoPowerOffAtMs = 0;
    return;
  }

  if (!servoPowerEnabled) return;

  if (anyServoMoving()) {
    servoPowerOffAtMs = millis() + servoPowerHoldMs;
    return;
  }

  if (servoPowerOffAtMs > 0 && millis() >= servoPowerOffAtMs) {
    setServoPower(false);
    servoPowerOffAtMs = 0;
  }
}

void moveServoSmooth(int servoNum, uint16_t targetPulse) {
  requestServoPowerHold();
  servoTargetPos[servoNum] = constrainPulse(targetPulse);
}

void updateServoSmooth() {
  if (!servoPowerEnabled) return;

  unsigned long now = millis();

  for (int i = 0; i < 28; i++) {
    uint16_t current = currentPos[i];
    uint16_t target  = servoTargetPos[i];

    if (current == target) continue;
    if (now - servoLastStepMs[i] < servoStepDelayMs) continue;

    servoLastStepMs[i] = now;

    if (current < target) {
      uint32_t next = current + servoStepSize;
      if (next > target) next = target;
      currentPos[i] = (uint16_t)next;
    } else {
      int next = (int)current - (int)servoStepSize;
      if (next < target) next = target;
      currentPos[i] = (uint16_t)next;
    }

    writeServoPwm(i, currentPos[i]);
  }

  updateBoardLeds();
}

void moveSelected(int delta) {
  int next = (int)servoTargetPos[selectedServo] + delta;
  moveServoSmooth(selectedServo, constrainPulse(next));
}

void moveToStored(int servoNum, char state) {
  if (state == 'o') moveServoSmooth(servoNum, cal[servoNum].open);
  else if (state == 'c') moveServoSmooth(servoNum, cal[servoNum].closed);
}

void moveAllToOpen() {
  requestServoPowerHold();
  for (int i = 0; i < 28; i++) servoTargetPos[i] = cal[i].open;
}

void moveAllToClosed() {
  requestServoPowerHold();
  for (int i = 0; i < 28; i++) servoTargetPos[i] = cal[i].closed;
}

void printServo(int s) {
  Serial.printf("Servo %d -> current=%u open=%u closed=%u inverted=%s\n",
                s, currentPos[s], cal[s].open, cal[s].closed,
                cal[s].inverted ? "true" : "false");
  webLogf("Servo %d  cur=%u  open=%u  closed=%u  inv=%s",
          s, currentPos[s], cal[s].open, cal[s].closed,
          cal[s].inverted ? "Y" : "N");
}

void printAll() {
  for (int i = 0; i < 28; i++) {
    printServo(i);
  }
}

void setDefaults() {
  for (int i = 0; i < 28; i++) {
    cal[i].open    = 530;
    cal[i].closed  = 300;
    cal[i].inverted = false;
    currentPos[i]      = cal[i].closed;
    servoTargetPos[i]  = cal[i].closed;
    servoLastStepMs[i] = 0;
  }
}

void applyDigitPatternToServos(int result[28]) {
  for (int i = 0; i < 28; i++) {
    moveToStored(i, result[i] == 1 ? 'o' : 'c');
  }
}
