#ifndef _VERTICLE_H_
#define _VERTICLE_H_

#include <memory>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/queue.h>
//#include <croutine.h>
//#include "esp/uart.h"
//#include "espressif/esp_common.h"

#include <Log.h>
#include <Uid.h>
#include <Cbor.h>
#include <LinkedList.hpp>
#include <Uid.h>
#include <CborQueue.h>

#define MS_TO_TICK(xx) (xx/portTICK_PERIOD_MS)

class Verticle;

class Verticle : public LinkedList<Verticle>
{
public:
    virtual const char *name()
    {
        return "ERRORE!";
    };
    virtual void start() =0;
    virtual void stop() =0;
    virtual void run()=0;
    virtual void signal(uint32_t n)=0;
    virtual bool isTask()
    {
        return false;
    };

    //    virtual void onTimer() = 0;
    //   virtual void onInterrupt() = 0;
};

#define SIGNAL_MESSAGE 31
#define SIGNAL_TIMER 30

class VerticleTask : public Verticle
{
    char *_name;
    uint16_t _stackSize;
    uint8_t _priority;
    uint32_t _nextEvent;
    uint32_t  _lastNotify;
public:
    TaskHandle_t _taskHandle;
    VerticleTask(const char *name,  uint16_t stack, uint8_t priority);
    const char *name();
    virtual void run();
    virtual void start();
    void stop();
    uint32_t newEvent();

    void signal(uint32_t  n);
    void signalFromIsr(uint32_t n);

    bool hasSignal(uint32_t n);
    uint32_t waitSignal(uint32_t time);
    bool isTask()
    {
        return true;
    };

    static void handler(void *p);

    static void timerHandler(TimerHandle_t th);
    void print();
    TaskHandle_t getHandle();
};

typedef void (Verticle::*MethodHandler)(Cbor &);
typedef void (*StaticHandler)(Cbor &);

class VerticleCoRoutine : public Verticle
{
    char *_name;

protected:
    unsigned short _ptLine=0;
    uint64_t __timeout;
    uint32_t _signal;

public:
    VerticleCoRoutine(const char *name) ;
    const char* name();
    virtual void run();
//    static void loop();
    void start();
    void stop();
    void signal(uint32_t s);
    bool hasSignal(uint32_t s);
    void clearSignal();
    bool isTask();
    inline void timeout(uint32_t t)
    {
        __timeout = Sys::millis()+t;
    }
    inline uint64_t timeout()
    {
        return __timeout;
    }
    inline uint32_t signal()
    {
        return _signal;
    }
};


// Declare start of protothread (use at start of Run() implementation).
#define PT_BEGIN() bool ptYielded = true; switch (_ptLine) { case 0:
// Stop protothread and end it (use at end of Run() implementation).
#define PT_END() default:  ;} stop(); return ;(void)ptYielded;
// Cause protothread to wait until given condition is true.
#define PT_WAIT_UNTIL(condition) \
    do { _ptLine = __LINE__; case __LINE__: \
        if (!(condition)) return ; } while (0)
#define PT_WAIT_SIGNAL(___time)  \
    do { _signal=0;timeout(___time);_ptLine = __LINE__; case __LINE__: \
        if (!(_signal)) return ; } while (0)
#define PT_WAIT(___time)  \
    do { _signal=0;timeout(___time);_ptLine = __LINE__; case __LINE__: \
        if (!hasSignal(SIGNAL_TIMER) )return ; } while (0)
// Cause protothread to wait while given condition is true.
#define PT_WAIT_WHILE(condition) PT_WAIT_UNTIL(!(condition))
// Cause protothread to wait until given child protothread completes.
#define PT_WAIT_THREAD(child) PT_WAIT_WHILE((child).dispatch(msg))
// Restart and spawn given child protothread and wait until it completes.
#define PT_SPAWN(child) \
    do { (child).restart(); PT_WAIT_THREAD(child); } while (0)
// Restart protothread's execution at its PT_BEGIN.
#define PT_RESTART() do { restart(); return ; } while (0)
// Stop and exit from protothread.
#define PT_EXIT() do { stop(); return ; } while (0)
// Yield protothread till next call to its Run().
#define PT_YIELD() \
    do { ptYielded = false; _ptLine = __LINE__; case __LINE__: \
        if (!ptYielded) return ; } while (0)
// Yield protothread until given condition is true.
#define PT_YIELD_UNTIL(condition) \
    do { ptYielded = false; _ptLine = __LINE__; case __LINE__: \
        if (!ptYielded || !(condition)) return ; } while (0)



class Message :  public Cbor
{

public:
    Message(uint32_t size):Cbor(size)
    {
    }
    template <typename T> bool get(uid_t uid,T& var)
    {
        return getKeyValue(uid,var);
    };
    template <typename T> Message& put(uid_t uid,T var)
    {
        addKeyValue(uid,var);
        return *this;
    }
    void clear()
    {
        Cbor::clear();
    }
};

typedef const char* EventLabel;
#define EventHandler std::function<void (Message&)>

const uid_t EB_DST = H("dst");
const uid_t EB_SRC = H("src");

class EventBus
{
    CborQueue _queue;
public:
    EventBus(uint32_t size);
    Erc publish(EventLabel);
    Erc publish(uid_t);
    Erc publish(EventLabel, Message &);
    Erc publish(uid_t,Message& );
    Erc send(EventLabel, Message &);
    Erc consumer(EventLabel, EventHandler );
    Erc localConsumer(EventLabel, EventHandler );
    Erc addInterceptor(EventHandler );
    Erc removeInterceptor(EventHandler );
    Erc on(EventLabel  address,EventHandler);
    void eventLoop();
};

extern EventBus eb;


#endif
