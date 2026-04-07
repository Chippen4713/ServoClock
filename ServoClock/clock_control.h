#ifndef CLOCK_CONTROL_H
#define CLOCK_CONTROL_H

#include "globals.h"

void printRTCNow();
String rtcTimeString();
void setRTCFromCompileTime();
void setTimeFromRTC();
void startClock();
void stopClock();
void updateClock();

#endif