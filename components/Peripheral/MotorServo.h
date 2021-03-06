#ifndef MOTORSERVO_H
#define MOTORSERVO_H

#include <vertx.h>
#include <Hardware.h>

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#define MAX_SAMPLES 4

class MotorServo : VerticleCoRoutine
{

// D34 : L_IS
// D35 : R_IS
// D25 : ENABLE
// D26 : L_PWM
// D27 : R_PWM
// D32 : ADC POT
    ADC& _adcLeftIS;
    ADC& _adcRightIS;
    DigitalOut& _pinEnable;
    uint32_t _pinPwmLeft;
    uint32_t _pinPwmRight;
    ADC& _adcPot;
    mcpwm_unit_t _mcpwm_num;
    mcpwm_timer_t _timer_num;
    float _angleCurrent=0.0;
    float _angleTarget=20;
    float _KP=15;
    float _KI=0.005;
    float _KD=0;
    float _bias=0;
    float _error=0;
    float _errorPrior=0;
    float _iteration_time=1;
    float _integral=0;
    float _derivative=0;
    float _output=0;
    float _angleSamples[MAX_SAMPLES];
    uint32_t _indexSample=0;
    float _angleFiltered;
    float _currentLeft,_currentRight;
public:
    MotorServo(const char* name,Connector& connector);
    MotorServo(const char* name,
               uint32_t pinLeftIS,
               uint32_t pinrightIS,
               uint32_t pinEnable,
               uint32_t pinLeftPwm,
               uint32_t pinRightPwm,
               uint32_t pinPot);
    ~MotorServo();
    void start();
    void run();
    void calcTarget(float);
    float PID(float);
    void left(float);
    void right(float);
    void setOutput(float output);
    float filterAngle(float inp);
    void round(float& f,float resol);
};

#endif // MOTORSERVO_H
