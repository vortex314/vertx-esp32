#ifndef PWM_H
#define PWM_H
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#include <Log.h>
#include <Sys.h>
#include <vertx.h>
#include <Hardware.h>

class Pwm : public VerticleCoRoutine
{
  uint32_t _pinPwmLeft;
  uint32_t _pinPwmRight;
  DigitalOut& _pinEnable;
  DigitalIn& _pinOverCurrent; 
  mcpwm_unit_t _mcpwm_num;
  mcpwm_timer_t _timer_num;
  double _duty_cycle;
    
public:
    Pwm(const char* name,uint32_t pinPwmLeft,uint32_t pinPwmRight,uint32_t pinEnable,uint32_t pinOverCurrent);
    ~Pwm();
    void start();
    void run();
    void left(float pwm);
    void right(float pwm);
    void stopMotor();

};

#endif // PWM_H
