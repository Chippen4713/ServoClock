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
static unsigned long servoPowerOnMs = 0;
static const unsigned long SERVO_POWER_STABILIZE_MS = 1000;

void setServoPower(bool enabled) {
  // Change HIGH/LOW if your relay modules are active LOW
  digitalWrite(RELAY_BOARD1_PIN, enabled ? LOW : HIGH);
  digitalWrite(RELAY_BOARD2_PIN, enabled ? LOW : HIGH);

  servoPowerEnabled = enabled;
  if (enabled) servoPowerOnMs = millis();

  Serial.printf("Servo power %s\n", enabled ? "ON" : "OFF");
}

void requestServoPowerHold() {
  if (!servoPowerEnabled) {
    setServoPower(true);
  }
  servoPowerOffAtMs = millis() + servoPowerHoldMs;
}

bool anyServoMoving() {
  for (int i = 0; i < 28; i++) {
    if (currentPos[i] != servoTargetPos[i]) {
      return true;
    }
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

void moveServoSmooth(int servoNum, uint16_t targetPulse, uint16_t stepSize, uint16_t stepDelayMs) {
  targetPulse = constrainPulse(targetPulse);

  uint16_t current = currentPos[servoNum];
  if (current == targetPulse) return;

  if (stepSize == 0) stepSize = 1;

  if (current < targetPulse) {
    for (uint16_t p = current; p < targetPulse; p += stepSize) {
      writeServoPwm(servoNum, p);
      delay(stepDelayMs);
    }
  } else {
    for (int p = current; p > targetPulse; p -= stepSize) {
      writeServoPwm(servoNum, p);
      delay(stepDelayMs);
    }
  }

  writeServoPwm(servoNum, targetPulse);
  currentPos[servoNum] = targetPulse;

  Serial.printf("Servo %d -> SMOOTH actual=%u\n", servoNum, targetPulse);
}

void moveServoSmooth(int servoNum, uint16_t targetPulse) {
  requestServoPowerHold();
  servoTargetPos[servoNum] = constrainPulse(targetPulse);
}

void updateServoSmooth() {
  if (!servoPowerEnabled) return;

  unsigned long now = millis();

  // Wait for relay/power to stabilize before moving
  if (now - servoPowerOnMs < SERVO_POWER_STABILIZE_MS) return;

  // Staggered tick — alternate board1 (0-13) and board2 (14-27) each step
  // This halves peak current draw and reduces PSU voltage sag
  static unsigned long lastStepMs = 0;
  static bool board1Turn = true;
  if (now - lastStepMs < servoStepDelayMs) return;
  lastStepMs = now;

  int start = board1Turn ?  0 : 14;
  int end   = board1Turn ? 14 : 28;
  board1Turn = !board1Turn;

  for (int i = start; i < end; i++) {
    uint16_t current = currentPos[i];
    uint16_t target  = servoTargetPos[i];
    if (current == target) continue;

    if (current < target) {
      uint32_t next = (uint32_t)current + servoStepSize;
      currentPos[i] = (next >= target) ? target : (uint16_t)next;
    } else {
      int next = (int)current - (int)servoStepSize;
      currentPos[i] = (next <= (int)target) ? target : (uint16_t)next;
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
  if (state == 'o') {
    moveServoSmooth(servoNum, cal[servoNum].open);
  } else if (state == 'c') {
    moveServoSmooth(servoNum, cal[servoNum].closed);
  }
}

void printServo(int s) {
  Serial.printf("Servo %d -> current=%u actual=%u open=%u closed=%u inverted=%s\n",
                s,
                currentPos[s],
                applyServoInversion(s, currentPos[s]),
                cal[s].open,
                cal[s].closed,
                cal[s].inverted ? "true" : "false");
  webLogf("Servo %d  cur=%u  open=%u  closed=%u  inv=%s",
          s, currentPos[s], cal[s].open, cal[s].closed,
          cal[s].inverted ? "Y" : "N");
}

void printAll() {
  for (int i = 0; i < 28; i++) {
    Serial.printf("%2d: open=%u closed=%u current=%u inverted=%s\n",
                  i,
                  cal[i].open,
                  cal[i].closed,
                  currentPos[i],
                  cal[i].inverted ? "true" : "false");
  }
}

void setDefaults() {
  bool invertedList[28] = {
    true, true, false, true, false, true, true, true,
    true, false, true, false, true, true, true, true,
    false, true, false, true, true, true, true, false,
    true, false, true, true
  };

  for (int i = 0; i < 28; i++) {
    cal[i].open = 530;
    cal[i].closed = 300;
    cal[i].inverted = invertedList[i];
    currentPos[i] = cal[i].closed;
    servoTargetPos[i] = cal[i].closed;
    servoLastStepMs[i] = 0;
  }
}


void moveAllToClosed() {
  requestServoPowerHold();
  for (int i = 0; i < 28; i++) {
    servoTargetPos[i] = cal[i].closed;
  }
}

void moveAllToOpen() {
  requestServoPowerHold();
  for (int i = 0; i < 28; i++) {
    servoTargetPos[i] = cal[i].open;
  }
}

void applyDigitPatternToServos(int result[28]) {
  for (int i = 0; i < 28; i++) {
    bool open = (result[i] == 1);
    if (cal[i].inverted) open = !open;
    moveToStored(i, open ? 'o' : 'c');
  }
}