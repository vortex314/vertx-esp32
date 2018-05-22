#ifndef MOTORSPEED_H
#define MOTORSPEED_H

#include <vertx.h>
#include <Hardware.h>

#include <MedianFilter.hpp>

#include "driver/mcpwm.h"
#include "driver/pcnt.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"



#define MAX_SAMPLES 4
#define CAP0_INT_EN BIT(27)  //Capture 0 interrupt bit
#define CAP1_INT_EN BIT(28)  //Capture 1 interrupt bit
#define CAP2_INT_EN BIT(29)  //Capture 2 interrupt bit

typedef enum  {
    SIG_CAPTURED=2
} Signal;


class MotorSpeed : VerticleCoRoutine
{

// D34 : L_IS
// D35 : R_IS
// D25 : ENABLE
// D26 : L_PWM
// D27 : R_PWM
// D32 : ADC POT
    ADC& _adcLeftIS;
    ADC& _adcRightIS;
    DigitalOut& _pinLeftEnable;
    DigitalOut& _pinRightEnable;
    uint32_t _pinPwmLeft;
    uint32_t _pinPwmRight;
    uint32_t _pinTachoA;
    DigitalIn& _dInTachoB;
    uint32_t _pinTachoB;
    mcpwm_unit_t _mcpwm_num;
    mcpwm_timer_t _timer_num;
    pcnt_unit_t pcnt_unit;
    pcnt_channel_t pcnt_channel;
    pcnt_config_t pcnt_config;
    uint32_t _capture;
    uint32_t _prevCapture;
    uint32_t _delta;
    uint32_t _captureInterval;
    uint32_t _isrCounter;
    int _direction=1;
    float _rpmMeasured;
    float _rpmTarget=100;
    float _rpmFiltered;
    float _KP=0.08;
    float _KI=0.005;
    float _KD=-0.03;
    float _bias=0;
    float _error=0;
    float _errorPrior=0;
    float _iteration_time=1;
    float _integral=0;
    float _derivative=0;
    float _output=0;
    float _samples[MAX_SAMPLES];
    uint32_t _indexSample=0;
    float _angleFiltered;
    float _currentLeft,_currentRight;
    AverageFilter<float>* _rpmMeasuredFilter;
public:
    MotorSpeed(const char* name,Connector& connector);
    MotorSpeed(const char* name,
               uint32_t pinLeftIS,
               uint32_t pinrightIS,
               uint32_t pinLeftEnable,
               uint32_t pinRightEnable,
               uint32_t pinLeftPwm,
               uint32_t pinRightPwm,
               uint32_t pinTachoA,
               uint32_t pinTachoB );
    ~MotorSpeed();
    static void onRaise(void*);
    static void isrHandler(void*);
    void start();
    void run();
    void calcTarget(float);
    float PID(float);
    void left(float);
    void right(float);
    void setOutput(float output);
    float filter(float inp);
    void round(float& f,float resol);
};

#endif // MOTORSPEED_H
