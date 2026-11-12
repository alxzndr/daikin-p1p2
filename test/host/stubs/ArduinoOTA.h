/* ArduinoOTA.h — host stub for the replay harness (see stubs/Arduino.h).
 * OTA is never served by the harness; handle() does nothing, so the callbacks
 * registered by setup() are never called. Part of the P1P2MQTT fork,
 * CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_ARDUINOOTA_H
#define HOST_STUB_ARDUINOOTA_H
#include "Arduino.h"
#include <functional>

typedef std::function<void(void)> THandlerFunction_OTA;
enum ota_error_t { OTA_AUTH_ERROR = 0, OTA_BEGIN_ERROR = 1, OTA_CONNECT_ERROR = 2, OTA_RECEIVE_ERROR = 3, OTA_END_ERROR = 4 };
typedef std::function<void(ota_error_t)> THandlerFunction_Error_OTA;
typedef std::function<void(unsigned int, unsigned int)> THandlerFunction_Progress_OTA;

class ArduinoOTAClass {
public:
  void setPort(uint16_t) {}
  void setHostname(const char *) {}
  void setInstanceType(const char *) {}
  void setPassword(const char *) {}
  void setPasswordHash(const char *) {}
  void onStart(THandlerFunction_OTA fn) { (void)fn; }
  void onEnd(THandlerFunction_OTA fn) { (void)fn; }
  void onError(THandlerFunction_Error_OTA fn) { (void)fn; }
  void onProgress(THandlerFunction_Progress_OTA fn) { (void)fn; }
  void begin() {}
  void handle() {}
  int getCommand() { return 0; }  // U_FLASH
};
extern ArduinoOTAClass ArduinoOTA;
#endif
