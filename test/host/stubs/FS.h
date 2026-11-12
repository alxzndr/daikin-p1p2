/* FS.h — host stub for the replay harness (see stubs/Arduino.h).
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_FS_H
#define HOST_STUB_FS_H
#include "Arduino.h"

namespace fs {
class FS {
public:
  uint64_t totalBytes() { return 0; }
  uint64_t usedBytes() { return 0; }
  bool begin() { return false; }
  void end() {}
};
struct FSInfo {
  size_t totalBytes = 0;
  size_t usedBytes = 0;
  size_t blockSize = 0;
  size_t volumeSize = 0;
};
};  // namespace fs
extern fs::FS SPIFFS;
#endif
