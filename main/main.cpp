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
#include <Property.h>
#include <Compass.h>
#include <UltraSonic.h>
#include <MotorServo.h>

// esp_err_t event_handler(void *ctx, system_event_t *event) { return ESP_OK; }


#include "driver/spi_master.h"
#include "esp_system.h"

class CoRoutineTask : public VerticleTask
{

public:
    CoRoutineTask(const char *name)
        : VerticleTask(name, 3000, 1) // prio 1 to be at same level as IDLEtask, that resets watchdog
    {
    }
    void start()
    {
        INFO(" %s ",__func__);
        VerticleTask::start();
    }
    void run()
    {
        while (true) {

            eb.eventLoop(); // handle incoming messages first
            for(Verticle* pv = Verticle::first(); pv; pv=pv->next()) {
                if ( !pv->isTask()) {
                    VerticleCoRoutine* pvcr = (VerticleCoRoutine*)pv;
                    if ( Sys::millis() >  pvcr->timeout() ) pvcr->signal(SIGNAL_TIMER); // set timeout if needed
                    if ( pvcr->signal() ) {
                        pvcr->run();
                        pvcr->clearSignal();
                    }
                }
            }
            taskYIELD();
        }
    }
};

EventBus eb(1024);
Log logger(256);
Wifi wifi("wifi");
LedBlinker led("led1",DigitalOut::create(2));
CoRoutineTask coRoutines("CoRout");
Monitor monitor("monitor");
Telnet telnet("telnet");
Mdns mdns("mdns");
Mqtt mqtt("mqtt");
PropertyVerticle prop("property");
/*Connector uext2(2);
MotorServo motorServo("servo",uext2);*/
/*Connector uext1(1);
Compass compass("compass",uext1);
Connector uext3(3);
UltraSonic us("ultraSonic",uext3);*/
#include <Controller.h>
//Controller controller("controller");
class Tacho : public VerticleCoRoutine
{
    DigitalIn& _pin;
    uint64_t _lastMeasurement;
    uint32_t _delta;
    uint32_t _rpm;
public :
    Tacho() : VerticleCoRoutine("tacho"), _pin(DigitalIn::create(35))
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
        _pin.onChange(DigitalIn::DIN_RAISE,onRaise,this);
        _pin.init();
        new PropertyFunction<int32_t>("tacho/pin", [this]() {
            return _pin.read();
        },1000);
        new PropertyReference<uint32_t>("tacho/rpm",_rpm,1000);

        VerticleCoRoutine::start();
    }
    void run()
    {
        PT_BEGIN();
        while(true) {
            PT_WAIT_SIGNAL(100);
            INFO(" tacho pin : %d rpm : %d ",_pin.read(),_rpm);
            if ( (Sys::micros() - _lastMeasurement) > 1000000 ) _rpm=0;
        }
        PT_END();
    }
};

//Tacho tacho;

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
