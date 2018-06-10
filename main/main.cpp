#include <stdio.h>
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

#include <Config.h>
#include <Wifi.h>
#include <Log.h>
#include <LedBlinker.h>
#include <Monitor.h>
#include <Telnet.h>
#include <Mdns.h>
#include <Mqtt.h>
#include <MqttSerial.h>
#include <Property.h>
#include <Compass.h>
#include <UltraSonic.h>
#include <MotorServo.h>


// esp_err_t event_handler(void *ctx, system_event_t *event) { return ESP_OK; }


#include "driver/spi_master.h"
#include "esp_system.h"


EventBus eb(1024);
Log logger(256);


LedBlinker led("led1",DigitalOut::create(2));
Monitor monitor("monitor");
#ifdef WIFI
Wifi wifi("wifi");
Telnet telnet("telnet");
Mdns mdns("mdns");
Mqtt mqtt("mqtt");
#else
MqttSerial mqtt("mqttSerial");
#endif
CoRoutineTask coRoutines("coRout"); // VerticleTask to handle all VerticleCoRoutine 
PropertyVerticle prop("property");
// Connector uext2(2);
// D34 : L_IS
// D35 : R_IS
// D25 : ENABLE
// D26 : L_PWM
// D27 : R_PWM
// D32 : ADC POT
// MotorServo motorServo("servo",34,35,25,26,27,32);
// prototype board based on kicad
// GPIO36 : L_IS
// GPIO39 : R_IS
// GPIO32 : L_PWM was L_EN
// GPIO33 : R_PWM was R_EN
// GPIO25 : L_EN was L_PWM
// GPIO26 : R_EN was R_PWM
// GPIO34 : channel A of rotary sensor
// GPIO35 : channel B of rotary sensor
/*Connector uext1(1);
Compass compass("compass",uext1);
Connector uext3(3);
UltraSonic us("ultraSonic",uext3);*/
//#include <Controller.h>
//Controller controller("controller");
#include <MotorSpeed.h>
MotorSpeed motorSpeed("speed",36,39,25,26,32,33,34,35);

class Tacho : public VerticleCoRoutine
{
    DigitalIn& _pinTacho;
    DigitalOut& _pinPwmLeft;
    DigitalOut& _pinPwmRight;
    uint64_t _lastMeasurement;
    uint32_t _delta;
    uint32_t _rpm;
public :
    Tacho() : VerticleCoRoutine("tacho"),
        _pinTacho(DigitalIn::create(35)),
        _pinPwmLeft(DigitalOut::create(27)),
        _pinPwmRight(DigitalOut::create(26))
    {
    }
    void calcDelta(uint64_t t)
    {
        _delta = t-_lastMeasurement;
        _lastMeasurement = t;
        _rpm = ( 1000000*60  / _delta );
    }
    static void onRaise(void *p)
    {
        uint64_t t =  Sys::micros();
        Tacho* tacho=(Tacho*)p;
        tacho->calcDelta(t);
    }
    void start()
    {
        _pinTacho.onChange(DigitalIn::DIN_RAISE,onRaise,this);
        _pinTacho.init();
        _pinPwmLeft.init();
        _pinPwmRight.init();
        _pinPwmLeft.write(1);
        _pinPwmRight.write(0);
        new PropertyFunction<int32_t>("tacho/pin", [this]() {
            return _pinTacho.read();
        },1000);
        new PropertyReference<uint32_t>("tacho/rpm",_rpm,1000);

        VerticleCoRoutine::start();
    }
    void run()
    {
        PT_BEGIN();
        while(true) {
            PT_WAIT_SIGNAL(100);
            INFO(" tacho pin : %d rpm : %d ",_pinTacho.read(),_rpm);
            if ( (Sys::micros() - _lastMeasurement) > 1000000 ) _rpm=0;
        }
        PT_END();
    }
};

//Tacho tacho;

#include <Pwm.h>

// D34 : L_IS
// D35 : R_IS
// D25 : ENABLE
// D26 : L_PWM
// D27 : R_PWM
//Pwm pwm("pwm",26,27,25,36,35);

extern "C" void app_main()
{
//       config.clear();
    nvs_flash_init();

    config.load();
    config.setNameSpace("system");

    Str hn(20);
    hn ="ESP-";
    uint64_t serial=Sys::getSerialId();
    hn.appendHex((uint8_t*)&serial,6,0);
    config.get("host",hn,hn.c_str());
    INFO(" host : %s",hn.c_str());
    Sys::hostname(hn.c_str());

    eb.on("mqtt/connected", [](Message& evt) {
        led.setInterval(1000);
    });
    eb.on("mqtt/disconnected", [](Message& evt) {
        led.setInterval(100);
    });

    Verticle* pv;
    for(pv=Verticle::first(); pv; pv=pv->next())
        pv->start();

    config.save();
}
