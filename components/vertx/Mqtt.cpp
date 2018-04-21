

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
      _keepAlive(20),_topicIncoming(50),_topic(50),_message(100),_topicTxd(50),_messageTxd(100),_messageRxd(10),_topicRxd(50),_messageEb(100)
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
// static handler for esp_mqtt
void Mqtt::onMessage(const char *topic, uint8_t *payload, uint32_t size)
{
    _me->onMessageReceived(topic, payload, size);
}

void Mqtt::onConnected()
{
    _mqttConnected = true;
    Str subTopic(50);
    subTopic="dst/";
    subTopic+= Sys::hostname();
    subTopic+= "/#";
    // subscribe to my main destination
    if ( esp_mqtt_subscribe(subTopic.c_str(),0) )
        INFO(" subscribed to %s",subTopic.c_str());;
    eb.publish("mqtt/connected");
}

void Mqtt::onDisconnected()    //
{
    _mqttConnected = false;
    eb.publish("mqtt/disconnected");
};
/*
 * receive MQTT event , translate to eb message
 *
 * dst/<myName>/<service>/<property> : value
 *  set property via property function
 * dst/<myName>/<service> : { json_object }
 *  create event "service" publish ( "service" , Message with all json fields );
 *
 */
void Mqtt::onMessageReceived(const char *topic, uint8_t *payload,
                             uint32_t size)
{
    uint32_t offsetSlash[5];
    _messageRxd.clear();
    _messageRxd.write(payload,0,size);
 //   INFO("PUB RXD : %s = %s ",topic,_messageRxd.c_str());

    const char* first=topic;
    const char* ptr = first;

    uint32_t idx=0;
    while ( ptr && (idx < 5) ) {
        ptr = strchr(ptr,'/');
        if ( ptr ) {
            offsetSlash[idx]=ptr-first;
            ptr++;
        } else {
            offsetSlash[idx]=0;
            break;
        }
        idx++;

    }

    if ( (idx > 1 ) && offsetSlash[0] && offsetSlash[1] && offsetSlash[2] && offsetSlash[3]==0 ) { // found 3 slashes
        _topicRxd = &topic[offsetSlash[1]+1];
        INFO("property set %s = %s",_topicRxd.c_str(),_messageRxd.c_str());
        _messageEb.clear();
        _messageEb.put(H("value"),_messageRxd);
        _messageEb.put(H("key"),H(_topicRxd.c_str()));
        eb.publish("property/set",_messageEb);
    } else {
        INFO(" no property or command found in %s , slashes at : %d %d %d %d %d ",
             topic,
             offsetSlash[0],
             offsetSlash[1],
             offsetSlash[2],
             offsetSlash[3],
             offsetSlash[4]);
    }


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
    _willMessage= "false";
    _keepAlive = 5;
    //   esp_mqtt_lwt(_willTopic.c_str(), _willMessage.c_str(), 1, false); // implemented in 0.5 however breaks
//    INFO(" esp_mqtt_init(onStatusChange, onMessage, 256, 2000)  ");
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
//           WARN(" Publish lost : BUSY.");
        }
    });
    VerticleCoRoutine::start();
}

void Mqtt::publish(const char *topic, uint8_t *payload, size_t len, int qos,
                   bool retained)
{
    if ( ! _mqttConnected ) return;
 //       INFO("esp_mqtt_publish(topic:%s, payload, len, qos:%d, retained:%d) ",topic,qos,retained);

    if (!esp_mqtt_publish(topic, payload, len, qos, retained)) {
        ERROR(" mqtt publish failed : %s size:%d ", topic, len);
    } else {
        /*
                Str data(100);
                data.write(payload,0,len);
                INFO(" published %s = %s", topic,data.c_str());*/
    }
}

void Mqtt::run()
{
    PT_BEGIN();
    while(true) {
        PT_WAIT_SIGNAL(1000);
        if ( hasSignal(SIG_WIFI_CONNECTED )) {

        } else if ( hasSignal(SIG_WIFI_DISCONNECTED )) {
 //            INFO("esp_mqtt_stop(); ");
            esp_mqtt_stop();
            onDisconnected();
        }  else if ( hasSignal(SIG_MQTT_PUBLISH )) {
            publish(_topicTxd.c_str(), _messageTxd.data(), _messageTxd.length(), 0, false);
            _busyTxd=false;
        } else if ( hasSignal(SIG_MQTT_SUBSCRIBE )) {
 //            INFO("esp_mqtt_subscribe(_topicTxd.c_str(), 0) ");
            if (!esp_mqtt_subscribe(_topicTxd.c_str(), 0) ) {
                ERROR(" mqtt subscribe failed");
            }
            _busyTxd=false;
        } else { // timeout
            if ( _wifiConnected && !_mqttConnected) {
 //                INFO("esp_mqtt_start(_host.c_str(), 1883, _clientId.c_str(), _user.c_str(),_password.c_str())");
                esp_mqtt_start(_host.c_str(), "1883", _clientId.c_str(), _user.c_str(),
                               _password.c_str());
            }
        }
    }
    PT_END();
}
