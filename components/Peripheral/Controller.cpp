#include "Controller.h"


Controller::Controller(const char* name)  : VerticleCoRoutine(name),
    _led_right(23),
    _led_left(32),
    _pot_left(36),
    _pot_right(39),
    _leftSwitch(DigitalIn::create(13)),
    _rightSwitch(DigitalIn::create(16))
{
}

Controller::~Controller()
{
}


void Controller::start()
{
    _leftSwitch.setMode(DigitalIn::DIN_PULL_UP);
    _rightSwitch.setMode(DigitalIn::DIN_PULL_UP);

    _led_right.init();
    _led_left.init();
    _led_left.off();
    _pot_left.init();
    _pot_right.init();
    _leftSwitch.init();
    _rightSwitch.init();

    new PropertyReference<uint32_t> ("controller/potLeft",_potLeft,1000);
    new PropertyReference<uint32_t> ("controller/potRight",_potRight,1000);
    new PropertyFunction<bool>("controller/buttonLeft",[this]() {
        return _leftSwitch.read()==1?false:true;
    },1000);
    new PropertyFunction<bool>("controller/buttonRight",[this]() {
        return _rightSwitch.read()==1?false:true;
    },1000);

    new PropertyWriteFunction<bool>("controller/ledLeft",[this]() {
        return _led_left.isOn();
    },[this](bool b) {
        if ( b ) _led_left.on();
        else _led_left.off();
        return E_OK;
    },1000);

    new PropertyWriteFunction<bool>("controller/ledRight",[this]() {
        return _led_right.isOn();
    },[this](bool b) {
        if ( b ) _led_right.on();
        else _led_right.off();
        return E_OK;
    },1000);
}

void Controller::run()
{
    PT_BEGIN();
    while(true) {
        PT_WAIT_SIGNAL(10);
        _potLeft = _potLeft + ( _pot_left.getValue() - _potLeft ) / 2;
        _potRight = _potRight + ( _pot_right.getValue() - _potRight )/2;
 /*       INFO(" %d:%d %d:%d %d:%d ",
             _potLeft,
             _potRight,
             _leftSwitch.read(),
             _rightSwitch.read(),
             _led_left.isOn(),
             _led_right.isOn());
        if ( _potLeft > 512 ) _led_left.on();
        if ( _potRight > 512 ) _led_right.on();
        if ( _potLeft <= 512 ) _led_left.off();
        if ( _potRight <= 512 ) _led_right.off();*/

    }
    PT_END();
}
