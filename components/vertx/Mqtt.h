#ifndef MQTT_H
#define MQTT_H
#include <vertx.h>
#include <Erc.h>
extern "C" {
#include <esp_mqtt.h>
};

#define HOST_LENGTH 40
#define USER_LENGTH 40
#define PASSWORD_LENGTH 40
#define CLIENT_ID_LENGTH 40

class Mqtt : public VerticleCoRoutine {
  static Mqtt* _me;
  Str _host;
  uint32_t _port;
  Str _user;
  Str _password;
  Str _clientId;
  Str _willTopic;
  Str _willMessage;
  int _willQos;
  bool _willRetain;
  uint32_t _keepAlive;
  uid_t _wifi;
  bool _wifiConnected;
    bool _mqttConnected;
    Str _topicIncoming;
    Str _topic;
    Str _message;
    Str _topicTxd;
    Str _messageTxd;
    bool _busyTxd;

 public:
  Mqtt(const char* name);
  void init();
  Erc configure(const char* host, uint16_t port, const char* user,
                const char* password);
  void setWifi(uid_t wifi) { _wifi = wifi; }

  void onConnected();
  void onDisconnected();
  void onMessageReceived(const char* topic, uint8_t* payload, uint32_t size);

  static void onStatusChange(esp_mqtt_status_t status);
  static void onMessage(const char* topic, uint8_t* payload, uint32_t size);

  void publish(const char* topic, uint8_t* payload, size_t len, int qos,
               bool retained);
  void start();
  void run();
};
#endif