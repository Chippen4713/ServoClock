#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include "globals.h"

uint16_t constrainPulse(uint16_t pulse);
uint16_t applyServoInversion(int servoNum, uint16_t logicalPulse);
void setClockServoRaw(int servoNum, uint16_t logicalPulse);
void setClockServoActual(int servoNum, uint16_t actualPulse);
void moveServoSmooth(int servoNum, uint16_t targetPulse);
void updateServoSmooth();
void moveSelected(int delta);
void printServo(int s);
void printAll();
void setDefaults();
void moveToStored(int servoNum, char state);
void moveAllToOpen();
void moveAllToClosed();
void applyDigitPatternToServos(int result[28]);
void setServoPower(bool enabled);
void requestServoPowerHold();
void updateServoPower();
bool anyServoMoving();

#endif