#include "Pwm.h"



Pwm::Pwm(const char* name,
         uint32_t pinPwmLeft,
         uint32_t pinPwmRight,
         uint32_t pinEnable,
         uint32_t pinOverCurrent,
         uint32_t pinTacho) :
    VerticleCoRoutine(name),
    _pinPwmLeft(pinPwmLeft),
    _pinPwmRight(pinPwmRight),
    _pinEnable (DigitalOut::create(pinEnable)),
    _pinOverCurrent (DigitalIn::create(pinOverCurrent)),
    _pinTacho(pinTacho)
{

}

Pwm::~Pwm()
{
}

static mcpwm_dev_t *MCPWM[2] = {&MCPWM0, &MCPWM1};



void Pwm::start()
{
    _pinEnable.init();
    _pinEnable.write(0);
    _pinOverCurrent.init();
    _timer_num = MCPWM_TIMER_0;
    _mcpwm_num = MCPWM_UNIT_0;
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, _pinPwmLeft);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, _pinPwmRight);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, _pinTacho);
    INFO("Configuring Initial Parameters of mcpwm...\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 2000;    //frequency = 500Hz,
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
//    pwm_config.mcpwm_cap0_in_num   = _pinTacho;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
//    VerticleCoRoutine::start();
    mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_POS_EDGE, 0);  //capture signal on rising edge, prescale = 0 i.e. 800,000,000 counts is equal to one second
    MCPWM[MCPWM_UNIT_0]->int_ena.val = CAP0_INT_EN ;  //Enable interrupt on  CAP0, CAP1 and CAP2 signal
    mcpwm_isr_register(MCPWM_UNIT_0, isrHandler, this, ESP_INTR_FLAG_IRAM, NULL);  //Set ISR Handler
    new PropertyReference<float> ("drive/rpmTarget",_rpmTarget,1000);
    new PropertyReference<float> ("drive/rpmMeasured",_rpmMeasured,1000);
    new PropertyReference<uint32_t> ("drive/delta",_delta,1000);
    new PropertyReference<float> ("drive/KP",_KP,1000);
    new PropertyReference<float> ("drive/KI",_KI,1000);
    new PropertyReference<float> ("drive/KD",_KD,1000);
}

/**
 * @brief motor moves in forward direction, with duty cycle = duty %
 */
void Pwm::left(float duty_cycle)
{
    mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_B);
    mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0); //call this each time, if operator was previously in low/high state
}

/**
 * @brief motor moves in backward direction, with duty cycle = duty %
 */
void Pwm::right( float duty_cycle)
{
    if ( duty_cycle > 30 ) duty_cycle=30;
    if ( duty_cycle < 0 ) duty_cycle=0;
    mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_A);
    mcpwm_set_duty(_mcpwm_num, _timer_num, MCPWM_OPR_B, duty_cycle);
    mcpwm_set_duty_type(_mcpwm_num, _timer_num, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);  //call this each time, if operator was previously in low/high state
}

/**
 * @brief motor stop
 */
void Pwm::stopMotor()
{
    mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_A);
    mcpwm_set_signal_low(_mcpwm_num, _timer_num, MCPWM_OPR_B);
}

float Pwm::PID(float err)
{
    _integral = _integral + (err*_iteration_time);
    _derivative = (err - _errorPrior)/_iteration_time;
    _output = _KP*err + _KI*_integral + _KD*_derivative +_bias;
    INFO(" error : %f => %f P=%f : I=%f : D=%f ",err,_output,_KP*err , _KI*_integral , _KD*_derivative);
    _errorPrior = err;
    return _output;
}

void IRAM_ATTR Pwm::isrHandler(void* pv)
{
    Pwm* pwm=(Pwm*)pv;
    uint32_t mcpwm_intr_status;

    mcpwm_intr_status = MCPWM[MCPWM_UNIT_0]->int_st.val; //Read interrupt status
    if (mcpwm_intr_status & CAP0_INT_EN) { //Check for interrupt on rising edge on CAP0 signa
        pwm->_prevCapture = pwm->_capture;
        uint32_t capt = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0); //get capture signal counter value
        if ( (capt-pwm->_prevCapture) > 10000 ) {
            pwm->_capture = capt;
            pwm->_delta = pwm->_capture - pwm->_prevCapture;

        }
    }

    MCPWM[MCPWM_UNIT_0]->int_clr.val = mcpwm_intr_status;
}

void Pwm::run()
{
    static float lastDelta;
    PT_BEGIN();
    PT_WAIT_SIGNAL(3000);
    _pinEnable.write(1);
    right(20.0);
    INFO(" freq APB %u ",rtc_clk_apb_freq_get());
    while(true) {

//       INFO(" delta : %u : %u : %u : %u  ",_delta,_capture,_prevCapture ,_capture-_prevCapture);
        /*       _delta = (_delta / 10000) * (10000000000 / rtc_clk_apb_freq_get());
               if ( _delta==0 ) {
                   _rpmMeasured=0;
                   _integral=0;
               } else {
                   _rpmMeasured = 60000000/(20*_delta);
               }
               _error = _rpmTarget;
               _error -= _rpmMeasured;

               if ( _error > 0 ) dutyCycle++;
               if ( _error < 0 ) dutyCycle--;
               INFO(" duty cycle : %f ",dutyCycle);
               right(dutyCycle);
        //        right(PID(_error));*/
        PT_WAIT_SIGNAL(1);
        if ( lastDelta != _delta ) {
            _rpmMeasured =  80000000.0*60.0;
            _rpmMeasured /= _delta;
            _iteration_time =  60.0/_rpmMeasured ;
            INFO("%u clicks, %f rpm, %f sec %f error %f : P=%f I=%f ",
            _delta,
            _rpmMeasured,
            _iteration_time,
            _error,
            _output,
            _error*_KP,
            _integral*_KI);

        };
        _error = _rpmTarget-_rpmMeasured;
        _output = PID(_error);
        right(_output);
        lastDelta=_delta;
//       right(15.0);
        /*       INFO(" capture : %u rpmTarget %u rpmMeasured %u error : %f output : %f I:%f D:%f  ",
                    _capture,
                    _rpmTarget,
                    _rpmMeasured,
                    _error,
                    _output,
                    _integral,
                    _derivative);*/

        /*        if ( _rpmMeasured == _rpmLastMeasured) _rpmMeasured=0;
                _rpmLastMeasured = _rpmMeasured;*/
    };
    PT_END();
}
