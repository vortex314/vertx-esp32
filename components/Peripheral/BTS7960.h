#ifndef BTS7960_H
#define BTS7960_H
#include <Hardware.h>

class BTS7960
{
    DigitalOut& _left;
    DigitalOut& _right;
    DigitalOut& _enable;
    ADC& _current;
    ADC& _position;
    float _voltage,_target,_min,_max,_zero;
    float _angleTarget;
    float _angleCurrent;
public:
    BTS7960(Connector& conn);
    BTS7960(DigitalOut& left,DigitalOut& right,DigitalOut& enable,ADC& current,ADC& position);
    ~BTS7960();
    void init();
    void loop();
    void calcTarget(float v);
    float getPosition();
    void motorStop();
    void motorLeft();
    void motorRight();
};

#endif // BTS7960_H
