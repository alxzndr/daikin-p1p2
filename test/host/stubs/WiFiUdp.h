/* WiFiUdp.h — host stub for the replay harness (see stubs/Arduino.h).
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_WIFIUDP_H
#define HOST_STUB_WIFIUDP_H
#include "Arduino.h"

class WiFiUDP {
public:
  static void stopAll() {}
  bool begin(uint16_t = 0) { return true; }
  bool begin(IPAddress, uint16_t) { return true; }
  void stop() {}
  int parsePacket() { return 0; }
  int available() { return 0; }
  int read() { return -1; }
  void flush() {}
};
#endif
