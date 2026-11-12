/* ESP8266WiFi.h — host stub for the replay harness (see stubs/Arduino.h).
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 *
 * WiFi has no meaning offline. isConnected() is configurable so that loop()
 * reaches the MQTT branch deterministically.
 */
#ifndef HOST_STUB_ESP8266WIFI_H
#define HOST_STUB_ESP8266WIFI_H
#include "Arduino.h"
#include "WiFiClient.h"

enum wl_status_t { WL_IDLE_STATUS = 0, WL_NO_SSID_AVAIL = 1, WL_CONNECTED = 3, WL_CONNECT_FAILED = 4, WL_DISCONNECTED = 6 };

class ESP8266WiFiClass {
public:
  bool connected = true;   // harness decides; see replay.cpp --wifi
  void persistent(bool) {}
  void begin() {}
  void begin(const char *, const char * = nullptr) {}
  void disconnect(bool = false, bool = false) {}
  bool isConnected() { return connected; }
  wl_status_t status() { return connected ? WL_CONNECTED : WL_DISCONNECTED; }
  int8_t RSSI() { return -50; }
  IPAddress localIP() { return IPAddress(10, 0, 1, 99); }
  IPAddress softAPIP() { return IPAddress(192, 168, 4, 1); }
  IPAddress subnetMask() { return IPAddress(255, 255, 255, 0); }
  IPAddress gatewayIP() { return IPAddress(10, 0, 1, 1); }
  String SSID() { return String("host"); }
  String psk() { return String(""); }
  String macAddress() { return String("aa:bb:cc:dd:ee:ff"); }
  void setHostname(const char *) {}
  void mode(uint8_t) {}
};

// sdk-level bits the sketch uses for its startup report
enum if_t { STATION_IF = 0x00, SOFTAP_IF = 0x10 };
inline void wifi_get_macaddr(if_t iface, uint8_t *mac) {
  static const uint8_t sta[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
  static const uint8_t ap[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x02};
  memcpy(mac, iface == STATION_IF ? sta : ap, 6);
}

extern ESP8266WiFiClass WiFi;
#endif
