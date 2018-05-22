#include "MotorSpeed.h"
#include <Property.h>

#define MAX_PWM 40
/*
 * CAPTURE :
 * Rotary sensor generates 600 pulses per rotation
 * Capture generates an interrupt per 10 pulses
 * The capture counter works at 80 M pulses per second
 *
 * 10 rpm => 6000 pulses per min  => 600 isr per min => 10 isr per sec
 *
 *  delta_time = (capture / 80M)*10 => time in sec for 1 rotation
 *
 * rpm = (1/delta_time)*60
 *
 * rpm = (80M/capture)/10*60 = ( 80M/capture )*6
 *
 *
 *
 * */
 
 
 

static mcpwm_dev_t *MCPWM[2] = {&MCPWM0, &MCPWM1};

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
    _dInTachoB(DigitalIn::create(pinTachoB))
{
    _isrCounter=0;
    _rpmMeasuredFilter = new AverageFilter<float>();
}

MotorSpeed::~MotorSpeed()
{
}

void IRAM_ATTR MotorSpeed::isrHandler(void* pv)
{
    MotorSpeed* ms=(MotorSpeed*)pv;
    uint32_t mcpwm_intr_status;
    int b=ms->_dInTachoB.read();
    ms->_direction = (b==1) ? -1 : 1;

    mcpwm_intr_status = MCPWM[MCPWM_UNIT_0]->int_st.val; //Read interrupt status
    ms->_isrCounter++;
    if (mcpwm_intr_status & CAP0_INT_EN) { //Check for interrupt on rising edge on CAP0 signa
        ms->_prevCapture = ms->_capture;
        uint32_t capt = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0); //get capture signal counter value
        ms->_capture = capt;
        ms->_delta = ms->_capture - ms->_prevCapture;
        ms->signal(SIG_CAPTURED);
    }

    MCPWM[MCPWM_UNIT_0]->int_clr.val = mcpwm_intr_status;
}

void MotorSpeed::onRaise(void *pv)
{
    MotorSpeed* ms=(MotorSpeed*)pv;
    if ( gpio_get_level((gpio_num_t)ms->_pinTachoA)) {
        ms->_direction=1;
    } else {
        ms->_direction=-1;
    }
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
    _dInTachoB.init();
    _timer_num = MCPWM_TIMER_0;
    _mcpwm_num = MCPWM_UNIT_0;
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, _pinPwmLeft);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, _pinPwmRight);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, _pinTachoA);
    _dInTachoB.onChange(DigitalIn::DIN_RAISE,onRaise,this);
    INFO("Configuring Initial Parameters of mcpwm... on %d , %d ",_pinPwmLeft,_pinPwmRight);
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 2000;    //frequency = 500Hz,
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
//   _bts7960.init();
    esp_err_t rc=mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_POS_EDGE, 1);  //capture signal on rising edge, prescale = 0 i.e. 800,000,000 counts is equal to one second
    MCPWM[MCPWM_UNIT_0]->int_ena.val = CAP0_INT_EN ;  //Enable interrupt on  CAP0, CAP1 and CAP2 signal
    rc = mcpwm_isr_register(MCPWM_UNIT_0, isrHandler, this, ESP_INTR_FLAG_IRAM, NULL);  //Set ISR Handler
    if ( rc ) {
        WARN("mcpwm_capture_enable() : %d",rc);
    }
    new PropertyReference<float> ("motor/rpmTarget",_rpmTarget,1000);
    new PropertyReference<float> ("motor/rpmMeasured",_rpmMeasured,-1000);
    new PropertyReference<float> ("motor/rpmFiltered",_rpmFiltered,-1000);
    new PropertyReference<float> ("motor/currentLeft",_currentLeft,-1000);
    new PropertyReference<float> ("motor/currentRight",_currentRight,-1000);
    new PropertyReference<uint32_t> ("motor/delta",_delta,-1000);
    new PropertyReference<int> ("motor/direction",_direction,1000);


    new PropertyReference<float> ("motor/KP",_KP,1000);
    new PropertyReference<float> ("motor/KI",_KI,1000);
    new PropertyReference<float> ("motor/KD",_KD,1000);
}

void MotorSpeed::left(float duty_cycle)
{
    if ( duty_cycle > MAX_PWM ) duty_cycle=MAX_PWM;
    mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_B);
    mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0); //call this each time, if operator was previously in low/high state
}



/**
 * @brief motor moves in backward direction, with duty cycle = duty %
 */
void MotorSpeed::right( float duty_cycle)
{
    if ( duty_cycle > MAX_PWM ) duty_cycle=MAX_PWM;
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
    } else if ( output < 0 ) {
        _pinLeftEnable.write(1);
        _pinRightEnable.write(1);
        left(-output);
    } else if ( output > 0 ) {
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

    PT_BEGIN();
    _pinLeftEnable.write(1);
    _pinRightEnable.write(1);
    while(true) {
        PT_WAIT_SIGNAL(100);
        uint32_t sig= signal();

        if ( hasSignal( SIG_CAPTURED) ) {
            _rpmMeasured = (80000000.0 / _delta)*0.6;
        } else  {
            _rpmMeasured=0;
        }
        _rpmMeasured= _direction*_rpmMeasured;
        _rpmMeasured = _rpmMeasuredFilter->filter(_rpmMeasured);
        _rpmFiltered =  filter(_rpmMeasured);

        _currentLeft = (_adcLeftIS.getValue()*3.9 / 1024.0)*0.85;
        _currentRight =  (_adcRightIS.getValue()*3.9 / 1024.0)*0.85;

        round(_currentLeft,0.1);
        round(_currentRight,0.1);

        _error = _rpmTarget - _rpmMeasured;
        PID(_error);
        _output *= -1;
        if ( _currentLeft > 3.0 || _currentRight > 3.0 ) {
            _output= 0;
        }
        setOutput(_output);

        /*        INFO(" %d %f/%f rpm, %d ,%f/%f A,  %f sec, %f error, %f == P=%f + I=%f + D=%f",
                     _direction,
                     _rpmMeasured,
                     _rpmTarget,
                     _isrCounter,
                     _currentLeft,
                     _currentRight,
                     _iteration_time,
                     _error,
                     _output,
                     _error*_KP,
                     _integral*_KI,
                     _derivative*_KD);*/
    }
    PT_END();
}

