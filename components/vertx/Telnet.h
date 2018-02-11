#ifndef _TELNET_H_
#define _TELNET_H_
#include <vertx.h>
#include <Config.h>
#include "lwip/err.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/mem.h"
class Telnet : public VerticleTask
{
    struct netconn *_acceptConn;
    struct netconn *_ttyConn;
    bool _echo=false;
    Str _outputBuffer;
    Bytes _inputBuffer;
    Str _inputLine;
    uint32_t _port;
    bool _wifiConnected=false;
public:
    Telnet(const char *name);
    void start();
    void run();
    Erc readLine(Str& str);
    void command(Str& line);
    char getChar();
    Erc read(uint8_t* pb);
    void write(char c);
    void write(Str& str);
    void write(const char* line);
    void write(uint8_t* data,uint32_t length);
};

#endif
