/* WiFiClient.h — host stub for the replay harness (see stubs/Arduino.h).
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_WIFICLIENT_H
#define HOST_STUB_WIFICLIENT_H
#include "Arduino.h"

class WiFiClient : public Stream {
public:
  bool connect(IPAddress, uint16_t) { return false; }
  bool connect(const char *, uint16_t) { return false; }
  bool connected() { return false; }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void stop() {}
  void setNoDelay(bool = true) {}
  void setWriteDelay(int) {}
  size_t write(uint8_t c) override { (void)c; return 1; }
  operator bool() { return false; }
};
#endif
