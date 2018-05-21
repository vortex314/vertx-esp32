#include "MotorSpeed.h"
#include <Property.h>


MotorSpeed::MotorSpeed(const char* name,
                       uint32_t pinLeftIS,
                       uint32_t pinRightIS,
                       uint32_t pinLeftEnable,
                       uint32_t pinRightEnable,
                       uint32_t pinLeftPwm,
                       uint32_t pinRightPwm,
                       uint32_t pinTachoA,
                       uint32_t pinTachoB ) :VerticleCoRoutine(name),
    _adcLeftIS(ADC::create(pinLeftIS)),
    _adcRightIS(ADC::create(pinRightIS)),
    _pinLeftEnable(DigitalOut::create(pinLeftEnable)),
    _pinRightEnable(DigitalOut::create(pinRightEnable)),
    _pinPwmLeft(pinLeftPwm),
    _pinPwmRight(pinRightPwm),
    _pinTachoA(pinTachoA),
    _pinTachoB(pinTachoB)
{
}

MotorSpeed::~MotorSpeed()
{
}

void MotorSpeed::start()
{
    UID.add("cm");
    UID.add("distance");
    for(uint32_t i=0; i<MAX_SAMPLES; i++)
        _samples[i]=0;
    _adcLeftIS.init();
    _adcRightIS.init();
    _pinLeftEnable.init();
    _pinLeftEnable.write(0);
    _pinRightEnable.init();
    _pinRightEnable.write(0);
    _timer_num = MCPWM_TIMER_0;
    _mcpwm_num = MCPWM_UNIT_0;
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, _pinPwmLeft);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, _pinPwmRight);
//    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, _pinTacho);
    INFO("Configuring Initial Parameters of mcpwm... on %d , %d ",_pinPwmLeft,_pinPwmRight);
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 2000;    //frequency = 500Hz,
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
//    pwm_config.mcpwm_cap0_in_num   = _pinTacho;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
    right(10);
//   _bts7960.init();
    new PropertyReference<float> ("motor/rpmTarget",_rpmTarget,1000);
    new PropertyReference<float> ("motor/rpmMeasured",_rpmMeasured,1000);
    new PropertyReference<float> ("motor/rpmFiltered",_rpmFiltered,1000);
    new PropertyReference<float> ("motor/currentLeft",_currentLeft,1000);
    new PropertyReference<float> ("motor/currentRight",_currentRight,1000);


    new PropertyReference<float> ("motor/KP",_KP,1000);
    new PropertyReference<float> ("motor/KI",_KI,1000);
    new PropertyReference<float> ("motor/KD",_KD,1000);
}

void MotorSpeed::left(float duty_cycle)
{
    if ( duty_cycle > 90 ) duty_cycle=90;
    mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_B);
    mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0); //call this each time, if operator was previously in low/high state
}

/**
 * @brief motor moves in backward direction, with duty cycle = duty %
 */
void MotorSpeed::right( float duty_cycle)
{
    if ( duty_cycle > 90 ) duty_cycle=90;
    mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_A);
    mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_B, duty_cycle);
    mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);  //call this each time, if operator was previously in low/high state
}



float MotorSpeed::PID(float err)
{
    _integral = _integral + (err*_iteration_time);
    _derivative = (err - _errorPrior)/_iteration_time;
    _output = _KP*err + _KI*_integral + _KD*_derivative +_bias;
    _errorPrior = err;
    return _output;
}


void MotorSpeed::setOutput(float output)
{
    static float lastOutput=0;
    if (( output < 0 && lastOutput >0 ) || ( output > 0 && lastOutput < 0 )) {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        left(0);
        right(0);
    } else if ( output < -10 ) {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        left(-output);
    } else if ( output > 10 ) {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        right(output);
    } else {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        left(0);
        right(0);
    }
    lastOutput =  output;
}

float MotorSpeed::filter(float f)
{
    float result;
    _samples[(_indexSample++) % MAX_SAMPLES]=f;
    result=0;
    for(uint32_t i=0; i<MAX_SAMPLES; i++) {
        result += _samples[i];
    }
    result /= MAX_SAMPLES;
    return result;
}


void MotorSpeed::round(float& f,float resolution)
{
    int i =  f / resolution;
    f = i;
    f *= resolution;
}



void MotorSpeed::run()
{
    static uint32_t loopCount=0;
    PT_BEGIN();
    _pinLeftEnable.write(1);
    _pinRightEnable.write(1);
    while(true) {
        PT_WAIT_SIGNAL(100);
        /*        _pinEnable.write(0);
                PT_WAIT_SIGNAL(10);*/
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        _currentLeft = (_adcLeftIS.getValue()*3.9 / 1024.0)*0.85;
        _currentRight =  (_adcRightIS.getValue()*3.9 / 1024.0)*0.85;
        round(_currentLeft,0.1);
        round(_currentRight,0.1);
        _rpmFiltered =  filter(_rpmMeasured);
        _error = _rpmTarget - _rpmFiltered;
        if ( _error <1 && _error >-1) _error=0;

        setOutput(5);


        INFO(" %f/%f/%f angle, %f/%f V,  %f sec, %f error, %f == P=%f + I=%f + D=%f",
             _rpmMeasured,
             _rpmFiltered,
             _rpmTarget,
             _currentLeft,
             _currentRight,
             _iteration_time,
             _error,
             _output,
             _error*_KP,
             _integral*_KI,
             _derivative*_KD);

        loopCount++;


//        _bts7960.loop();
    }
    PT_END();
}
