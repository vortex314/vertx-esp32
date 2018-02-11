#include "UltraSonic.h"

#include "HCSR04.h"

UltraSonic::UltraSonic(const char* name, Connector& connector)
    : VerticleCoRoutine(name), _hcsr(connector)
{
}

UltraSonic::UltraSonic(const char* name, DigitalOut& pinTrigger, DigitalIn& pinEcho)
    : VerticleCoRoutine(name), _hcsr(pinTrigger, pinEcho)
{
}

void UltraSonic::start()
{
    UID.add("cm");
    UID.add("distance");
    _hcsr.init();
    new PropertyFunction<float>("ultraSonic/distance",[this]()  {
        return _hcsr.getCentimeters() / 100.0;
    },1000);
    new PropertyFunction<uint64_t>("ultraSonic/microSeconds",[this]()  {
        return _hcsr.getTime();
    },1000);

    VerticleCoRoutine::start();
}
void UltraSonic::run()
{
    PT_BEGIN();
    while(true) {
        PT_WAIT_SIGNAL(500);
        float cm = _hcsr.getCentimeters();
        if (cm < 400 && cm > 0) {
            INFO("distance : %lld µsec = %f m ", _hcsr.getTime(),
                 _hcsr.getCentimeters()/100.0);

        }
        _hcsr.trigger();
    }
    PT_END();
}
