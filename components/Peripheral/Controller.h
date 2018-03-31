#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <vertx.h>
#include <Led.h>
#include <Pot.h>
#include <Property.h>

class Controller : public VerticleCoRoutine
{
    Led _led_right;
    Led _led_left;
    Pot _pot_left;
    Pot _pot_right;
    uint32_t _potLeft;
    uint32_t _potRight;
    DigitalIn& _leftSwitch;
    DigitalIn& _rightSwitch;
public:
    Controller(const char* name);
    ~Controller();
    void start();
    void run();

};

#endif // CONTROLLER_H
