/* host_env.h — the whole stub environment, in the order the host build needs it.
 *
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 *
 * Included (via -include) before the firmware sources:
 *   1. stubs/Arduino.h            types, String, Print/Stream, virtual time, ESP
 *   2. the stubbed ESP/Arduino libraries the sketch includes
 *   3. prototypes.h               generated declarations for the sketch functions,
 *                                 so the sketch's own call order compiles
 * After this file, replay.cpp includes the sketch itself.
 */
#ifndef HOST_STUB_ENV_H
#define HOST_STUB_ENV_H

#include "Arduino.h"

// The sketch's own leaf headers, before the generated prototypes so that the
// prototypes can use their types. The sketch includes them again later; the
// #ifndef guards in those files make the second include a no-op, so this changes
// nothing for the firmware build (which does not see this file at all).
#include "P1P2_Config.h"
#include "P1P2_NetworkParams.h"
#include "P1P2_HomeAssistant.h"
#include "P1P2_System.h"

#include "WiFiClient.h"
#include "WiFiServer.h"
#include "WiFiUdp.h"
#include "ESP8266WiFi.h"
#include "ESP8266mDNS.h"
#include "DNSServer.h"
#include "Update.h"
#include "FS.h"
#include "StreamString.h"
#include "flash_hal.h"
#include "ESP8266WebServer.h"
#include "WiFiManager.h"
#include "ESPTelnet.h"
#include "AsyncMqttClient.h"
#include "EEPROM.h"
#include "SPI.h"
#include "ESP8266AVRISP.h"
#include "ArduinoOTA.h"
#include "TZ.h"
#include "W5500lwIP.h"

#include "prototypes.h"  // generated into the build directory

#endif
