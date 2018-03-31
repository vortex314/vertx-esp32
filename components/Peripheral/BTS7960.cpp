#include "BTS7960.h"
#include <Log.h>

BTS7960::BTS7960(DigitalOut& left,DigitalOut& right,DigitalOut& enable,ADC& current,ADC& position) :
    _left(left),_right(right),_enable(enable),_current(current),_position(position)
{

}
BTS7960::BTS7960(Connector& connector) :
    _left(connector.getDigitalOut(LP_SCL)),
    _right(connector.getDigitalOut(LP_SDA)),
    _enable(connector.getDigitalOut(LP_TXD)),
    _current(connector.getADC(LP_SCK)),
    _position(connector.getADC(LP_RXD))
{

}

void BTS7960::init()
{
    _left.init();
    _right.init();
    _enable.init();
    _position.init();
    _current.init();
    _left.write(0);
    _right.write(0);
    _enable.write(0);
    _max=0.6;
    _min=0.3;
    _angleTarget=0.0;

}

float absolute(float f)
{
    if (f > 0) return f;
    return -f;
}

float BTS7960::getAngle()
{
    return (_position.getValue()/4096.0)*90.0;
}


BTS7960::~BTS7960()
{
}

void BTS7960::motorLeft()
{
    INFO("%s",__func__);
    _left.write(1);
    _right.write(0);
    _enable.write(1);
}

void BTS7960::motorRight()
{
    INFO("%s",__func__);
    _left.write(0);
    _right.write(1);
    _enable.write(1);
}

void BTS7960::motorStop()
{
    INFO("%s",__func__);
    _left.write(1);
    _right.write(1);
    _enable.write(1);
}

void BTS7960::loop()
{
    float _angleCurrent=getAngle();
    INFO("angle %f target %f",_angleCurrent,_angleTarget);
    float delta = abs(_angleCurrent - _angleTarget);
    if (delta < 0.05) {
        motorStop();
    } else if (_angleCurrent < _angleTarget) {
        motorRight();
    } else if (_angleCurrent > _angleTarget) {
        motorLeft();
    }
}
