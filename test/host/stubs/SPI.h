/* SPI.h — host stub for the replay harness (see stubs/Arduino.h).
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_SPI_H
#define HOST_STUB_SPI_H
#include "Arduino.h"

#define MSBFIRST 1
#define LSBFIRST 0
#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3
#define SPI_CLOCK_DIV2 2
#define SPI_CLOCK_DIV4 4
#define SPI_CLOCK_DIV8 8
#define SPI_CLOCK_DIV16 16
#define SPI_CLOCK_DIV32 32
#define SPI_CLOCK_DIV64 64
#define SPI_CLOCK_DIV128 128

class SPIClassStub {
public:
  void begin() {}
  void end() {}
  void setClockDivider(uint32_t) {}
  void setBitOrder(uint8_t) {}
  void setDataMode(uint8_t) {}
  uint8_t transfer(uint8_t v) { return v; }
};
extern SPIClassStub SPI;
#endif
