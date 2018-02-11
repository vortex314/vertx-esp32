#include "Mdns.h"
#include <mdns.h>
extern "C" {
//#include <mdnsresponder.h>
}
enum {
    SIG_WIFI_CONNECTED
};

Mdns::Mdns(const char* name)  : VerticleCoRoutine(name)
{
}

void Mdns::start()
{
    eb.on("wifi/connected",[this](Message msg) {
        signal(SIG_WIFI_CONNECTED);
        mdns_hostname_set(Sys::hostname());
        //set default instance
        mdns_instance_name_set("ESP32 Telnet");
        mdns_service_add(NULL, "_telnet", "_tcp", 23, NULL, 0);
    });
    esp_err_t err = mdns_init();
    if (err) {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

}

void Mdns::run()
{
    PT_BEGIN();
    while(true) {
        PT_WAIT_SIGNAL(1000000);
    }
    PT_END();

    //initialize mDNS service


    //set hostname

    /*    //initialize mDNS service
        esp_err_t err = mdns_init();
        if (err) {
            printf("MDNS Init failed: %d\n", err);
            return;
        }

        //set hostname
        mdns_hostname_set(Sys::hostname());
        //set default instance
        mdns_instance_name_set("My ESP32 ");     */
    /*
    eb.on("wifi/connected",[this](Message& msg) {
        signal(WIFI_CONNECTED);
        INFO(" SNTP wifi connected ");
        mdns_init();
        mdns_flags flags;
        flags = mdns_TCP;
        mdns_add_facility(Sys::hostname(), "_tcp", "Zoom=1", flags, 23, 600);
        flags = mdns_Browsable;
        mdns_add_facility(Sys::hostname(), "_telnet", "Zoom=1", flags, 23, 600);
    });
    */
//    VerticleTask::start();
}

Mdns::~Mdns()
{
}
