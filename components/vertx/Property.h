#ifndef PROPERTY_H
#define PROPERTY_H
#include <LinkedList.hpp>
#include <vertx.h>
//#include <memory.h>
#include <ArduinoJson.h>

class Property : public LinkedList<Property>
{
public:
    uint64_t _timeout;
    uid_t _uid;
    uint32_t _interval;
    Property(const char* name,uint32_t interval) : _interval(interval) {
        _uid = UID.add(name);
        _timeout = Sys::millis()+interval;
        add(this);
    }
    virtual void toJson(Str&) {};
    void updated(){
        _timeout=Sys::millis()-1;
    }

};

template <typename T>
class PropertyReference : public Property
{
public:
    T& _var;

    PropertyReference(const char* name,T& var,uint32_t interval) : Property(name,interval),_var(var) {
    }

    ~PropertyReference() {
    }
    void toJson(Str& msg) {
        msg.append(_var);
    }
};

template <typename T>
class PropertyFunction : public Property
{
public:
    std::function<T()> _f;

    PropertyFunction(const char* name,std::function<T ()> f,uint32_t interval) : Property(name,interval),_f(f) {
    }

    ~PropertyFunction() {
    }
    void toJson(Str& msg) {
        msg.append(_f());
    }
};


class PropertyVerticle : public VerticleCoRoutine
{
    Property* _currentProp;
    bool  _mqttConnected;
    Message _toMqttMsg;
    Str _topic;
    Str _message;
    void sendProp(Property* p);
public:
    PropertyVerticle(const char* name);
    void start();
    void run();
};

extern PropertyVerticle propertyVerticle;



#endif // PROPERTY_H
