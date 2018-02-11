#include <Config.h>
#include <Property.h>
#include <Wifi.h>

#include <stdlib.h>
#include <string.h>

#include <esp_event_loop.h>
//#include <esp_mqtt.h>
#include <esp_wifi.h>

Wifi *Wifi::_me = 0;

char my_ip_address[20];
const char *getIpAddress() { return my_ip_address; }

Wifi::Wifi(const char *name) : VerticleCoRoutine(name), _ssid(32), _pswd(64) { _me = this; }

Erc Wifi::configure(const char *ssid, const char *pswd) {
  _ssid = ssid;
  _pswd = pswd;
  return E_OK;
}

esp_err_t Wifi::event_handler(void *ctx, system_event_t *event) {
  INFO(" wifi event : %d ", event->event_id);
  switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
      esp_wifi_connect();
      break;

    case SYSTEM_EVENT_STA_GOT_IP: {
      system_event_sta_got_ip_t *got_ip = &event->event_info.got_ip;
      ip4addr_ntoa_r(&got_ip->ip_info.ip, my_ip_address, 20);
      eb.publish("wifi/connected");
      break;
    }

    case SYSTEM_EVENT_STA_DISCONNECTED:
      eb.publish("wifi/disconnected");

      esp_wifi_connect();
      break;

    default:
      break;
  }

  return ESP_OK;
}



void Wifi::start() {
  config.setNameSpace("wifi");
  config.get("ssid", _ssid, WIFI_SSID);
  config.get("password", _pswd, WIFI_PASS);
  //  _hostname = Sys::hostname();

  INFO(" connecting to Wifi ssid:%s ", _ssid.c_str());
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  wifi_config_t wifi_config;
  memset(&wifi_config, 0, sizeof(wifi_config));  // needed !!
  strncpy((char *)wifi_config.sta.ssid, _ssid.c_str(), 32);
  strncpy((char *)wifi_config.sta.password, _pswd.c_str(), 64);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  new PropertyFunction<const char *>("wifi/ipAddress",getIpAddress, 10000);
}

void Wifi::run(){
    PT_BEGIN();
    while(true){
        PT_WAIT_SIGNAL(100000);
    }
    PT_END();
}

