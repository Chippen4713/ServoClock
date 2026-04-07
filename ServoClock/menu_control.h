#ifndef MENU_CONTROL_H
#define MENU_CONTROL_H

#include "globals.h"

void printMainMenu();
void printServoMenu();
void printSmoothMenu();
void printRTCMenu();
void printMotionMenu();
void printWiFiMenu();
void printRunMenu();

void setMode(AppMode newMode);
void handleServoCommand(String cmd);
void handleSmoothCommand(String cmd);
void handleRTCCommand(String cmd);
void handleMotionCommand(String cmd);
void handleWiFiCommand(String cmd);
void handleMainCommand(String cmd);
void handleRunCommand(String cmd);
void routeCommand(String cmd);

#endif