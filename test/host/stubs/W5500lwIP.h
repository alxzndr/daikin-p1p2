/* W5500lwIP.h — host stub for the replay harness (see stubs/Arduino.h).
 * initEthernet() succeeds immediately so that setup() skips the WiFiManager
 * config portal (which would otherwise block waiting for a browser). The link
 * state is configurable so both branches can be replayed.
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_W5500LWIP_H
#define HOST_STUB_W5500LWIP_H
#include "Arduino.h"

class Wiznet5500lwIP {
public:
  bool hwAvailable = true;
  bool linkUp = true;
  explicit Wiznet5500lwIP(uint8_t, const uint8_t * = nullptr) {}
  bool begin() { return hwAvailable; }
  bool connected() { return linkUp; }
  void setDefault() {}
  void config(IPAddress, IPAddress, IPAddress, IPAddress) {}
  IPAddress localIP() { return IPAddress(10, 0, 1, 99); }
  IPAddress subnetMask() { return IPAddress(255, 255, 255, 0); }
  IPAddress gatewayIP() { return IPAddress(10, 0, 1, 1); }
  void setHostname(const char *) {}
  uint8_t *macAddress() { return _mac; }
  int8_t status() { return 3; }

private:
  uint8_t _mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
};
#endif
