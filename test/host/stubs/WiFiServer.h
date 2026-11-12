/* WiFiServer.h — host stub for the replay harness (see stubs/Arduino.h).
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_WIFISERVER_H
#define HOST_STUB_WIFISERVER_H
#include "Arduino.h"
#include "WiFiClient.h"

class WiFiServer {
public:
  using Client = WiFiClient;
  WiFiServer(uint16_t port = 23) : _port(port) {}
  void begin() {}
  void close() {}
  WiFiClient available() { return WiFiClient(); }
  uint16_t port() const { return _port; }

protected:
  uint16_t _port;
};

namespace BearSSL {
class WiFiServerSecure : public WiFiServer {
public:
  using Server = WiFiServer;
  using Client = WiFiClient;
  WiFiServerSecure(uint16_t port = 443) : WiFiServer(port) {}
};
};  // namespace BearSSL
#endif
