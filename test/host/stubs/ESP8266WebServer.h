/* ESP8266WebServer.h — host stub for the replay harness (see stubs/Arduino.h).
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 *
 * Only the surface used by P1P2_ESP8266HTTPUpdateServer and by setup()/loop() is
 * provided. Requests never arrive, so no handler ever runs during a replay.
 */
#ifndef HOST_STUB_ESP8266WEBSERVER_H
#define HOST_STUB_ESP8266WEBSERVER_H
#include "Arduino.h"
#include "WiFiClient.h"
#include "WiFiServer.h"
#include "Update.h"
#include <functional>

enum HTTPMethod { HTTP_ANY = 0, HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_PATCH, HTTP_DELETE, HTTP_OPTIONS };
enum UploadStatus { UPLOAD_IDLE = 0, UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END, UPLOAD_FILE_ABORTED };
enum AuthenticationType { BASIC_AUTH, WEB_AUTH };

typedef std::function<void(void)> THandlerFunction;

struct HTTPUpload {
  UploadStatus status = UPLOAD_IDLE;
  String filename;
  String name;
  uint32_t currentSize = 0;
  uint32_t totalSize = 0;
  uint8_t buf[1460] = {0};
};

#define HTTP_UPLOAD_BUFLEN 1460

namespace esp8266webserver {

template <typename ServerType>
class ESP8266WebServerTemplate {
public:
  ESP8266WebServerTemplate(uint16_t port = 80) : _port(port) {}

  void on(const String &, THandlerFunction fn) { (void)fn; }
  void on(const String &, HTTPMethod, THandlerFunction fn) { (void)fn; }
  void on(const String &, HTTPMethod, THandlerFunction fn, THandlerFunction ufn) { (void)fn; (void)ufn; }
  void on(const char *uri, HTTPMethod m, THandlerFunction fn) { on(String(uri), m, fn); }
  void on(const char *uri, HTTPMethod m, THandlerFunction fn, THandlerFunction ufn) { on(String(uri), m, fn, ufn); }
  void onNotFound(THandlerFunction fn) { (void)fn; }

  void begin() {}
  void close() {}
  void handleClient() {}

  WiFiClient &client() { return _client; }
  HTTPUpload &upload() { return _upload; }

  void send(int code, const String &contentType = String(), const String &content = String()) { (void)code; (void)contentType; (void)content; }
  void send(int code, const char *contentType, const char *content) { send(code, String(contentType), String(content)); }
  void send_P(int code, const char *contentType, const char *content) { send(code, String(contentType), String(content)); }
  void send_P(int code, PGM_P contentType, const String &content) { send(code, String((const char *)contentType), content); }
  void sendContent(const String &content) { (void)content; }
  void sendContent_P(const char *content) { (void)content; }
  void sendHeader(const String &, const String &, bool = false) {}
  void sendHeader(const char *, const char *, bool = false) {}
  bool authenticate(const char *, const char *) { return true; }
  void requestAuthentication(AuthenticationType = BASIC_AUTH) {}
  void collectHeaders(const char *[], uint8_t) {}

  String arg(const String &) { return String(); }
  String arg(const char *) { return String(); }
  String argName(uint8_t) { return String(); }
  int args() { return 0; }
  bool hasArg(const String &) { return false; }
  bool hasArg(const char *) { return false; }
  String uri() { return String(); }
  String hostHeader() { return String(); }
  HTTPMethod method() { return HTTP_GET; }
  String contentType() { return String(); }
  uint32_t contentLength() { return 0; }
  uint16_t port() const { return _port; }

protected:
  uint16_t _port;
  WiFiClient _client;
  HTTPUpload _upload;
};

};  // namespace esp8266webserver

using esp8266webserver::ESP8266WebServerTemplate;
using ESP8266WebServer = esp8266webserver::ESP8266WebServerTemplate<WiFiServer>;
namespace BearSSL {
using ESP8266WebServerSecure = esp8266webserver::ESP8266WebServerTemplate<WiFiServerSecure>;
};

using ESP8266WebServerLegacy = esp8266webserver::ESP8266WebServerTemplate<WiFiServer>;
#endif
