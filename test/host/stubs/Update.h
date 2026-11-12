/* Update.h — host stub for the replay harness (see stubs/Arduino.h).
 * Firmware updates are never performed by the harness: every method is a no-op
 * that reports "no error", so the web-update page code compiles but does nothing.
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_UPDATE_H
#define HOST_STUB_UPDATE_H
#include "Arduino.h"

#define U_FLASH 0
#define U_SPIFFS 100
#define U_FS 101
#define UPDATE_ERROR_OK 0

class UpdateClass {
public:
  bool begin(size_t = 0, int = U_FLASH) { return true; }
  size_t write(const uint8_t *, size_t n) { return n; }
  bool end(bool = false) { return true; }
  bool hasError() { return false; }
  bool endError() { return false; }
  void printError(Print &p) { p.print("host: update is a no-op"); }
  const char *errorString() { return "host"; }
  void abort() {}
  uint32_t remainingSpace() { return 1024 * 1024; }
  int getProgress() { return 0; }
  int getSize() { return 0; }
  operator bool() { return true; }
};

extern UpdateClass Update;
#endif
