#include "MqttSerial.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
enum {
    SIG_WIFI_CONNECTED,
    SIG_WIFI_DISCONNECTED,
    SIG_MQTT_PUBLISH,
    SIG_MQTT_SUBSCRIBE,
    SIG_UART_RXD_LINE,
    SIG_UPTIME_RECEIVED
};

MqttSerial::MqttSerial(const char *name)
    : VerticleCoRoutine(name),
      _topic(50),
      _message(100),
      _topicTxd(50),
      _messageTxd(100),
      _messageRxd(100),
      _topicRxd(50),
      _rxdLine(256),
      _uart(UART::create(1,3)),
      _messageEb(256),
      _myUptime(40)
{
}

MqttSerial::~MqttSerial()
{
}

void MqttSerial::start()
{
    _myUptime="src/";
    _myUptime+=Sys::hostname(); // is not created at instanciation of class
    _myUptime+="/system/upTime";

    eb.on("mqtt/publish",[this](Message& msg) {
        if ( !_busyTxd ) {
            if ( msg.get(H("topic"),_topicTxd) && msg.get(H("message"),_messageTxd)) {
                _busyTxd=true;
                signal(SIG_MQTT_PUBLISH);
            }
        } else {
            WARN(" Publish lost : BUSY.");
        }
    });
    eb.on("mqtt/subscribe",[this](Message& msg) {
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
//   esp_wifi_deinit();
    _uart.onRxd([](void* pv) {
        MqttSerial* me=(MqttSerial*)pv;
        me->onRxdData();
    },this);
    _uart.init();
}

void MqttSerial::run()
{
    PT_BEGIN();
    _myUptimeTimer.atInterval(3000).doThis([this]() {
        eb.publish("mqtt/disconnected");
        subscribe(_myUptime);
        Str sTime(10);
        sTime.append(Sys::sec());
        publish(_myUptime,sTime,0,false);
    });
    while(true) {
        PT_WAIT_SIGNAL(5000);
        _myUptimeTimer.check();
        if ( hasSignal(SIG_MQTT_PUBLISH )) {
            publish(_topicTxd, _messageTxd, 0, false);
            _busyTxd=false;
        } else if ( hasSignal(SIG_MQTT_SUBSCRIBE )) {
            subscribe(_topicTxd);
            _busyTxd=false;
        } else if ( hasSignal(SIG_UPTIME_RECEIVED )) {
            eb.publish("mqtt/connected");
            _myUptimeTimer.atInterval(3000); // reset timer to next 3 sec 
        } else { // timeout

        }
    }
    PT_END();
}

void MqttSerial::publish(Str& topic,Str& message,int qos,bool retained)
{
    JsonObject& json = jsonBuffer.createObject();
    json["cmd"]="MQTT-PUB";
    json["topic"]=topic.c_str();
    json["message"]=message.c_str();
    if ( qos > 0 ) json["qos"]=qos;
    if ( retained != false ) json["retained"]=retained;
    json.printTo(_txdBuffer,sizeof(_txdBuffer));
    printf("%s\n",_txdBuffer);
    jsonBuffer.clear();
}

void MqttSerial::subscribe(Str& topics)
{
    JsonObject& json = jsonBuffer.createObject();
    json["cmd"]="MQTT-SUB";
    json["topic"]=topics.c_str();
    json.printTo(_txdBuffer,sizeof(_txdBuffer));
    printf("%s\n",_txdBuffer);
    jsonBuffer.clear();
}

void MqttSerial::onRxdData()
{
    while(_uart.hasData()) {
        uint8_t b=_uart.read();
//       INFO("%s() 0x%x",__func__,b);
        if ( b=='\n' || b=='\r' ) {
            if ( _rxdLine.length() )  onRxdLine();
            _rxdLine.clear();
        } else {
            _rxdLine.write(b);
        }
    }
}

void MqttSerial::onRxdLine()
{
//    INFO(" line received : %s ",_rxdLine.c_str());
    if ( _rxdLine.startsWith("{") ) {
        DEBUG("%s : PARSE",__func__);
        JsonObject& json = jsonBuffer.parseObject(_rxdLine.c_str());
        if ( !json.success()) {
            WARN(" JSON cmd parse failed ");
            return;
        }
        if ( json.containsKey("cmd") && strcmp("MQTT-PUB",json["cmd"])==0) {
            DEBUG("%s : SUCCESS cmd",__func__);
            if ( strcmp(json.get<const char *>("topic"),_myUptime.c_str())==0) {
                signal(SIG_UPTIME_RECEIVED);
            } else {
                onMessageReceived(json.get<const char *>("topic"),json.get<const char *>("message"));
            }
        } else {
            WARN(" JSON unknown or incomplete cmd ");
        }
        jsonBuffer.clear();
    } else {
        WARN(" no JSON object in rxd ");
    }

}

void MqttSerial::onMessageReceived(const char *topic, const char*payload)
{
    uint32_t offsetSlash[5];
    _messageRxd=payload;
    INFO("PUB RXD : %s = %s ",topic,_messageRxd.c_str());

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
