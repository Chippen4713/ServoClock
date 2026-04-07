#include "web_interface.h"
#include "menu_control.h"
#include <stdarg.h>

static const int TCP_PORT = 23;

static WiFiServer tcpServer(TCP_PORT);
static WiFiClient tcpClient;
static char      tcpLineBuf[128];
static int       tcpLineLen = 0;
static bool      serverStarted = false;

// Transmit buffer — all webLog calls accumulate here, flushed once per command
static char      txBuf[2048];
static int       txLen = 0;

static void webFlush() {
  if (txLen > 0 && tcpClient && tcpClient.connected()) {
    tcpClient.write((const uint8_t*)txBuf, txLen);
    txLen = 0;
  }
}

void webLog(const char* line) {
  if (!tcpClient || !tcpClient.connected()) return;
  int lineLen = strlen(line);
  int needed  = lineLen + 2; // \r\n
  if (txLen + needed >= (int)sizeof(txBuf)) webFlush();
  memcpy(txBuf + txLen, line, lineLen);
  txLen += lineLen;
  txBuf[txLen++] = '\r';
  txBuf[txLen++] = '\n';
}

void webLog(const String& line) {
  webLog(line.c_str());
}

void webLogf(const char* fmt, ...) {
  char buf[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  webLog(buf);
}

void setupWebInterface() {
  if (serverStarted) return;
  serverStarted = true;
  tcpServer.begin();
  tcpServer.setNoDelay(true);
  Serial.printf("TCP terminal on port %d\n", TCP_PORT);
}

void updateWebInterface() {
  if (!serverStarted) return;

  // Accept incoming connection (one client at a time)
  if (tcpServer.hasClient()) {
    if (tcpClient && tcpClient.connected()) tcpClient.stop();
    tcpClient = tcpServer.available();
    txLen = 0;
    tcpClient.print("=== Servo Clock Terminal ===\r\n");
    tcpClient.print("Type 'h' for help.\r\n");
    tcpClient.print("\r\n");
    tcpLineLen = 0;
  }

  if (!tcpClient || !tcpClient.connected()) return;

  // Read incoming characters into line buffer
  while (tcpClient.available()) {
    char c = tcpClient.read();
    if (c == '\r') continue;
    if (c == '\n') {
      tcpLineBuf[tcpLineLen] = '\0';
      tcpLineLen = 0;
      String cmd = String(tcpLineBuf);
      cmd.trim();
      if (cmd.length() > 0) {
        tcpClient.print("> ");
        tcpClient.print(cmd);
        tcpClient.print("\r\n");
        routeCommand(cmd);
        webFlush();  // send all buffered output in one TCP write
      }
    } else if (tcpLineLen < (int)sizeof(tcpLineBuf) - 1) {
      tcpLineBuf[tcpLineLen++] = c;
    }
  }
}
