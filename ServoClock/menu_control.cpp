#include "menu_control.h"
#include "servo_control.h"
#include "clock_control.h"
#include "storage.h"
#include "connectivity.h"
#include "web_interface.h"

#include <stdlib.h>

static bool parseIntStrict(const String& s, int& outValue) {
  String t = s;
  t.trim();

  if (t.length() == 0) return false;

  char buf[24];
  size_t len = t.length();
  if (len >= sizeof(buf)) return false;

  t.toCharArray(buf, sizeof(buf));

  char* endPtr = nullptr;
  long value = strtol(buf, &endPtr, 10);

  if (*endPtr != '\0') return false;

  outValue = (int)value;
  return true;
}

static bool parseCommandIntArg(const String& cmd, const char* prefix, int& outValue) {
  String p = String(prefix);
  if (!cmd.startsWith(p)) return false;

  String arg = cmd.substring(p.length());
  return parseIntStrict(arg, outValue);
}

static bool nextWord(const String& text, int& index, String& word) {
  int len = text.length();

  while (index < len && text[index] == ' ') index++;
  if (index >= len) return false;

  int start = index;
  while (index < len && text[index] != ' ') index++;

  word = text.substring(start, index);
  return true;
}

static void clearConsole() {
  Serial.write(27);
  Serial.print("[2J");
  Serial.write(27);
  Serial.print("[H");

  for (int i = 0; i < 6; i++) {
    Serial.println();
  }
}

static void printDivider() {
  Serial.println("=================================");
}

static void printSection(const char* title) {
  printDivider();
  Serial.printf("  %s\n", title);
  printDivider();
}

static void printPrompt() {
  Serial.println();
  Serial.print("> ");
}

static void printUnknownCommand(const char* context) {
  Serial.println();
  Serial.printf("[ERROR] Unknown %s command\n", context);
  Serial.println("        Type 'h' to show this menu.");
  webLogf("[ERROR] Unknown %s command", context);
  printPrompt();
}

static void printOk(const char* msg) {
  Serial.println();
  Serial.print("[OK] ");
  Serial.println(msg);
  webLogf("[OK] %s", msg);
  printPrompt();
}

static void showCurrentMenu() {
  clearConsole();

  switch (currentMode) {
    case MODE_MAIN:      printMainMenu(); break;
    case MODE_SERVO_CAL: printServoMenu(); break;
    case MODE_SMOOTH:    printSmoothMenu(); break;
    case MODE_RTC:       printRTCMenu(); break;
    case MODE_MOTION:    printMotionMenu(); break;
    case MODE_WIFI:      printWiFiMenu(); break;
    case MODE_RUN:       printRunMenu(); break;
    default: break;
  }

  printPrompt();
}

void printMainMenu() {
  printSection("MAIN MENU");

  Serial.println("1  -> Servo calibration");
  Serial.println("2  -> Smooth settings");
  Serial.println("3  -> RTC menu");
  Serial.println("4  -> Motion sensor menu");
  Serial.println("5  -> WiFi / Internet menu");
  Serial.println("6  -> Run clock mode");
  Serial.println("7  -> Show calibration");
  Serial.println("h  -> Show this menu");
}

void printServoMenu() {
  printSection("SERVO CALIBRATION");

  Serial.println("Selection:");
  Serial.println("  s <n>      select servo 0..27");

  Serial.println();
  Serial.println("Movement:");
  Serial.println("  +          move selected +10");
  Serial.println("  -          move selected -10");
  Serial.println("  ++         move selected +2");
  Serial.println("  --         move selected -2");

  Serial.println();
  Serial.println("Position:");
  Serial.println("  g <value>  go to logical value 220..420");
  Serial.println("  a <value>  send direct actual PWM 220..420");

  Serial.println();
  Serial.println("Calibration:");
  Serial.println("  i          toggle invert direction");
  Serial.println("  o          save current as OPEN");
  Serial.println("  c          save current as CLOSED");
  Serial.println("  to         move selected to OPEN");
  Serial.println("  tc         move selected to CLOSED");

  Serial.println();
  Serial.println("Info:");
  Serial.println("  p          print selected");
  Serial.println("  pa         print all");

  Serial.println();
  Serial.println("Storage:");
  Serial.println("  w          write calibration file");
  Serial.println("  l          load calibration file");

  Serial.println();
  Serial.println("Other:");
  Serial.println("  h          show this menu");
  Serial.println("  m          back to main menu");
}

void printSmoothMenu() {
  printSection("SMOOTH SETTINGS");

  Serial.println("show             show current smooth settings");
  Serial.println("step <n>         set global step size");
  Serial.println("delay <n>        set global step delay ms");
  Serial.println("test <n>         test selected servo with n pulse");
  Serial.println("powerhold <n>    Tune how long servo power stays on");
  Serial.println("h                show this menu");
  Serial.println("m                back to main menu");
}

void printRTCMenu() {
  printSection("RTC MENU");

  Serial.println("status           show RTC time");
  Serial.println("setfixed         set RTC to compile/example time");
  Serial.println("syncntp          sync RTC from NTP");
  Serial.println("watch            print RTC time 10 times");
  Serial.println("h                show this menu");
  Serial.println("m                back to main menu");
}

void printMotionMenu() {
  printSection("MOTION MENU");

  Serial.println("read             read PIR pin");
  Serial.println("watch            live PIR monitor for 10 sec");
  Serial.println("h                show this menu");
  Serial.println("m                back to main menu");
}

void printWiFiMenu() {
  printSection("WIFI MENU");

  Serial.println("status                 show WiFi status");
  Serial.println("scan                   scan WiFi");
  Serial.println("ssid <name>            set WiFi SSID");
  Serial.println("pass <password>        set WiFi password");
  Serial.println("connect                connect WiFi");
  Serial.println("disconnect             disconnect WiFi");
  Serial.println("h                      show this menu");
  Serial.println("m                      back to main menu");
}

void printRunMenu() {
  printSection("RUN CLOCK MODE");

  Serial.println("start            start clock");
  Serial.println("stop             stop clock");
  Serial.println("time             show current RTC time");
  Serial.println("h                show this menu");
  Serial.println("m                back to main menu");
}

static const char* modeName(AppMode m) {
  switch (m) {
    case MODE_MAIN:      return "MAIN";
    case MODE_SERVO_CAL: return "SERVO CAL";
    case MODE_SMOOTH:    return "SMOOTH";
    case MODE_RTC:       return "RTC";
    case MODE_MOTION:    return "MOTION";
    case MODE_WIFI:      return "WIFI";
    case MODE_RUN:       return "RUN";
    default:             return "?";
  }
}

static void webLogHelp(AppMode m) {
  switch (m) {
    case MODE_MAIN:
      webLog("=================================");
      webLog("  MAIN MENU");
      webLog("=================================");
      webLog("1  -> Servo calibration");
      webLog("2  -> Smooth settings");
      webLog("3  -> RTC menu");
      webLog("4  -> Motion sensor menu");
      webLog("5  -> WiFi / Internet menu");
      webLog("6  -> Run clock mode");
      webLog("7  -> Show calibration");
      webLog("h  -> Show this menu");
      break;

    case MODE_SERVO_CAL:
      webLog("=================================");
      webLog("  SERVO CALIBRATION");
      webLog("=================================");
      webLog("Selection:");
      webLog("  s <n>      select servo 0..27");
      webLog("");
      webLog("Movement:");
      webLog("  +          move selected +10");
      webLog("  -          move selected -10");
      webLog("  ++         move selected +2");
      webLog("  --         move selected -2");
      webLog("");
      webLog("Position:");
      webLog("  g <value>  go to logical value 220..420");
      webLog("  a <value>  send direct actual PWM 220..420");
      webLog("");
      webLog("Calibration:");
      webLog("  i          toggle invert direction");
      webLog("  o          save current as OPEN");
      webLog("  c          save current as CLOSED");
      webLog("  to         move selected to OPEN");
      webLog("  tc         move selected to CLOSED");
      webLog("");
      webLog("Info:");
      webLog("  p          print selected");
      webLog("  pa         print all");
      webLog("");
      webLog("Storage:");
      webLog("  w          write calibration file");
      webLog("  l          load calibration file");
      webLog("");
      webLog("Other:");
      webLog("  h          show this menu");
      webLog("  m          back to main menu");
      break;

    case MODE_SMOOTH:
      webLog("=================================");
      webLog("  SMOOTH SETTINGS");
      webLog("=================================");
      webLog("show             show current smooth settings");
      webLog("step <n>         set global step size");
      webLog("delay <n>        set global step delay ms");
      webLog("test <n>         test selected servo with n pulse");
      webLog("powerhold <n>    Tune how long servo power stays on");
      webLog("h                show this menu");
      webLog("m                back to main menu");
      break;

    case MODE_RTC:
      webLog("=================================");
      webLog("  RTC MENU");
      webLog("=================================");
      webLog("status           show RTC time");
      webLog("setfixed         set RTC to compile/example time");
      webLog("syncntp          sync RTC from NTP");
      webLog("watch            print RTC time 10 times");
      webLog("h                show this menu");
      webLog("m                back to main menu");
      break;

    case MODE_MOTION:
      webLog("=================================");
      webLog("  MOTION MENU");
      webLog("=================================");
      webLog("read             read PIR pin");
      webLog("watch            live PIR monitor for 10 sec");
      webLog("h                show this menu");
      webLog("m                back to main menu");
      break;

    case MODE_WIFI:
      webLog("=================================");
      webLog("  WIFI MENU");
      webLog("=================================");
      webLog("status                 show WiFi status");
      webLog("scan                   scan WiFi");
      webLog("ssid <name>            set WiFi SSID");
      webLog("pass <password>        set WiFi password");
      webLog("connect                connect WiFi");
      webLog("disconnect             disconnect WiFi");
      webLog("h                      show this menu");
      webLog("m                      back to main menu");
      break;

    case MODE_RUN:
      webLog("=================================");
      webLog("  RUN CLOCK MODE");
      webLog("=================================");
      webLog("start            start clock");
      webLog("stop             stop clock");
      webLog("time             show current RTC time");
      webLog("h                show this menu");
      webLog("m                back to main menu");
      break;

    default: break;
  }
}

void setMode(AppMode newMode) {
  currentMode = newMode;
  showCurrentMenu();
  webLogf("[SYS] Mode: %s", modeName(newMode));
  webLogHelp(newMode);
}

void handleSerialInput() {
  static String inputBuffer = "";

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      inputBuffer.trim();

      if (inputBuffer.length() > 0) {
        routeCommand(inputBuffer);
      } else {
        printPrompt();
      }

      inputBuffer = "";
    } else {
      inputBuffer += c;
    }
  }
}

static bool handleServoToken(const String& token, const String& nextToken, bool hasNextToken, bool& consumedNextToken, bool& shouldExitMode) {
  consumedNextToken = false;
  shouldExitMode = false;

  if (token == "h") {
    showCurrentMenu();
    webLogHelp(currentMode);
    return true;
  }

  if (token == "m") {
    setMode(MODE_MAIN);
    shouldExitMode = true;
    return true;
  }

  int n = 0;

  if (token == "s") {
    if (!hasNextToken) {
      Serial.println();
      Serial.println("[ERROR] Missing servo number");
      Serial.println("        Use: s <n>");
      printPrompt();
      return true;
    }

    if (!parseIntStrict(nextToken, n)) {
      Serial.println();
      Serial.println("[ERROR] Invalid servo number");
      Serial.println("        Use: s <n>");
      consumedNextToken = true;
      printPrompt();
      return true;
    }

    consumedNextToken = true;

    if (n >= 0 && n < 28) {
      selectedServo = n;
      Serial.println();
      Serial.printf("[OK] Selected servo %d\n", selectedServo);
      webLogf("[OK] Selected servo %d", selectedServo);
      printServo(selectedServo);
      setClockServoRaw(selectedServo, currentPos[selectedServo]);
      printPrompt();
    } else {
      Serial.println();
      Serial.println("[ERROR] Servo number out of range");
      Serial.println("        Valid range: 0..27");
      printPrompt();
    }
    return true;
  }

  if (token == "+") {
    moveSelected(10);
    Serial.println();
    Serial.printf("[OK] Servo %d moved +10\n", selectedServo);
    webLogf("[OK] Servo %d moved +10", selectedServo);
    printPrompt();
    return true;
  }

  if (token == "-") {
    moveSelected(-10);
    Serial.println();
    Serial.printf("[OK] Servo %d moved -10\n", selectedServo);
    webLogf("[OK] Servo %d moved -10", selectedServo);
    printPrompt();
    return true;
  }

  if (token == "++") {
    moveSelected(2);
    Serial.println();
    Serial.printf("[OK] Servo %d moved +2\n", selectedServo);
    webLogf("[OK] Servo %d moved +2", selectedServo);
    printPrompt();
    return true;
  }

  if (token == "--") {
    moveSelected(-2);
    Serial.println();
    Serial.printf("[OK] Servo %d moved -2\n", selectedServo);
    webLogf("[OK] Servo %d moved -2", selectedServo);
    printPrompt();
    return true;
  }

  if (token == "g") {
    if (!hasNextToken) {
      Serial.println();
      Serial.println("[ERROR] Missing logical value");
      Serial.println("        Use: g <value>");
      printPrompt();
      return true;
    }

    if (!parseIntStrict(nextToken, n)) {
      Serial.println();
      Serial.println("[ERROR] Invalid logical value");
      Serial.println("        Use: g <value>");
      consumedNextToken = true;
      printPrompt();
      return true;
    }

    consumedNextToken = true;
    currentPos[selectedServo] = constrainPulse(n);
    setClockServoRaw(selectedServo, currentPos[selectedServo]);

    Serial.println();
    Serial.printf("[OK] Servo %d logical target set to %u\n", selectedServo, currentPos[selectedServo]);
    webLogf("[OK] Servo %d -> %u", selectedServo, currentPos[selectedServo]);
    printPrompt();
    return true;
  }

  if (token == "a") {
    if (!hasNextToken) {
      Serial.println();
      Serial.println("[ERROR] Missing PWM value");
      Serial.println("        Use: a <value>");
      printPrompt();
      return true;
    }

    if (!parseIntStrict(nextToken, n)) {
      Serial.println();
      Serial.println("[ERROR] Invalid PWM value");
      Serial.println("        Use: a <value>");
      consumedNextToken = true;
      printPrompt();
      return true;
    }

    consumedNextToken = true;
    uint16_t constrained = constrainPulse(n);
    setClockServoActual(selectedServo, constrained);

    Serial.println();
    Serial.printf("[OK] Servo %d actual PWM set to %u\n", selectedServo, constrained);
    webLogf("[OK] Servo %d actual PWM -> %u", selectedServo, constrained);
    printPrompt();
    return true;
  }

  if (token == "i") {
    cal[selectedServo].inverted = !cal[selectedServo].inverted;
    setClockServoRaw(selectedServo, currentPos[selectedServo]);

    Serial.println();
    Serial.printf("[OK] Servo %d invert toggled\n", selectedServo);
    webLogf("[OK] Servo %d inverted=%s", selectedServo, cal[selectedServo].inverted ? "true" : "false");
    printServo(selectedServo);
    printPrompt();
    return true;
  }

  if (token == "step") {
    if (!hasNextToken) {
      Serial.println();
      Serial.println("[ERROR] Missing step value");
      Serial.println("        Use: step <n>");
      printPrompt();
      return true;
    }

    if (!parseIntStrict(nextToken, n)) {
      Serial.println();
      Serial.println("[ERROR] Invalid step value");
      Serial.println("        Use: step <n>");
      consumedNextToken = true;
      printPrompt();
      return true;
    }

    consumedNextToken = true;
    if (n < 1) n = 1;
    servoStepSize = (uint16_t)n;

    Serial.println();
    Serial.printf("[OK] Servo step size = %u\n", servoStepSize);
    printPrompt();
    return true;
  }

  if (token == "delay") {
    if (!hasNextToken) {
      Serial.println();
      Serial.println("[ERROR] Missing delay value");
      Serial.println("        Use: delay <n>");
      printPrompt();
      return true;
    }

    if (!parseIntStrict(nextToken, n)) {
      Serial.println();
      Serial.println("[ERROR] Invalid delay value");
      Serial.println("        Use: delay <n>");
      consumedNextToken = true;
      printPrompt();
      return true;
    }

    consumedNextToken = true;
    if (n < 0) n = 0;
    servoStepDelayMs = (uint16_t)n;

    Serial.println();
    Serial.printf("[OK] Servo step delay = %u ms\n", servoStepDelayMs);
    printPrompt();
    return true;
  }

  if (token == "smooth") {
    Serial.println();
    Serial.printf("[OK] Smooth settings -> step=%u delay=%u ms\n", servoStepSize, servoStepDelayMs);
    printPrompt();
    return true;
  }

  if (token == "o") {
    cal[selectedServo].open = currentPos[selectedServo];
    Serial.println();
    Serial.printf("[OK] Saved OPEN for servo %d\n", selectedServo);
    webLogf("[OK] Saved OPEN=%u for servo %d", cal[selectedServo].open, selectedServo);
    printPrompt();
    return true;
  }

  if (token == "c") {
    cal[selectedServo].closed = currentPos[selectedServo];
    Serial.println();
    Serial.printf("[OK] Saved CLOSED for servo %d\n", selectedServo);
    webLogf("[OK] Saved CLOSED=%u for servo %d", cal[selectedServo].closed, selectedServo);
    printPrompt();
    return true;
  }

  if (token == "to") {
    moveToStored(selectedServo, 'o');
    Serial.println();
    Serial.printf("[OK] Moved servo %d to OPEN\n", selectedServo);
    webLogf("[OK] Servo %d -> OPEN", selectedServo);
    printPrompt();
    return true;
  }

  if (token == "tc") {
    moveToStored(selectedServo, 'c');
    Serial.println();
    Serial.printf("[OK] Moved servo %d to CLOSED\n", selectedServo);
    webLogf("[OK] Servo %d -> CLOSED", selectedServo);
    printPrompt();
    return true;
  }

  if (token == "p") {
    Serial.println();
    printServo(selectedServo);
    printPrompt();
    return true;
  }

  if (token == "pa") {
    Serial.println();
    printAll();
    printPrompt();
    return true;
  }

  if (token == "w") {
    saveCalibration();
    printOk("Calibration saved");
    return true;
  }

  if (token == "l") {
    loadCalibration();
    printOk("Calibration loaded");
    return true;
  }

  return false;
}

void handleServoCommand(String cmd) {
  cmd.trim();

  int index = 0;
  bool handledAnything = false;

  while (true) {
    String token;
    if (!nextWord(cmd, index, token)) break;

    String nextToken;
    int lookaheadIndex = index;
    bool hasNextToken = nextWord(cmd, lookaheadIndex, nextToken);

    bool consumedNextToken = false;
    bool shouldExitMode = false;

    bool handled = handleServoToken(token, nextToken, hasNextToken, consumedNextToken, shouldExitMode);
    if (!handled) {
      printUnknownCommand("servo");
      return;
    }

    handledAnything = true;

    if (consumedNextToken) {
      String skipToken;
      nextWord(cmd, index, skipToken);
    }

    if (shouldExitMode) {
      return;
    }
  }

  if (!handledAnything) {
    printUnknownCommand("servo");
  }
}

void handleSmoothCommand(String cmd) {
  cmd.trim();

  if (cmd == "h") {
    showCurrentMenu();
    webLogHelp(MODE_SMOOTH);
  }
  else if (cmd == "m") {
    setMode(MODE_MAIN);
  }
  else if (cmd == "show") {
    Serial.println();
    Serial.printf("[OK] Global smooth step size = %u\n", servoStepSize);
    Serial.printf("[OK] Global smooth step delay = %u ms\n", servoStepDelayMs);
    webLogf("[OK] step=%u  delay=%u ms", servoStepSize, servoStepDelayMs);
    printPrompt();
  }
  else {
    int v;

    if (parseCommandIntArg(cmd, "step ", v)) {
      if (v < 1) v = 1;
      servoStepSize = (uint16_t)v;

      Serial.println();
      Serial.printf("[OK] Global smooth step size set to %u\n", servoStepSize);
      printPrompt();
    }
    else if (parseCommandIntArg(cmd, "delay ", v)) {
      if (v < 0) v = 0;
      servoStepDelayMs = (uint16_t)v;

      Serial.println();
      Serial.printf("[OK] Global smooth step delay set to %u ms\n", servoStepDelayMs);
      printPrompt();
    }
    else if (parseCommandIntArg(cmd, "test ", v)) {
      uint16_t target = constrainPulse(v);
      moveServoSmooth(selectedServo, target);

      Serial.println();
      Serial.printf("[OK] Testing servo %d to target %u\n", selectedServo, target);
      printPrompt();
    }
    else if (cmd.startsWith("powerhold ")) {
      int v = cmd.substring(10).toInt();
      if (v < 100) v = 100;
      servoPowerHoldMs = (unsigned long)v;
      Serial.printf("Servo power hold = %lu ms\n", servoPowerHoldMs);
    }


    else {
      printUnknownCommand("smooth");
    }
  }
}

void handleRTCCommand(String cmd) {
  cmd.trim();

  if (cmd == "h") {
    showCurrentMenu();
    webLogHelp(MODE_RTC);
  }
  else if (cmd == "m") {
    setMode(MODE_MAIN);
  }
  else if (cmd == "status") {
    Serial.println();
    printRTCNow();
    printPrompt();
  }
  else if (cmd == "setfixed") {
    setRTCFromCompileTime();
    printOk("RTC set from compile/example time");
  }
  else if (cmd == "syncntp") {
    syncRtcFromNtp();
    printOk("RTC sync requested from NTP");
  }
  else if (cmd == "watch") {
    Serial.println();
    for (int i = 0; i < 10; i++) {
      printRTCNow();
      delay(1000);
    }
    printPrompt();
  }
  else {
    printUnknownCommand("RTC");
  }
}

void handleMotionCommand(String cmd) {
  cmd.trim();

  if (cmd == "h") {
    showCurrentMenu();
    webLogHelp(MODE_MOTION);
  }
  else if (cmd == "m") {
    setMode(MODE_MAIN);
  }
  else if (cmd == "read") {
    int pirVal = digitalRead(PIR_PIN);
    Serial.println();
    Serial.printf("[OK] PIR state = %d\n", pirVal);
    webLogf("[OK] PIR = %d", pirVal);
    printPrompt();
  }
  else if (cmd == "watch") {
    Serial.println();
    for (unsigned long start = millis(); millis() - start < 10000;) {
      Serial.printf("PIR = %d\n", digitalRead(PIR_PIN));
      delay(500);
    }
    printPrompt();
  }
  else {
    printUnknownCommand("motion");
  }
}

void handleWiFiCommand(String cmd) {
  cmd.trim();

  if (cmd == "h") {
    showCurrentMenu();
    webLogHelp(MODE_WIFI);
  }
  else if (cmd == "m") {
    setMode(MODE_MAIN);
  }
  else if (cmd == "status") {
    Serial.println();
    printWiFiStatus();
    if (WiFi.status() == WL_CONNECTED) {
      webLogf("[OK] WiFi: %s  IP: %s  RSSI: %d dBm",
              WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
      webLog("[OK] WiFi not connected");
    }
    printPrompt();
  }
  else if (cmd == "scan") {
    Serial.println();
    int n = WiFi.scanNetworks();

    if (n < 0) {
      Serial.println("[ERROR] WiFi scan failed");
    } else if (n == 0) {
      Serial.println("[OK] No WiFi networks found");
    } else {
      Serial.printf("[OK] Found %d WiFi network(s)\n", n);
      Serial.println();
      for (int i = 0; i < n; i++) {
        Serial.printf("%2d. %-24s (%d dBm)\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
      }
    }

    printPrompt();
  }
  else if (cmd.startsWith("ssid ")) {
    wifiSSID = cmd.substring(5);
    wifiSSID.trim();

    Serial.println();
    if (wifiSSID.length() == 0) {
      Serial.println("[ERROR] SSID cannot be empty");
    } else {
      Serial.printf("[OK] WiFi SSID set to: %s\n", wifiSSID.c_str());
    }

    printPrompt();
  }
  else if (cmd.startsWith("pass ")) {
    wifiPASS = cmd.substring(5);
    Serial.println();
    Serial.println("[OK] WiFi password updated");
    printPrompt();
  }
  else if (cmd == "connect") {
    Serial.println();
    connectWiFi();
    printPrompt();
  }
  else if (cmd == "disconnect") {
    WiFi.disconnect();
    printOk("WiFi disconnected");
  }
  else {
    printUnknownCommand("WiFi");
  }
}

void handleMainCommand(String cmd) {
  cmd.trim();

  if (cmd == "h") {
    showCurrentMenu();
    webLogHelp(MODE_MAIN);
    return;
  }

  if (cmd == "1") {
    setMode(MODE_SERVO_CAL);
  }
  else if (cmd == "2") {
    setMode(MODE_SMOOTH);
  }
  else if (cmd == "3") {
    setMode(MODE_RTC);
  }
  else if (cmd == "4") {
    setMode(MODE_MOTION);
  }
  else if (cmd == "5") {
    setMode(MODE_WIFI);
  }
  else if (cmd == "6") {
    setMode(MODE_RUN);
  }
  else if (cmd == "7") {
    Serial.println();
    printAll();
    printPrompt();
  }
  else {
    printUnknownCommand("main");
  }
}

void handleRunCommand(String cmd) {
  cmd.trim();

  if (cmd == "h") {
    showCurrentMenu();
    webLogHelp(MODE_RUN);
  }
  else if (cmd == "m") {
    setMode(MODE_MAIN);
  }
  else if (cmd == "start") {
    startClock();
    printOk("Clock started");
  }
  else if (cmd == "stop") {
    stopClock();
    printOk("Clock stopped");
  }
  else if (cmd == "time") {
    Serial.println();
    printRTCNow();
    printPrompt();
  }
  else {
    printUnknownCommand("run");
  }
}

void routeCommand(String cmd) {
  switch (currentMode) {
    case MODE_MAIN:      handleMainCommand(cmd); break;
    case MODE_SERVO_CAL: handleServoCommand(cmd); break;
    case MODE_SMOOTH:    handleSmoothCommand(cmd); break;
    case MODE_RTC:       handleRTCCommand(cmd); break;
    case MODE_MOTION:    handleMotionCommand(cmd); break;
    case MODE_WIFI:      handleWiFiCommand(cmd); break;
    case MODE_RUN:       handleRunCommand(cmd); break;

    default:
      Serial.println();
      Serial.println("[ERROR] Invalid app mode, returning to main menu");
      setMode(MODE_MAIN);
      break;
  }
}