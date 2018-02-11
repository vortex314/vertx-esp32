#include "LedBlinker.h"


LedBlinker::LedBlinker(const char *name,DigitalOut& ledGpio)
    : VerticleCoRoutine(name),_ledGpio(ledGpio)
{
    _interval=100;
}

void LedBlinker::run()
{
    PT_BEGIN();
    _ledGpio.init();

    while (true) {
        PT_WAIT_SIGNAL(_interval);
        _ledGpio.write(1);

        PT_WAIT_SIGNAL(_interval);
        _ledGpio.write(0);

    }
    PT_END();
}

void LedBlinker::setup()
{
    _ledGpio.init();
}

void LedBlinker::setInterval(uint32_t interval)
{
    INFO(" %s  => interval : %d ",name(),interval);
    _interval=interval;
}
