/* ESP8266mDNS.h — host stub for the replay harness (see stubs/Arduino.h).
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_ESP8266MDNS_H
#define HOST_STUB_ESP8266MDNS_H
#include "Arduino.h"

class MDNSResponder {
public:
  bool begin(const char * = nullptr) { return true; }
  bool addService(const char *, const char *, uint16_t, const char * = nullptr) { return true; }
  void update() {}
  void end() {}
  String hostname() { return String("host"); }
};
extern MDNSResponder MDNS;
#endif
