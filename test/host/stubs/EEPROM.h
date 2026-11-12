/* EEPROM.h — host stub for the replay harness (see stubs/Arduino.h).
 * The firmware uses ESP.flashRead/flashWrite directly; this header exists only
 * because the sketch includes it. Part of the P1P2MQTT fork, CC BY-NC-ND 4.0
 * with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_EEPROM_H
#define HOST_STUB_EEPROM_H
#include "Arduino.h"

class EepromStub {
public:
  bool begin(size_t) { return true; }
  void end() {}
  void commit() {}
  uint8_t read(int) { return 0xFF; }
  void write(int, uint8_t) {}
  template <typename T> T &get(int, T &v) { return v; }
  template <typename T> const T &put(int, const T &v) { return v; }
  size_t length() { return 2048; }
};
extern EepromStub EEPROM;
#endif
