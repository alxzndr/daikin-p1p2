/* Arduino.h — host stub for the offline replay harness
 *
 * Part of the P1P2MQTT fork (https://github.com/alxzndr/daikin-p1p2). License:
 * CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 *
 * This file is NOT part of the firmware. It replaces the ESP8266 Arduino core for
 * a host (macOS/Linux) build of the decoder, so that the unmodified firmware
 * sources can be compiled and replayed offline. Nothing here is compiled into the
 * ESP firmware; the ESP build is unaffected.
 *
 * Design rules:
 *   - Only what the firmware actually touches. No behaviour that could make a
 *     replay differ from the bridge, except where documented (time, memory, EEPROM).
 *   - Time comes from the virtual clock in replay.cpp, never from the wall clock.
 */

#ifndef HOST_STUB_ARDUINO_H
#define HOST_STUB_ARDUINO_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <cstdarg>
#include <unistd.h>
// All standard library headers the harness itself needs are included HERE, before the
// Arduino min/max/abs macros below. libc++/libstdc++ break if those macros exist when
// their headers are parsed - the same reason Arduino code must not include <map> after
// Arduino.h in a sketch.
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

typedef uint8_t byte;
typedef bool boolean;
typedef unsigned int word;
typedef const char *flash_string_t;

// Vendored ESP8266 core code (the web update server) uses these core macros.
#define _F(str) (String(F(str)))

// ---------------------------------------------------------------- PROGMEM ----
// The ESP keeps constant tables in flash; on the host everything is plain RAM.
#define PROGMEM
typedef const char PROGMEM PGM_P;
#define PSTR(s) (s)
#define FPSTR(s) (s)
#define F(s) (s)
#define pgm_read_byte(a) (*(const uint8_t *)(a))
#define pgm_read_word(a) (*(const uint16_t *)(a))
#define pgm_read_dword(a) (*(const uint32_t *)(a))
#define pgm_read_float(a) (*(const float *)(a))
#define pgm_read_ptr(a) (*(const void *const *)(a))
#define pgm_read_byte_near(a) pgm_read_byte(a)
#define pgm_read_word_near(a) pgm_read_word(a)
#define pgm_read_dword_near(a) pgm_read_dword(a)
#define pgm_read_byte_far(a) pgm_read_byte(a)
#define pgm_read_word_far(a) pgm_read_word(a)
#define pgm_read_dword_far(a) pgm_read_dword(a)

inline int snprintf_P(char *buf, size_t n, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
inline int snprintf_P(char *buf, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsnprintf(buf, n, fmt, ap);
  va_end(ap);
  return r;
}
inline int sprintf_P(char *buf, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
inline int sprintf_P(char *buf, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsnprintf(buf, 0x7fffffff, fmt, ap);
  va_end(ap);
  return r;
}
inline int vsnprintf_P(char *buf, size_t n, const char *fmt, va_list ap) {
  return vsnprintf(buf, n, fmt, ap);
}
inline int vsprintf_P(char *buf, const char *fmt, va_list ap) {
  return vsnprintf(buf, 0x7fffffff, fmt, ap);
}
inline int strcmp_P(const char *a, const char *b) { return strcmp(a, b); }
inline int strncmp_P(const char *a, const char *b, size_t n) { return strncmp(a, b, n); }
inline int strcasecmp_P(const char *a, const char *b) { return strcasecmp(a, b); }
inline size_t strlen_P(const char *a) { return strlen(a); }
inline char *strcpy_P(char *d, const char *s) { return strcpy(d, s); }
inline char *strncpy_P(char *d, const char *s, size_t n) { return strncpy(d, s, n); }
inline char *strcat_P(char *d, const char *s) { return strcat(d, s); }
inline char *strncat_P(char *d, const char *s, size_t n) { return strncat(d, s, n); }
inline char *strstr_P(const char *h, const char *n) { return (char *)strstr(h, n); }
inline char *strchr_P(const char *s, int c) { return (char *)strchr(s, c); }

// ---------------------------------------------------------------- String ----
// Minimal Arduino String; enough for the telnet callbacks and the update server.
class String {
public:
  String() {}
  String(const char *s) : _s(s ? s : "") {}
  String(const std::string &s) : _s(s) {}
  String(char c) : _s(1, c) {}
  String(int v) : _s(std::to_string(v)) {}
  String(unsigned v) : _s(std::to_string(v)) {}
  String(long v) : _s(std::to_string(v)) {}
  String(unsigned long v) : _s(std::to_string(v)) {}
  String(int v, int base) : _s(toStringStd(v, base)) {}
  String(float v, unsigned decimals = 2) { char b[32]; snprintf(b, sizeof(b), "%.*f", (int)decimals, v); _s = b; }
  String(double v, unsigned decimals = 2) { char b[32]; snprintf(b, sizeof(b), "%.*f", (int)decimals, v); _s = b; }

  static std::string toStringStd(long v, int base) {
    if (base == 16) { char b[24]; snprintf(b, sizeof(b), "%lx", (unsigned long)v); return b; }
    if (base == 2) { std::string r; unsigned long u = (unsigned long)v; do { r.insert(r.begin(), char('0' + (u & 1))); u >>= 1; } while (u); return r; }
    return std::to_string(v);
  }

  unsigned int length() const { return (unsigned int)_s.length(); }
  const char *c_str() const { return _s.c_str(); }
  bool isEmpty() const { return _s.empty(); }
  void clear() { _s.clear(); }
  char charAt(unsigned i) const { return _s[i]; }
  char operator[](unsigned i) const { return _s[i]; }
  void reserve(unsigned n) { _s.reserve(n); }
  void trim() {
    size_t a = _s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) { _s.clear(); return; }
    size_t b = _s.find_last_not_of(" \t\r\n");
    _s = _s.substr(a, b - a + 1);
  }
  int indexOf(const char *t, unsigned from = 0) const { size_t p = _s.find(t, from); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(char t, unsigned from = 0) const { size_t p = _s.find(t, from); return p == std::string::npos ? -1 : (int)p; }
  int lastIndexOf(char t) const { size_t p = _s.rfind(t); return p == std::string::npos ? -1 : (int)p; }
  String substring(unsigned from, unsigned to = 0xffffffffu) const { return String(_s.substr(from, to - from).c_str()); }
  long toInt() const { return atol(_s.c_str()); }
  float toFloat() const { return (float)atof(_s.c_str()); }
  void toCharArray(char *dst, unsigned n) const { strncpy(dst, _s.c_str(), n); dst[n - 1] = '\0'; }

  int compareTo(const String &o) const { return _s.compare(o._s); }
  bool equals(const String &o) const { return _s == o._s; }
  bool equalsIgnoreCase(const String &o) const { return strcasecmp(_s.c_str(), o._s.c_str()) == 0; }
  bool startsWith(const String &p) const { return _s.rfind(p._s, 0) == 0; }
  bool endsWith(const String &p) const { return p._s.empty() || (_s.size() >= p._s.size() && _s.compare(_s.size() - p._s.size(), p._s.size(), p._s) == 0); }

  String &operator=(const char *s) { _s = s ? s : ""; return *this; }
  String &operator=(const String &s) { _s = s._s; return *this; }
  String &operator+=(const String &s) { _s += s._s; return *this; }
  String &operator+=(const char *s) { if (s) _s += s; return *this; }
  String &operator+=(char c) { _s += c; return *this; }
  String &operator+=(int v) { _s += std::to_string(v); return *this; }
  String &operator+=(unsigned long v) { _s += std::to_string(v); return *this; }
  String &operator+=(long v) { _s += std::to_string(v); return *this; }
  String &operator+=(float v) { char b[32]; snprintf(b, sizeof(b), "%f", v); _s += b; return *this; }
  String operator+(const String &o) const { return String((_s + o._s).c_str()); }

  bool operator==(const String &o) const { return _s == o._s; }
  bool operator==(const char *o) const { return _s == (o ? o : ""); }
  bool operator!=(const String &o) const { return _s != o._s; }
  bool operator!=(const char *o) const { return !(*this == o); }
  bool operator>(const String &o) const { return _s > o._s; }
  bool operator<(const String &o) const { return _s < o._s; }

private:
  std::string _s;
};
inline String operator+(const char *a, const String &b) { return String(a) + b; }
inline bool operator==(const char *a, const String &b) { return b == a; }

extern const String emptyString;
extern const String newlineString;

// -------------------------------------------------------------- time base ----
// Virtual clock, driven by the capture (see replay.cpp). Never the wall clock.
uint32_t millis();
uint32_t micros();
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);
void yield();
void optimistic_yield(uint32_t) { yield(); }

// ------------------------------------------------------------------ Print ----
class Print {
public:
  virtual ~Print() {}
  virtual size_t write(uint8_t c) = 0;
  virtual size_t write(const uint8_t *buf, size_t n) {
    for (size_t i = 0; i < n; i++) write(buf[i]);
    return n;
  }
  size_t write(const char *s) { return s ? write((const uint8_t *)s, strlen(s)) : 0; }
  size_t write(const char *s, size_t n) { return write((const uint8_t *)s, n); }
  size_t print(const char *s) { return write(s); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(int v) { char b[16]; snprintf(b, sizeof(b), "%d", v); return write(b); }
  size_t print(unsigned v) { char b[16]; snprintf(b, sizeof(b), "%u", v); return write(b); }
  size_t print(long v) { char b[24]; snprintf(b, sizeof(b), "%ld", v); return write(b); }
  size_t print(unsigned long v) { char b[24]; snprintf(b, sizeof(b), "%lu", v); return write(b); }
  size_t print(double v, int dec = 2) { char b[32]; snprintf(b, sizeof(b), "%.*f", dec, v); return write(b); }
  size_t print(const String &s) { return write(s.c_str()); }
  size_t println() { return write((uint8_t)'\n'); }
  size_t println(const char *s) { size_t n = write(s); return n + println(); }
  size_t println(char c) { size_t n = write((uint8_t)c); return n + println(); }
  size_t println(int v) { size_t n = print(v); return n + println(); }
  size_t println(unsigned v) { size_t n = print(v); return n + println(); }
  size_t println(long v) { size_t n = print(v); return n + println(); }
  size_t println(unsigned long v) { size_t n = print(v); return n + println(); }
  size_t println(double v, int dec = 2) { size_t n = print(v, dec); return n + println(); }
  size_t println(const String &s) { size_t n = write(s.c_str()); return n + println(); }
  size_t printf(const char *fmt, ...) __attribute__((format(printf, 2, 3))) {
    char b[512];
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(b, sizeof(b), fmt, ap);
    va_end(ap);
    if (r < 0) return 0;
    return write(b, (size_t)(r < (int)sizeof(b) ? r : (int)sizeof(b) - 1));
  }
  void setDebugOutput(bool) {}
  virtual void flush() {}
};

// ----------------------------------------------------------------- Stream ----
class Stream : public Print {
public:
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;
  void setTimeout(unsigned long) {}
  String readString() {
    String s;
    int c;
    while ((c = read()) >= 0) s += (char)c;
    return s;
  }
  size_t readBytes(char *buf, size_t n) {
    size_t i = 0;
    while (i < n) { int c = read(); if (c < 0) break; buf[i++] = (char)c; }
    return i;
  }
  int readStringBytes(uint8_t *, size_t n) { return (int)n; }
};

// -------------------------------------------------------------- IPAddress ----
class IPAddress {
public:
  uint8_t _b[4] = {0, 0, 0, 0};
  IPAddress() {}
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) { _b[0] = a; _b[1] = b; _b[2] = c; _b[3] = d; }
  IPAddress(uint32_t v) { for (int i = 0; i < 4; i++) _b[i] = (uint8_t)(v >> (8 * i)); }
  bool fromString(const char *s) {
    unsigned a, b, c, d;
    if (!s || sscanf(s, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return false;
    _b[0] = (uint8_t)a; _b[1] = (uint8_t)b; _b[2] = (uint8_t)c; _b[3] = (uint8_t)d;
    return true;
  }
  operator uint32_t() const { return (uint32_t)_b[0] | ((uint32_t)_b[1] << 8) | ((uint32_t)_b[2] << 16) | ((uint32_t)_b[3] << 24); }
  bool operator==(const IPAddress &o) const { return memcmp(_b, o._b, 4) == 0; }
  bool operator!=(const IPAddress &o) const { return !(*this == o); }
  uint8_t operator[](int i) const { return _b[i]; }
  String toString() const { char b[16]; snprintf(b, sizeof(b), "%u.%u.%u.%u", _b[0], _b[1], _b[2], _b[3]); return String(b); }
};

// --------------------------------------------------------------- GPIO stubs ----
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t) { return LOW; }
inline void interrupts() {}
inline void noInterrupts() {}

// --------------------------------------------------------------- ESP object ----
// EEPROM lives in a host file so that a replay can start from a blank flash
// (all 0xFF -> firmware factory defaults) or from a captured image.
#define SPI_FLASH_SEC_SIZE 4096
// The firmware derives EEPROM_sector/EEPROM_size from the ESP linker symbol
// _EEPROM_start; that symbol is provided by replay.cpp.

class HostEsp {
public:
  bool restartRequested = false;
  char restartReason[32] = "host";

  // The host image is the raw EEPROM sector, so the sector address (derived from
  // the ESP linker symbol) is not used as an offset.
  bool flashRead(uint32_t, uint32_t *dst, uint32_t n) {
    if (!flashFile()) { memset(dst, 0xFF, n); return false; }
    FILE *f = fopen(flashFile(), "rb");
    if (!f) { memset(dst, 0xFF, n); return false; }
    size_t got = fread(dst, 1, n, f);
    fclose(f);
    if (got < n) memset((uint8_t *)dst + got, 0xFF, n - got);
    return true;
  }
  bool flashWrite(uint32_t, uint32_t *src, uint32_t n) {
    if (!flashFile()) return false;
    FILE *f = fopen(flashFile(), "r+b");
    if (!f) f = fopen(flashFile(), "w+b");
    if (!f) return false;
    fseek(f, 0, SEEK_SET);
    size_t put = fwrite(src, 1, n, f);
    fclose(f);
    return put == n;
  }
  bool flashEraseSector(uint32_t) { return true; }
  void eraseConfig() {}
  uint32_t getMaxFreeBlockSize() { return 60000; }   // never trigger the low-memory skip
  uint32_t getFreeHeap() { return 60000; }
  uint32_t getFreeSketchSpace() { return 1024 * 1024; }
  uint32_t getChipId() { return 0x00BEEF; }
  const char *getResetReason() { return "host"; }
  void restart() { restartRequested = true; }
  void reset() { restartRequested = true; }
  void wdtFeed() {}
  void wdtEnable(uint32_t) {}
  bool rtcUserMemoryRead(uint32_t addr, uint32_t *data, uint32_t size) {
    if (addr + size > sizeof(rtcMem)) return false;
    memcpy(data, ((uint8_t *)rtcMem) + addr, size);
    return true;
  }
  bool rtcUserMemoryWrite(uint32_t addr, uint32_t *data, uint32_t size) {
    if (addr + size > sizeof(rtcMem)) return false;
    memcpy(((uint8_t *)rtcMem) + addr, data, size);
    return true;
  }
  uint32_t getCycleCount() { return micros(); }

  // host-side configuration
  void setFlashFile(const char *path) { _flashFile = path ? path : ""; }

private:
  uint32_t rtcMem[128] = {0};
  const char *_flashFile = "";
  const char *flashFile() { return (_flashFile && _flashFile[0]) ? _flashFile : nullptr; }
};

extern HostEsp ESP;

// -------------------------------------------------------------- misc Arduino ----
// ------------------------------------------------------------- the ATmega link ----
// On the bridge, Serial is the 250 kbaud link to the ATmega. In the harness it is
// the input port for a capture: replay.cpp feeds the reconstructed ATmega lines in,
// the firmware reads them with its own unmodified parser.
class HostSerial : public Stream {
public:
  void begin(unsigned long) {}
  void begin(unsigned long, uint8_t) {}
  void end() {}
  void setRxBufferSize(size_t) {}
  void update() {}
  void setDebugOutput(bool) {}

  // host side, implemented in replay.cpp
  void hostFeed(const char *data, size_t n);
  void hostFeedLine(const char *line);
  void hostClearFeed();
  size_t hostFeedPending();

  int available() override;
  int read() override;
  int peek() override;
  size_t write(uint8_t c) override { (void)c; return 1; }  // ESP -> ATmega: dropped
  size_t write(const uint8_t *b, size_t n) override { (void)b; return n; }
  operator bool() { return true; }
};

extern HostSerial Serial;

template <typename T> T _cpp_min(T a, T b) { return a < b ? a : b; }
template <typename T> T _cpp_max(T a, T b) { return a > b ? a : b; }
#define min(a, b) _cpp_min((a), (b))
#define max(a, b) _cpp_max((a), (b))
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define abs(x) ((x) > 0 ? (x) : -(x))
#define round(x) ((x) >= 0 ? (long)((x) + 0.5) : (long)((x)-0.5))
#define sq(x) ((x) * (x))
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))
#define bit(b) (1UL << (b))
#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))
#define word(h, l) ((uint16_t)(((uint16_t)(h) << 8) | (uint8_t)(l)))
#define map(x, in_min, in_max, out_min, out_max) ((long)(x) * ((out_max) - (out_min)) / ((in_max) - (in_min)))

long random(long howbig);
inline long randomRange(long howsmall, long howbig) { return howsmall + random(howbig - howsmall); }
#define randomSeed(x) ((void)(x))

// avr-libc helpers used by the firmware
inline double dtostrf(double v, unsigned int width, unsigned int decimals, char *buf) {
  snprintf(buf, width + 1, "%*.*f", (int)width, (int)decimals, v);
  return v;
}
char *itoa(int value, char *result, int base);
char *utoa(unsigned value, char *result, int base);
char *ltoa(long value, char *result, int base);
char *ultoa(unsigned long value, char *result, int base);
#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);
#endif

#endif  // HOST_STUB_ARDUINO_H
