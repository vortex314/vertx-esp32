#include "Property.h"

uint32_t now;
PropertyReference<uint32_t> nowProp("now",now,2000);

#define MQTT_CONNECTED 0
#define MQTT_DISCONNECTED 1

PropertyVerticle::PropertyVerticle(const char* name) : VerticleCoRoutine(name),_toMqttMsg(100),_topic(50),_message(100)
{
    _currentProp=0;
    _mqttConnected=false;
}

bool alive=true;


void PropertyVerticle::start()
{
    new PropertyReference<bool>("system/alive",alive,5000);
    new PropertyFunction<uint64_t>("system/upTime",Sys::millis,5000);
    new PropertyFunction<uint32_t>("system/heap",Sys::getFreeHeap,5000);
    new PropertyFunction<uint64_t>("system/serialId",Sys::getSerialId,5000);
    eb.on("mqtt/connected",[this](Message& msg) {
        _mqttConnected=true;
    });
    eb.on("mqtt/disconnected",[this](Message& msg) {
        _mqttConnected=false;
    });
    VerticleCoRoutine::start();
}

void PropertyVerticle::sendProp(Property* p)
{
    _toMqttMsg.clear();
    _topic.clear();
    _message.clear();
    _topic = "src/";
    _topic += Sys::hostname();
    _topic +="/";
    _topic +=UID.label(_currentProp->_uid);
    _currentProp->toJson(_message);

    _toMqttMsg.put(H("topic"),_topic);
    _toMqttMsg.put(H("message"),_message);
    eb.publish("mqtt/publish",_toMqttMsg);
    p->_timeout = Sys::millis()+p->_interval;
}

void PropertyVerticle::run()
{
//   INFO("%X ",signal());
    PT_BEGIN();
    while(true) {
        INFO("0");
        while(!_mqttConnected) {
            PT_WAIT(1000);
        }

        while(_mqttConnected ) {
            _currentProp=Property::first();
            while (_currentProp ) {
                if ( _currentProp->_timeout < Sys::millis()) {
                    sendProp(_currentProp);
 //                   _currentProp=Property::first();
                    PT_WAIT(10);
                }
                _currentProp=_currentProp->next();
            }
            PT_WAIT(100);
        };

    }
    PT_END();
}
