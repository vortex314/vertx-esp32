#ifndef LEDBLINKER_H
#define LEDBLINKER_H

#include <vertx.h>
#include <Hardware.h>

class LedBlinker : public VerticleCoRoutine
{
    DigitalOut& _ledGpio;
    uint32_t _interval=100;

public:
    LedBlinker(const char *name,DigitalOut& ledGpio);
    void run();
    void setup();
    void setInterval(uint32_t);
};
#endif // LEDBLINKER_H
