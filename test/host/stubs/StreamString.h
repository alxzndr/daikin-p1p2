/* StreamString.h — host stub for the replay harness (see stubs/Arduino.h).
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_STREAMSTRING_H
#define HOST_STUB_STREAMSTRING_H
#include "Arduino.h"

class StreamString : public Stream {
public:
  StreamString() {}
  void clear() { _s.clear(); }
  unsigned int length() const { return (unsigned int)_s.length(); }
  const char *c_str() const { return _s.c_str(); }
  operator String() const { return String(_s.c_str()); }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  size_t write(uint8_t c) override { _s += (char)c; return 1; }
  void flush() override {}

private:
  std::string _s;
};
#endif
