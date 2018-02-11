

#include <Config.h>
#include <Mqtt.h>

enum {
    SIG_WIFI_CONNECTED,
    SIG_WIFI_DISCONNECTED,
    SIG_MQTT_PUBLISH,
    SIG_MQTT_SUBSCRIBE
};

Mqtt *Mqtt::_me = 0;

Mqtt::Mqtt(const char *name)
    : VerticleCoRoutine(name),
      _host(HOST_LENGTH),
      _port(1883),
      _user(USER_LENGTH),
      _password(PASSWORD_LENGTH),
      _clientId(CLIENT_ID_LENGTH),
      _willTopic(30),
      _willMessage(30),
      _willQos(0),
      _willRetain(false),
      _keepAlive(20),_topicIncoming(50),_topic(50),_message(100),_topicTxd(50),_messageTxd(100)
{
    _me = this;
    _mqttConnected = false;
}


Erc Mqtt::configure(const char *host, uint16_t port, const char *user,
                    const char *password)
{
    _host = host;
    _port = port;
    _user = user;
    _password = password;
    return E_OK;
}

void Mqtt::onStatusChange(esp_mqtt_status_t status)
{
    switch (status) {
    case ESP_MQTT_STATUS_CONNECTED:
        _me->onConnected();
        break;
    case ESP_MQTT_STATUS_DISCONNECTED:
        _me->onDisconnected();
        break;
    }
}

void Mqtt::onMessage(const char *topic, uint8_t *payload, uint32_t size)
{
    _me->onMessageReceived(topic, payload, size);
    INFO(" mqtt topic received : %s ", topic);
}

void Mqtt::onConnected()
{
    _mqttConnected = true;
    eb.publish("mqtt/connected");
}

void Mqtt::onDisconnected()    //
{
    _mqttConnected = false;
    eb.publish("mqtt/disconnected");
};

void Mqtt::onMessageReceived(const char *topic, uint8_t *payload,
                             uint32_t size)
{
    Str data(0);
    data.map(payload, size);
    INFO("PUB RXD : %s = %s ",topic,data.c_str());
}

void Mqtt::start()
{
    config.setNameSpace("mqtt");
    config.get("host", _host, "limero.ddns.net");
    config.get("port", _port, 1883);
    _clientId = Sys::hostname();
    config.get("user", _user, "");
    config.get("password", _password, "");
    _willTopic = "src/";
    _willTopic += Sys::hostname();
    _willTopic += "/system/alive";
    //    config.get("mqtt.willTopic",_willTopic,_willTopic.c_str());
    config.get("willMessage", _willMessage, "false");
    config.get("keepAlive", _keepAlive, 20);
    esp_mqtt_init(onStatusChange, onMessage, 256, 2000);
    eb.on("wifi/connected",[this](Message& msg) {
        _wifiConnected=true;
        signal(SIG_WIFI_CONNECTED);
    });
    eb.on("wifi/disconnected",[this](Message& msg) {
        _wifiConnected=false;
        signal(SIG_WIFI_DISCONNECTED);
    });
    eb.on("mqtt/publish",[this](Message& msg) {
//        INFO(" %s %s %d",__func__,"Mqtt::publish",_mqttConnected);
        if ( !_mqttConnected ) return;
        if ( !_busyTxd ) {
            if ( msg.get(H("topic"),_topicTxd) && msg.get(H("message"),_messageTxd)) {
                _busyTxd=true;
                signal(SIG_MQTT_PUBLISH);
//               INFO(" %s %s %d",__func__,"Mqtt::signal",_mqttConnected);
            }
        } else {
            WARN(" Publish lost : BUSY.");
        }
    });
    eb.on("mqtt/subscribe",[this](Message& msg) {
        if ( !_mqttConnected ) return;
        if ( !_busyTxd ) {
            if ( msg.get(H("topic"),_topicTxd) ) {
                _busyTxd=true;
                signal(SIG_MQTT_SUBSCRIBE);
            }
        } else {
            WARN(" Publish lost : BUSY.");
        }
    });
    VerticleCoRoutine::start();
}

void Mqtt::publish(const char *topic, uint8_t *payload, size_t len, int qos,
                   bool retained)
{
    if (!esp_mqtt_publish(topic, payload, len, qos, retained)) {
        ERROR(" mqtt publish failed : %s size:%d ", topic, len);
        esp_mqtt_stop();
        esp_mqtt_start(_host.c_str(), _port, _clientId.c_str(), _user.c_str(),
                       _password.c_str());
    } else {
        // INFO(" published %s", topic);
    }
}

void Mqtt::run()
{
    PT_BEGIN();
    while(true) {
        PT_WAIT_SIGNAL(1000);
        if ( hasSignal(SIG_WIFI_CONNECTED )) {

            esp_mqtt_start(_host.c_str(), _port, _clientId.c_str(), _user.c_str(),
                           _password.c_str());
        } else if ( hasSignal(SIG_WIFI_DISCONNECTED )) {
            esp_mqtt_stop();
            onDisconnected();
        }  else if ( hasSignal(SIG_MQTT_PUBLISH )) {
            publish(_topicTxd.c_str(), _messageTxd.data(), _messageTxd.length(), 1, false);
            _busyTxd=false;
        } else if ( hasSignal(SIG_MQTT_SUBSCRIBE )) {
            if (!esp_mqtt_subscribe(_topicTxd.c_str(), 0) ) {
                ERROR(" mqtt subscribe failed");
            }
            _busyTxd=false;
        }
    }
    PT_END();
}
