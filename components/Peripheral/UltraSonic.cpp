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
    new PropertyFunction<int32_t>("ultraSonic/distance",[this]()  {
        return _distance ;
    },1000);
/*    new PropertyFunction<int32_t>("ultraSonic/microSeconds",[this]()  {
        return _delay;
    },1000);*/

    VerticleCoRoutine::start();
}
void UltraSonic::run()
{
    PT_BEGIN();
    while(true) {
        PT_WAIT_SIGNAL(100);
        int cm = _hcsr.getCentimeters();

        if (cm < 400 && cm > 0) {
            _distance = _distance + ( cm- _distance  )/2;
            _delay = _delay +( _hcsr.getTime() - _delay)/2;
/*            INFO("distance : %d micro-sec = %d cm ", _delay,
                 _distance);*/
        }
        _hcsr.trigger();
    }
    PT_END();
}
