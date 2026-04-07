#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include "globals.h"

void setupWebInterface();
void updateWebInterface();
void webLog(const char* line);
void webLog(const String& line);
void webLogf(const char* fmt, ...);

#endif
