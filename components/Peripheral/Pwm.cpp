#include "Pwm.h"



Pwm::Pwm(const char* name,uint32_t pinPwmLeft,uint32_t pinPwmRight,uint32_t pinEnable,uint32_t pinOverCurrent) :
    VerticleCoRoutine(name),
    _pinPwmLeft(pinPwmLeft),
    _pinPwmRight(pinPwmRight),
    _pinEnable (DigitalOut::create(pinEnable)),
    _pinOverCurrent (DigitalIn::create(pinOverCurrent))
{

}

Pwm::~Pwm()
{
}

void Pwm::start()
{
    _pinEnable.init();
    _pinEnable.write(0);
    _pinOverCurrent.init();
    _timer_num = MCPWM_TIMER_0;
    _mcpwm_num = MCPWM_UNIT_0;
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, _pinPwmLeft);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, _pinPwmRight);
    INFO("Configuring Initial Parameters of mcpwm...\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 2000;    //frequency = 500Hz,
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
//    VerticleCoRoutine::start();
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

void Pwm::run(){
    float max=90,delta=1;
    PT_BEGIN();
    _pinEnable.write(1);
    INFO(" _pinEnable.write(1)");
     while (1) {
         for( _duty_cycle=0;_duty_cycle<max;_duty_cycle+=delta) {
             PT_WAIT_SIGNAL(100 );
             left( _duty_cycle);
             INFO(" duty : %f , overCurrent : %d ",_duty_cycle,_pinOverCurrent.read());
         }
          for( _duty_cycle=0;_duty_cycle<max;_duty_cycle+=delta) {
             PT_WAIT_SIGNAL(100 );
             left( max-_duty_cycle);
             INFO(" duty : %f , overCurrent : %d ",_duty_cycle,_pinOverCurrent.read());
         }
         for( _duty_cycle=0;_duty_cycle<max;_duty_cycle+=delta) {
             PT_WAIT_SIGNAL(100 );
             right( _duty_cycle);
             INFO(" duty : %f , overCurrent : %d ",_duty_cycle,_pinOverCurrent.read());
         }
         for( _duty_cycle=0;_duty_cycle<max;_duty_cycle+=delta) {
             PT_WAIT_SIGNAL(100 );
             right(  max-_duty_cycle);
             INFO(" duty : %f , overCurrent : %d ",_duty_cycle,_pinOverCurrent.read());
         }
        stopMotor();
        PT_WAIT_SIGNAL(1000 );
    }
    PT_END();
}
