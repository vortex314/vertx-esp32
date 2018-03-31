#include "MotorServo.h"
#include <Property.h>

MotorServo::MotorServo(const char* name,Connector& connector) : VerticleCoRoutine(name),_bts7960(connector)
{
}

MotorServo::~MotorServo()
{
}

void MotorServo::start()
{
    UID.add("cm");
    UID.add("distance");

    new PropertyFunction<int32_t>("servo/angleTarget",[this]()  {
        return _bts7960.getAngle();
    },1000);
}



void MotorServo::run()
{
    PT_BEGIN();
    while(true) {
        PT_WAIT_SIGNAL(100);
        _bts7960.loop();
    }
    PT_END();
}
