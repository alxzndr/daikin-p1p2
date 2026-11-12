/* flash_hal.h — host stub for the replay harness (see stubs/Arduino.h).
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_FLASH_HAL_H
#define HOST_STUB_FLASH_HAL_H
#include <cstdint>
#include "Arduino.h"   // SPI_FLASH_SEC_SIZE

#define SPI_FLASH_SEC_SIZE_LOCAL SPI_FLASH_SEC_SIZE
typedef struct {
  uint32_t sr;
  uint32_t freerun;
} ring_buf_t;
// The real core exposes the filesystem region through these linker symbols and a
// teardown function; the update server references them when a "filesystem" upload
// starts. Nothing is uploaded in a replay, so they only have to exist.
extern "C" {
extern char _FS_start;
extern char _FS_end;
void close_all_fs();
}

inline int flashSectorErase(uint32_t) { return 0; }
inline int flashWrite(uint32_t, uint32_t *, uint32_t) { return 0; }
#endif
