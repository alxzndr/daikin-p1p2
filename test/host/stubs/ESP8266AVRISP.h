/* ESP8266AVRISP.h — host stub for the replay harness (see stubs/Arduino.h).
 * Flashing the ATmega is never possible from the harness: begin()/serve() do
 * nothing and the state stays IDLE. Part of the P1P2MQTT fork, CC BY-NC-ND 4.0
 * with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_ESP8266AVRISP_H
#define HOST_STUB_ESP8266AVRISP_H
#include "Arduino.h"

enum AVRISPState_t { AVRISP_STATE_IDLE = 0, AVRISP_STATE_PENDING, AVRISP_STATE_ACTIVE };
enum AVRISPErr_t { AVRISP_OK = 0, AVRISP_FAIL = 1, AVRISP_ERR_MALLOC = 2, AVRISP_ERR_BUSY = 3 };

class ESP8266AVRISP {
public:
  ESP8266AVRISP(uint16_t port, uint8_t resetPin, double spiSpeed = 0) { (void)port; (void)resetPin; (void)spiSpeed; }
  AVRISPState_t update() { return AVRISP_STATE_IDLE; }
  void serve() {}
  AVRISPErr_t begin() { return AVRISP_OK; }
  void end() {}
  bool checkSignature() { return false; }
  bool chipErase() { return false; }
  bool readFlash(uint32_t, uint8_t *, size_t) { return false; }
};
#endif
