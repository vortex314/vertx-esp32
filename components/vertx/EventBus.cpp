#include <vertx.h>

class Consumer : public LinkedList<Consumer>
{
public:
    uid_t _eventUid;
    EventHandler _handler;

    Consumer(uid_t eventUid,EventHandler handler) : _eventUid(eventUid),_handler(handler) {
        add(this);
    };
};

#ifdef ESP32_IDF 
#else
namespace std
{
void __throw_bad_function_call()
{
    ERROR(" bad function call, invalid std::function<> ");
    while(1);
};
};
#endif

#include <CborQueue.h>

EventBus::EventBus(uint32_t size) :_queue(size)
{

}

Erc EventBus::on(EventLabel address,EventHandler f)
{
    return consumer(address,f);
}

Erc EventBus::consumer(EventLabel address,EventHandler f)
{
    uid_t uid=UID.add(address);
    new Consumer(uid,f);
    return E_OK;
}

Erc EventBus::publish(EventLabel address,Message& message)
{
//    INFO(" %s %s ",__func__,address);
//    INFO("IN %s",__func__);
    static  portMUX_TYPE mux= portMUX_INITIALIZER_UNLOCKED;
    Erc erc;
    uid_t uid = H(address);
    message.put(EB_DST,uid);
    taskENTER_CRITICAL(  &mux);
    erc = _queue.put(message);
    taskEXIT_CRITICAL( &mux );
//    INFO("OUT %s",__func__);
    if ( erc ) ERROR(" cannot publish to eventbus !!");
    return E_OK;
}

Erc EventBus::publish(EventLabel label)
{
    Message msg(10);
    INFO(" publish event : '%s'",label);
    return publish(label,msg);
}

Message _rxd(256);

void EventBus::eventLoop()
{
    static portMUX_TYPE mux= portMUX_INITIALIZER_UNLOCKED;
    Erc erc;
    Consumer* consumer;
    uid_t uidDst;
    taskENTER_CRITICAL(&mux  );
    erc = _queue.get(_rxd);
    taskEXIT_CRITICAL(&mux  );

    if ( erc ==0  && _rxd.get(EB_DST,uidDst) ) {
        for(consumer=Consumer::first(); consumer; consumer=consumer->next()) {
            if ( uidDst == consumer->_eventUid) {
                consumer->_handler(_rxd);
            }
        }
    }
}
