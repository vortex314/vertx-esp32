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
    _bts7960.init();

    new PropertyFunction<int32_t>("servo/angleCurrent",
    [this]()  {
        return _bts7960.getAngleCurrent();
    },1000);
    new PropertyWriteFunction<int32_t>("servo/angleTarget",
    [this]()  {
        return _bts7960.getAngleTarget();
    },
    [this](int32_t v)  {
        _bts7960.setAngleTarget(v);
        return E_OK;
    }
    ,1000);
}



void MotorServo::run()
{
    PT_BEGIN();
    while(true) {
        PT_WAIT_SIGNAL(10);
        _bts7960.loop();
    }
    PT_END();
}
