#ifndef PWM_H
#define PWM_H
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#include "soc/rtc.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#include <Log.h>
#include <Sys.h>
#include <vertx.h>
#include <Hardware.h>
#include <Property.h>

#define CAP0_INT_EN BIT(27)  //Capture 0 interrupt bit
#define CAP1_INT_EN BIT(28)  //Capture 1 interrupt bit
#define CAP2_INT_EN BIT(29)  //Capture 2 interrupt bit


class Pwm : public VerticleCoRoutine
{
    uint32_t _pinPwmLeft;
    uint32_t _pinPwmRight;
    DigitalOut& _pinEnable;
    DigitalIn& _pinOverCurrent;
    uint32_t _pinTacho;
    mcpwm_unit_t _mcpwm_num;
    mcpwm_timer_t _timer_num;
//    double _duty_cycle;
    uint32_t _capture;
    uint32_t _prevCapture;
    uint32_t _delta;
    uint32_t _captureInterval;
    float _KP=0.1;
    float _KI=0.01;
    float _KD=0.01;
    float _bias=0;
    float _error=0;
    float _errorPrior=0;
    float _iteration_time=1;
    float _integral=0;
    float _derivative=0;
    float _output=0;
//    uint32_t _captureTarget=200000;
    float _rpmTarget=200;
    float _rpmMeasured=0;
    bool _interrupt=false;

public:
    Pwm(const char* name,
        uint32_t pinPwmLeft,
        uint32_t pinPwmRight,
        uint32_t pinEnable,
        uint32_t pinOverCurrent,
        uint32_t pinTacho);
    ~Pwm();
    void start();
    void run();
    void left(float pwm);
    void right(float pwm);
    void stopMotor();
    void static isrHandler(void*);
    float PID(float err);
};

#endif // PWM_H
