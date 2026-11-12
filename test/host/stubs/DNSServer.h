/* DNSServer.h — host stub for the replay harness (see stubs/Arduino.h).
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_DNSSERVER_H
#define HOST_STUB_DNSSERVER_H
#include "Arduino.h"

class DNSServer {
public:
  void start() {}
  void stop() {}
  void processNextRequest() {}
  void setErrorReplyCode(uint8_t) {}
  void setTTL(uint32_t) {}
  void setPort(uint16_t) {}
};
#endif
