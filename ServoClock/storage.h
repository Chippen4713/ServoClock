#ifndef STORAGE_H
#define STORAGE_H

#include "globals.h"

bool saveCalibration();
bool loadCalibration();
bool saveClockSettings();
bool loadClockSettings();
bool setTimezoneByName(const String& name);

#endif