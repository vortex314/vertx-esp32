#ifndef MOTORSERVO_H
#define MOTORSERVO_H

#include <vertx.h>
#include <BTS7960.h>

class MotorServo : VerticleCoRoutine
{
    BTS7960 _bts7960;
 /*   float _target;
    float _direction;
    float _voltage;
    float _max,_min;*/
public:
    MotorServo(const char* name,Connector& connector);
    ~MotorServo();
    void start();
    void run();
    void calcTarget(float);
};

#endif // MOTORSERVO_H
