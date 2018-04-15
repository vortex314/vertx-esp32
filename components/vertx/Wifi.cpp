#include <Config.h>
#include <Property.h>
#include <Wifi.h>

#include <stdlib.h>
#include <string.h>

#include <esp_event_loop.h>
//#include <esp_mqtt.h>
#include <esp_wifi.h>

#define VERTX_CHECK(x) do {                                             \
        esp_err_t __err_rc = (x);                                       \
        if (__err_rc != ESP_OK) {                                       \
            WARN("%s = %d @ %s:%d ",#x,__err_rc, __FILE__, __LINE__);   \
        }                                                               \
    } while(0);


bool startsWith(const char *str,const char *pre )
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

Wifi *Wifi::_me = 0;

char my_ip_address[20];
const char *getIpAddress()
{
    return my_ip_address;
}

const char *Wifi::getSSID()
{
    return _ssid.c_str();
}

Wifi::Wifi(const char *name) : VerticleCoRoutine(name), _ssid(32), _pswd(64),_prefix(32)
{
    _me = this;
    _prefix = "Merckx";
}

Erc Wifi::configure(const char *ssid, const char *pswd)
{
    _ssid = ssid;
    _pswd = pswd;
    return E_OK;
}



esp_err_t Wifi::event_handler(void *ctx, system_event_t *event)
{
    INFO(" wifi event : %d ", event->event_id);
    Wifi* wifi = (Wifi*)ctx;
    switch (event->event_id) {

    case SYSTEM_EVENT_SCAN_DONE : {
        INFO("SYSTEM_EVENT_SCAN_DONE");
        wifi->scanDoneHandler();
        break;
    }
    case SYSTEM_EVENT_STA_STOP: {
        INFO("SYSTEM_EVENT_STA_STOP");
        break;
    }
    case SYSTEM_EVENT_STA_START: {
        INFO(" SYSTEM_EVENT_STA_START");
        wifi->startScan();
        break;
    }
    case SYSTEM_EVENT_STA_GOT_IP: {
        INFO(" SYSTEM_EVENT_STA_GOT_IP");
        system_event_sta_got_ip_t *got_ip = &event->event_info.got_ip;
        ip4addr_ntoa_r(&got_ip->ip_info.ip, my_ip_address, 20);
        eb.publish("wifi/connected");
        break;
    }
    case SYSTEM_EVENT_STA_CONNECTED: {
        INFO(" SYSTEM_EVENT_STA_CONNECTED");
        break;
    }
    case SYSTEM_EVENT_STA_DISCONNECTED:
        INFO("SYSTEM_EVENT_STA_DISCONNECTED");
        eb.publish("wifi/disconnected");
        wifi->startScan();
        break;

    default:
        break;
    }

    return ESP_OK;
}


void Wifi::connectToAP(const char* ssid)
{
    INFO(" connecting to SSID : %s",ssid);
    wifi_config_t wifi_config;
    _ssid = ssid;
    memset(&wifi_config, 0, sizeof(wifi_config));  // needed !!
    strncpy((char *)wifi_config.sta.ssid, _ssid.c_str(), 32);
    strncpy((char *)wifi_config.sta.password, _pswd.c_str(), 64);

    VERTX_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    VERTX_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    esp_wifi_connect();
}

void Wifi::scanDoneHandler()
{
    uint16_t sta_number;
    esp_wifi_scan_get_ap_num(&sta_number);
    INFO(" found %d AP's ",sta_number);
    if ( sta_number==0 ) {
        WARN(" no AP found , restarting scan.");
        startScan();
        return;
    }
    wifi_ap_record_t apRecords[sta_number];
    esp_wifi_scan_get_ap_records(&sta_number,apRecords);
    int strongestAP = -1;
    int strongestRssi=-200;
    for(uint32_t i=0; i<sta_number; i++) {
        INFO(" %s : %d ",apRecords[i].ssid,apRecords[i].rssi);
        if ( (apRecords[i].rssi > strongestRssi) && startsWith((const char*)apRecords[i].ssid,_prefix.c_str()) ) {
            strongestAP=i;
            strongestRssi=apRecords[i].rssi;
        }
    }
    if ( strongestAP == -1 ) {
        WARN(" no AP found matching pattern %s, restarting scan.",_prefix.c_str());
        startScan();
        return;
    }
    connectToAP((const char*)apRecords[strongestAP].ssid);
}

void Wifi::startScan()
{
    INFO(" starting WiFi scan.");
    wifi_scan_config_t scanConfig= {NULL,NULL,0,false,WIFI_SCAN_TYPE_ACTIVE,{0,0}};
    VERTX_CHECK(esp_wifi_scan_start(&scanConfig,false));
}


void Wifi::start()
{
    config.setNameSpace("wifi");
    config.get("ssid", _ssid, WIFI_SSID);
    config.get("password", _pswd, WIFI_PASS);
    //  _hostname = Sys::hostname();

    INFO(" connecting to Wifi ssid:%s ", _ssid.c_str());
    tcpip_adapter_init();
    VERTX_CHECK(esp_event_loop_init(event_handler, this));

    wifi_init_config_t wifiInitializationConfig = WIFI_INIT_CONFIG_DEFAULT();
    VERTX_CHECK(esp_wifi_init(&wifiInitializationConfig));
    VERTX_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    VERTX_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    VERTX_CHECK(esp_wifi_set_promiscuous(false));
    VERTX_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B));

    VERTX_CHECK(esp_wifi_start());
//    VERTX_CHECK(esp_wifi_set_channel(14, WIFI_SECOND_CHAN_NONE));
    /*    wifi_country_t country= {{'B','E',0},1,13,WIFI_COUNTRY_POLICY_AUTO};
        VERTX_CHECK(esp_wifi_set_country(&country));*/
//    wifi_country_t country =   WIFI_COUNTRY_EU;
//   VERTX_CHECK(esp_wifi_set_country(WIFI_COUNTRY_EU));


    new PropertyFunction<const char *>("wifi/ipAddress",getIpAddress, 10000);
    new PropertyFunction<const char* >("wifi/ssid",[this]() {
        return _ssid.c_str();
    },4000);
}

void Wifi::run()
{
    PT_BEGIN();
    while(true) {
        PT_WAIT_SIGNAL(100000);
    }
    PT_END();
}
