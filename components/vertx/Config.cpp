/*
 * Config.cpp
 *
 *  Created on: Jul 12, 2016
 *      Author: lieven
 */

#include "Config.h"

#include <Sys.h>


#define EEPROM_SIZE 512
#define EEPROM_MAGIC 0xDEADBEEF
#define KEY_SIZE 40
#define VALUE_SIZE 60

Config::Config() : _jsonBuffer(1024), _nameSpace(30), _loaded(false) {}

Config::~Config() {}

void Config::initMagic() {}

bool Config::checkMagic()
{
    return false;
}




const char* Config::clone(const char* s)
{
    char* p = (char*)_jsonBuffer.alloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

//======================================NAMESPACE
JsonObject& Config::nameSpace()
{
    if (_root->containsKey(_nameSpace.c_str())) {
        JsonObject& jso = (*_root)[_nameSpace.c_str()];
        return jso;
    } else {
        JsonObject& jso = _root->createNestedObject(clone(_nameSpace.c_str()));
        return jso;
    }
}

void Config::setNameSpace(const char* ns)
{
    _nameSpace = ns;
}

const char* Config::getNameSpace()
{
    return _nameSpace.c_str();
}
//==================================================
bool Config::hasKey(const char* key)
{
    JsonObject& ns = nameSpace();
    if (ns.containsKey(key)) {
        return true;
    }
    return false;
}

void Config::remove(const char* key)
{
    load();
    JsonObject& ns = nameSpace();
    ns.remove(key);
    INFO(" Config => SAVE  remove %s ", key);
}

void Config::set(const char* key, const char* value)
{
    JsonObject& ns = nameSpace();
    ns[clone(key)] = clone(value);
}

void Config::set(const char* key, Str& value)
{
    load();
    set(key, value.c_str());
}

void Config::set(const char* key, uint32_t value)
{
    load();
    JsonObject& ns = nameSpace();
    ns[clone(key)] = value;
}

void Config::set(const char* key, int32_t value)
{
    load();
    JsonObject& ns = nameSpace();
    ns[clone(key)] = value;
}

void Config::set(const char* key, double value)
{
    load();
    JsonObject& ns = nameSpace();
    INFO(" SET %s=%f ", key, value);
    ns[clone(key)] = value;
}

void Config::get(const char* key, Str& value, const char* defaultValue)
{
    load();
    JsonObject& ns = nameSpace();
    if (ns.containsKey(key)) {
        value = ns[key];
    } else {
        set(key, defaultValue);
        value = defaultValue;
    }
    INFO(" %s.%s = '%s' ", _nameSpace.c_str(), key, value.c_str());
}

void Config::get(const char* key, int32_t& value, int32_t defaultValue)
{
    load();
    JsonObject& ns = nameSpace();
    if (ns.containsKey(key)) {
        value = ns[key];
    } else {
        set(key, defaultValue);
        value = defaultValue;
    }
    INFO(" %s.%s = %d ", _nameSpace.c_str(), key, value);
}

void Config::get(const char* key, uint32_t& value, uint32_t defaultValue)
{
    load();
    JsonObject& ns = nameSpace();
    if (ns.containsKey(key)) {
        value = ns[key];
    } else {
        set(key, defaultValue);
        value = defaultValue;
    }
    INFO(" %s.%s = %u ", _nameSpace.c_str(), key, value);
}

void Config::get(const char* key, double& value, double defaultValue)
{
    load();
    JsonObject& ns = nameSpace();
    if (ns.containsKey(key)) {
        value = ns[key];
    } else {
        set(key, defaultValue);
        value = defaultValue;
    }
    INFO(" %s.%s = %f ", _nameSpace.c_str(), key, value);
}

void Config::print(Str& str)
{
    _root->printTo(_charBuffer, sizeof(_charBuffer));
    str=_charBuffer;
}

void Config::printPretty(Str& str)
{
    _root->prettyPrintTo(_charBuffer, sizeof(_charBuffer));
    str=_charBuffer;
}

#ifdef ESP32_IDF

#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"

void Config::clear(){
    nvs_flash_erase();
}

void Config::load()
{
    if (_loaded) {
//        char _buffer[1024];
//       _root->printPretty(_buffer, sizeof(_buffer));
        //    DEBUG(" config object : %s", _buffer);
        return;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ERROR("Error (%d) opening NVS handle!\n", err);
    } else {
        INFO("NVS storage open \n");

        uint32_t required_size;
        err = nvs_get_str(my_handle, "config", _charBuffer, &required_size);

        switch (err) {
        case ESP_OK:
            INFO(" config : %s ", _charBuffer);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            WARN(" no config in NVS ")
            strcpy(_charBuffer, "{}");
            break;
        default:
            ERROR(" error read NVS : %d ", err);
            strcpy(_charBuffer, "{}");
        }
        nvs_close(my_handle);
    }
    _loaded = true;
    INFO("%s",_charBuffer);
    _root = &_jsonBuffer.parseObject((const char*)_charBuffer);
    if (!_root->success()) {
        _root = &_jsonBuffer.parseObject("{}");
    }
}

void Config::save()
{
    esp_err_t err;
    _root->printTo(_charBuffer, sizeof(_charBuffer));
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ERROR("Error (%d) opening NVS handle!\n", err);
        return;
    }
    err = nvs_set_str(my_handle, "config", _charBuffer);
    if (err) {
        ERROR(" nvs_set_str(my_handle,  'config', buffer): %d ", err);
    }

    err = nvs_commit(my_handle);
    if (err) {
        ERROR(" nvs_commit(my_handle) : %d ", err);
        nvs_close(my_handle);
        return;
    }
    INFO(" config saved : %s ", _charBuffer);
}
#endif // ESP-IDF

//==========================================================================
//

#ifdef ESP_OPEN_RTOS
extern "C" {
#include <sysparam.h>
#include <espressif/spi_flash.h>
#include "espressif/esp_common.h"
}



void Config::load()
{
    if (_loaded) {
        return;
    }
    sysparam_status_t status;
    uint32_t base_addr, num_sectors;

    status = sysparam_get_info(&base_addr, &num_sectors);
    if (status == SYSPARAM_OK) {
        INFO("[current sysparam region is at 0x%08x (%d sectors)]", base_addr, num_sectors);
    } else {
        num_sectors = DEFAULT_SYSPARAM_SECTORS;
        base_addr = sdk_flashchip.chip_size - (5 + num_sectors) * sdk_flashchip.sector_size;
    }
    uint8_t* destPtr;
    size_t actual_length;
    bool is_binary;
    status = sysparam_get_data("config", &destPtr, &actual_length, &is_binary);
    if (status == SYSPARAM_OK) {
        strncpy(_charBuffer,(char*)destPtr,sizeof(_charBuffer));
        free(destPtr);
    } else {
        ERROR("sysparam_get_data('config',...) fails : %d ",status);
        sysparam_set_data("config",(uint8_t*)"{}",3,false);
        return;
    }
// load string from flash

    // load JsonObject from String

    _loaded = true;
    _root = &_jsonBuffer.parseObject((const char*)_charBuffer);
    if (_root->success()) {

    } else {
        ERROR(" couldn't parse config '%s' , dropped old config ! ",_charBuffer);
        strcpy(_charBuffer,"{}");
        _root = &_jsonBuffer.parseObject("{}");
    }
//    char buffer[1024];
    _root->printTo(_charBuffer, sizeof(_charBuffer));
    INFO(" config loaded : %s",_charBuffer);
}

void Config::save()
{
    _root->printTo(_charBuffer, sizeof(_charBuffer));
    sysparam_status_t status = sysparam_set_string("config", _charBuffer);
    if ( status == SYSPARAM_OK) {
        INFO(" config saved : %s ", _charBuffer);
    } else {
        ERROR("config save failed : %d ",status)
    }
}
#endif

Config config;
