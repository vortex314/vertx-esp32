#ifndef MQTTSERIAL_H
#define MQTTSERIAL_H
#include <vertx.h>
#include <Erc.h>
#include <Str.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
#include <Hardware.h>
class MqttSerial : public VerticleCoRoutine
{
    bool _mqttConnected;
    Str _topic;
    Str _message;
    Str _topicTxd;
    Str _messageTxd;
    bool _busyTxd;
    Str _messageRxd;
    Str _topicRxd;
    StaticJsonBuffer<1024> jsonBuffer;
    char _txdBuffer[256];
    Str _rxdLine;
    UART& _uart;
    Message _messageEb;
    Str _myUptime;
    Timer _myUptimeTimer;
    void onRxdData();
    void onRxdLine();
    void onMessageReceived(const char *topic, const char*payload);
public:
    MqttSerial(const char* name);
    ~MqttSerial();
    void init();
    void start();
    void run();

    void publish(Str& topic,Str& message, int qos,bool retained);
    void subscribe(Str& topics);
};

#endif // MQTTSERIAL_H
