

#include <vertx.h>



CoRoutineTask::CoRoutineTask(const char *name)
    : VerticleTask(name, 3000, 1) // prio 1 to be at same level as IDLEtask, that resets watchdog
{
}
void CoRoutineTask::start()
{
//    INFO(" %s ",__func__);
    VerticleTask::start();
}
void CoRoutineTask::run()
{
    while (true) {

        eb.eventLoop(); // handle incoming messages first
        for(Verticle* pv = Verticle::first(); pv; pv=pv->next()) {
            if ( !pv->isTask()) {
                VerticleCoRoutine* pvcr = (VerticleCoRoutine*)pv;
                if ( Sys::millis() >  pvcr->timeout() ) pvcr->signal(SIGNAL_TIMER); // set timeout if needed
                if ( pvcr->signal() ) {
                    pvcr->run();
                    pvcr->clearSignal();
                }
            }
        }
        vTaskDelay(1);
    }
}




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
        PT_WAIT_SIGNAL(10000);
        if ( hasSignal(SIGNAL_TIMER)) {
            INFO(" coroutine-%s running default.", name());
        } else {
            INFO(" I received a wakeup call !!");
        }
        PT_YIELD();
    }
    PT_END();
}

void VerticleCoRoutine::start()
{

}
void VerticleCoRoutine::stop()
{
}

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
