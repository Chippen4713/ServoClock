#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include "globals.h"

void setupConnectivity();
void connectWiFi();
void ensureWiFi();
void printWiFiStatus();

void startWiFiConnect();
void updateWiFi();

void startNtpSync();
void updateNtpSync();

bool connectMqtt();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttPublishRetained(const char* topic, const String& payload);

String isoTimestampNow();
bool syncRtcFromNtp();

void publishDiscovery();
void publishStates();
void publishClockTimeState(int hour, int minute);
void updateSensors();
void maintainConnectivity();

#endif