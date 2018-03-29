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
    new PropertyReference<float>("target",_target, 1000);
    new PropertyReference<float>("direction",_direction, 1000);
    new PropertyReference<float>("voltage",_voltage,  1000);
}

void MotorServo::calcTarget(float v)
{
    if (v < -100.0) {
        v = -100.0;
    } else if (v > 100) {
        v = 100.0;
    };
    float v1 = (v + 100) / 200;  // 0->1.0
    _target = (v1 * (_max - _min)) + _min;
}

void MotorServo::run()
{
    PT_BEGIN();
    PT_WAIT_SIGNAL(10);

    _bts7960.loop();

    /* if ((_count % 100) == 0) {
       _direction += 10;
       if (_direction >= 150) _direction = -150;
       calcTarget(_direction);
     }*/
    PT_END();

}
