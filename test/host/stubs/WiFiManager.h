/* WiFiManager.h — host stub for the replay harness (see stubs/Arduino.h).
 * There is no captive portal offline: setup() reaches WiFiManager only when
 * ethernet is "absent"; autoConnect() succeeds immediately so nothing blocks.
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_WIFIMANAGER_H
#define HOST_STUB_WIFIMANAGER_H
#include "Arduino.h"
#include <functional>
#include <string>
#include <vector>

enum WiFiParamType { WT_TEXT = 0, WT_PASSWORD = 1, WT_NUMBER = 2, WT_FILE = 3 };

class WiFiManagerParameter {
public:
  explicit WiFiManagerParameter(const char *html) : _id(""), _label(html ? html : ""), _value(""), _length(0) {}
  WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, size_t length,
                       const char *placeholder = nullptr, uint8_t type = WT_TEXT, const char *attributes = nullptr)
      : _id(id ? id : ""), _label(label ? label : ""), _value(defaultValue ? defaultValue : ""), _length(length) {
    (void)placeholder;
    (void)type;
    (void)attributes;
  }
  const char *getValue() { return _value.c_str(); }
  const char *getID() { return _id.c_str(); }
  const char *getLabel() { return _label.c_str(); }
  void setValue(const char *v) { _value = v ? v : ""; }

private:
  std::string _id, _label, _value;
  size_t _length;
};

class WiFiManager;
using WiFiManagerSaveCb = std::function<void(void)>;
using WiFiManagerApCb = std::function<void(WiFiManager *)>;

class WiFiManager {
public:
  void setDebugOutput(bool) {}
  void setSaveConfigCallback(WiFiManagerSaveCb cb) { _saveCb = cb; }
  void setAPCallback(WiFiManagerApCb cb) { (void)cb; }
  void addParameter(WiFiManagerParameter *p) { _params.push_back(p); }
  void setConfigPortalTimeout(unsigned int) {}
  void setConnectTimeout(int) {}
  void setAPStaticIPConfig(IPAddress, IPAddress, IPAddress) {}
  void setSTAStaticIPConfig(IPAddress, IPAddress, IPAddress, IPAddress = IPAddress()) {}
  void resetSettings() {}
  bool autoConnect(char * = nullptr, char * = nullptr) { return true; }
  bool autoConnect(const char * = nullptr, const char * = nullptr) { return true; }
  bool startConfigPortal(char * = nullptr, char * = nullptr) { return true; }
  bool startConfigPortal(const char * = nullptr, const char * = nullptr) { return true; }
  bool startAPOnly(char * = nullptr, char * = nullptr) { return true; }
  String getConfigPortalSSID() { return String("P1P2MQTT"); }
  String getWiFiSSID() { return String("host"); }
  String getWiFiPass() { return String(""); }
  IPAddress getFinalIP() { return IPAddress(10, 0, 1, 99); }

private:
  std::vector<WiFiManagerParameter *> _params;
  WiFiManagerSaveCb _saveCb;
};
#endif
