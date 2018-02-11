

#include <vertx.h>

VerticleCoRoutine::VerticleCoRoutine(const char *name) : Verticle()
{
    _name = new char[strlen(name) + 1];
    strcpy(_name, name);
    add(this);
    __timeout=0;
}
const char *VerticleCoRoutine::name()
{
    return _name;
}

void VerticleCoRoutine::run()
{
    PT_BEGIN();
    for (;;) {
        PT_WAIT_SIGNAL(3600000);
        if ( hasSignal(SIGNAL_TIMER)) {
            INFO(" coroutine-%s running default.", name());
        } else {
            INFO(" I received a wakeup call !!");
        }
        PT_YIELD();
    }
    PT_END();
}
/*void VerticleCoRoutine::handler(CoRoutineHandle_t xHandle, UBaseType_t uxIndex)
{
    VerticleCoRoutine *pvc = (VerticleCoRoutine *)uxIndex;
    pvc->_xHandle = xHandle;
    pvc->run();
}

void VerticleCoRoutine::start()
{
    xCoRoutineCreate(handler, 0, (UBaseType_t)this);
}*/
void VerticleCoRoutine::start()
{

}
void VerticleCoRoutine::stop()
{
}
/*void VerticleCoRoutine::loop()
{
    Verticle *pv;
    for( pv = Verticle::first(); pv; pv=pv->next()) {
        if ( !pv->isTask()) {

            VerticleCoRoutine* pvcr = (VerticleCoRoutine*)pv;
            if ( Sys::millis() >  pvcr->__timeout ) pvcr->signal(SIGNAL_TIMER); // set timeout if needed
            if ( pvcr->_signal ) {
//                INFO(" calling %s : %X : %ld ",pvcr->name(),pvcr->_signal,pvcr->__timeout);
                pvcr->run();
            }
        }
    }
}*/

void VerticleCoRoutine::signal(uint32_t sign)
{
    _signal |= (1 << sign);
}

bool VerticleCoRoutine::hasSignal(uint32_t sign)
{
    return _signal & (1<<sign);
}

void VerticleCoRoutine::clearSignal()
{
    _signal=0;
}

bool VerticleCoRoutine::isTask()
{
    return false;
}
