/* AsyncMqttClient.h — host stub for the replay harness.
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 *
 * This is the one stub that carries the payload of the whole harness: publish()
 * does not go to a broker, it records "<topic> <payload>" for the replay output.
 * Everything else (topic construction, publish-on-change, HA discovery) is the
 * real firmware code; only this transport is replaced.
 */
#ifndef HOST_STUB_ASYNCMQTTCLIENT_H
#define HOST_STUB_ASYNCMQTTCLIENT_H
#include "Arduino.h"
#include <functional>

enum AsyncMqttClientDisconnectReason : uint8_t {
  AsyncMqttClientDisconnectReasonTcpDisconnected = 0,
  AsyncMqttClientDisconnectReasonExpectedButReceivedDisconnect = 1,
  AsyncMqttClientDisconnectReasonUnrecognized = 2,
  AsyncMqttClientDisconnectReasonBlockedWithTooManyConnections = 3,
  AsyncMqttClientDisconnectReasonSessionTakenOver = 4,
  AsyncMqttClientDisconnectReasonMQTTServerMoved = 5,
  AsyncMqttClientDisconnectReasonServerUnavailable = 6,
  AsyncMqttClientDisconnectReasonBadCredentials = 7,
  AsyncMqttClientDisconnectReasonUnacceptable = 8,
  AsyncMqttClientDisconnectReasonProtocolVersionRejectedV5 = 9,
  AsyncMqttClientDisconnectReasonClientIdAlreadyInUse = 10,
  AsyncMqttClientDisconnectReasonNotAuthorized = 11,
  AsyncMqttClientDisconnectReasonServerNotAvailable = 12,
  AsyncMqttClientDisconnectReasonServerBusy = 13,
  AsyncMqttClientDisconnectReasonBadUsernameOrPassword = 14,
  AsyncMqttClientDisconnectReasonUseAnotherServer = 15,
  AsyncMqttClientDisconnectReasonServerShuttingDown = 16,
  AsyncMqttClientDisconnectReasonMqttUnspecificError = 0x80,
  AsyncMqttClientDisconnectReasonMqttUnacceptableProtocolVersionError = 0x81,
  AsyncMqttClientDisconnectReasonMqttIdentifierRejectedError = 0x82,
  AsyncMqttClientDisconnectReasonMqttServerUnavailableError = 0x83,
  AsyncMqttClientDisconnectReasonMqttBadUsernameOrPasswordError = 0x84,
  AsyncMqttClientDisconnectReasonMqttNotAuthorizedError = 0x85,
  AsyncMqttClientDisconnectReasonMqttTenantOrTopicInvalidError = 0x90,
  AsyncMqttClientDisconnectReasonMqttTooManyConnections = 0x9A
};

struct AsyncMqttClientMessageProperties {
  uint8_t dup;
  uint8_t qos;
  bool retain;
  uint32_t size;
  uint32_t index;
  uint32_t total;
};

// Implemented by replay.cpp - the recorder for every published message.
void hostRecordPublish(const char *topic, uint8_t qos, bool retain, const char *payload);
// Implemented by replay.cpp - lets the driver decide if the broker is reachable.
bool hostMqttLinkUp();

class AsyncMqttClient {
public:
  using THandlerFunction_Connect = std::function<void(bool)>;
  using THandlerFunction_Disconnect = std::function<void(AsyncMqttClientDisconnectReason)>;
  using THandlerFunction_Message =
      std::function<void(char *, char *, const AsyncMqttClientMessageProperties &, size_t, size_t, size_t)>;
  using THandlerFunction_Publish = std::function<void(uint16_t)>;

  AsyncMqttClient &onConnect(THandlerFunction_Connect fn) { _onConnect = fn; return *this; }
  AsyncMqttClient &onDisconnect(THandlerFunction_Disconnect fn) { _onDisconnect = fn; return *this; }
  AsyncMqttClient &onMessage(THandlerFunction_Message fn) { _onMessage = fn; return *this; }
  AsyncMqttClient &onPublish(THandlerFunction_Publish fn) { _onPublish = fn; return *this; }

  AsyncMqttClient &setWill(const char *topic, uint8_t qos, bool retain, const char *payload = nullptr) {
    (void)topic; (void)qos; (void)retain; (void)payload;
    return *this;
  }
  AsyncMqttClient &setServer(IPAddress, uint16_t) { return *this; }
  AsyncMqttClient &setServer(const char *, uint16_t) { return *this; }
  AsyncMqttClient &setClientId(const char *) { return *this; }
  AsyncMqttClient &setCleanSession(bool) { return *this; }
  AsyncMqttClient &setKeepAlive(uint16_t) { return *this; }
  AsyncMqttClient &setCleanWill(bool) { return *this; }
  AsyncMqttClient &setCredentials(const char *, const char *) { return *this; }
  AsyncMqttClient &setCertificateFingerprint(const char *const) { return *this; }

  uint16_t subscribe(const char *topic, uint8_t qos) { (void)qos; ++_subscriptions; _lastSubscribe = topic; return _subscriptions; }
  uint16_t unsubscribe(const char *topic) { (void)topic; return 1; }

  uint16_t publish(const char *topic, uint8_t qos, bool retain, const char *payload = nullptr,
                   size_t length = 0, bool dup = false) {
    (void)length;
    (void)dup;
    if (!hostMqttLinkUp()) return 0;
    hostRecordPublish(topic, qos, retain, payload ? payload : "");
    return ++_published;
  }

  void connect() { _linkUp = hostMqttLinkUp(); if (_linkUp && _onConnect) _onConnect(false); }
  void disconnect(bool = false) { if (_linkUp && _onDisconnect) _onDisconnect(AsyncMqttClientDisconnectReasonTcpDisconnected); _linkUp = false; }
  bool connected() const { return _linkUp && hostMqttLinkUp(); }
  bool connected(bool) const { return connected(); }
  void clearQueue() {}
  void loop() {}
  uint32_t outboxCount() { return 0; }

private:
  THandlerFunction_Connect _onConnect;
  THandlerFunction_Disconnect _onDisconnect;
  THandlerFunction_Message _onMessage;
  THandlerFunction_Publish _onPublish;
  bool _linkUp = false;
  uint16_t _subscriptions = 0;
  uint16_t _published = 0;
  String _lastSubscribe;
};
#endif
