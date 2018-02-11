#include <vertx.h>

VerticleTask::VerticleTask(const char *name, uint16_t stack, uint8_t priority)
{
    // _next = 0;
    _taskHandle=0;
    _name = new char[strlen(name) + 1];
    strcpy(_name, name);
    _stackSize = stack;
    _priority = priority;

    INFO(" name : %s : %X ", _name, this);
    _nextEvent = 8;
    INFO(" VerticleTask : %s = 0x%X", _name, this);
    add(this);
    // LinkedList<Verticle>::add(this);
    xTimerCreate("mqtt",1000,pdTRUE,this,timerHandler);
};

void VerticleTask::run()
{
    while (true) {
        DEBUG(" default  run()   : %s : %X ", _name, this);
        waitSignal(1000000);
    }
}

void VerticleTask::handler(void *p)
{
    DEBUG(" running %X ",p);
    VerticleTask *pV = (VerticleTask *)p;
    pV->run();
}

void VerticleTask::start()
{

    /*    TimerHandle_t timer = xTimerCreate("LED", 10000 / portTICK_PERIOD_MS, pdTRUE, (void *)0, timerHandler);
    vTimerSetTimerID(timer, this);
    xTimerStart(timer, 0);*/
    if (xTaskCreate(&handler, _name, _stackSize, this, _priority, &_taskHandle)==pdPASS) {
        INFO(" Task created %s : %X ",_name,_taskHandle);
    } else {
        ERROR(" Task creation failed  %s : %X ",_name,_taskHandle);
    }
}

void VerticleTask::stop()
{
    vTaskSuspend( _taskHandle );
}
/*
void VerticleTask::onMessage(Cbor &msg)
{
    signal(SIGNAL_MESSAGE);
    INFO(" received message in %s , default handler invoked.", _name);
} */

uint32_t VerticleTask::newEvent()
{
    _nextEvent <<= 1;
    return _nextEvent;
}

void VerticleTask::signal(uint32_t n)
{
    if ( _taskHandle )
        xTaskNotify(_taskHandle, 1<<n, eSetBits);
}

 void VerticleTask::signalFromIsr(uint32_t n)
{
    BaseType_t xHigherPriorityTaskWoken;
    if ( _taskHandle ) {
        xTaskNotifyFromISR(_taskHandle, 1<<n, eSetBits,&xHigherPriorityTaskWoken);
 //       portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
         portYIELD_FROM_ISR();
    }
}


uint32_t VerticleTask::waitSignal(uint32_t time)
{
    xTaskNotifyWait(0x00, UINT32_MAX, &_lastNotify, time/portTICK_PERIOD_MS);
    if (_lastNotify) {
        DEBUG(" %s : notification received %d", _name, _lastNotify);
    } else {
        DEBUG(" %s : timeout wait() ",_name);
    }
    return _lastNotify;
}

bool VerticleTask::hasSignal(uint32_t sig)
{
    return ( _lastNotify & (1<<(sig)));
}



void VerticleTask::timerHandler(TimerHandle_t th)
{
    VerticleTask *pv = (VerticleTask *)pvTimerGetTimerID(th);
    pv->signal(SIGNAL_TIMER);
    DEBUG(" TIMER FIRED ");
}

const char *VerticleTask::name()
{
    return _name;
}

void VerticleTask::print()
{
    INFO(" name : %s , worker : %d, stack : %d, prio : %d",_name,isTask(),_stackSize,_priority);
}

TaskHandle_t VerticleTask::getHandle()
{
    return _taskHandle;;
}
