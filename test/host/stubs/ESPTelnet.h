/* ESPTelnet.h — host stub for the replay harness (see stubs/Arduino.h).
 * No telnet connection can exist offline, so telnetConnected stays false and
 * telnet output is dropped (exactly as on the bridge without a client).
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_ESPTELNET_H
#define HOST_STUB_ESPTELNET_H
#include "Arduino.h"
#include <functional>

class ESPTelnet : public Print {
public:
  using THandlerFunction = std::function<void(String)>;

  bool begin() { return true; }
  bool begin(uint16_t port, bool useNW = true) { (void)port; (void)useNW; return true; }
  bool begin(IPAddress, uint16_t port = 23) { (void)port; return true; }
  void loop() {}
  void disconnect() {}
  void setHostname(const char *) {}

  void onConnect(THandlerFunction fn) { (void)fn; }
  void onDisconnect(THandlerFunction fn) { (void)fn; }
  void onReconnect(THandlerFunction fn) { (void)fn; }
  void onConnectionAttempt(THandlerFunction fn) { (void)fn; }
  void onInputReceived(THandlerFunction fn) { (void)fn; }

  String getIP() { return String("127.0.0.1"); }
  bool isConnected() { return false; }
  uint8_t getClientCount() { return 0; }

  size_t write(uint8_t c) override { (void)c; return 1; }
  size_t write(const uint8_t *b, size_t n) override { (void)b; return n; }
};
#endif
