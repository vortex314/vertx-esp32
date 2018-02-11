#include <Telnet.h>
#include <Str.h>
/*
 *  data                     All terminal input/output data.
   End subNeg    240 FO     End of option subnegotiation command.
   No Operation  241 F1     No operation command.
   Data Mark     242 F2     End of urgent data stream.
   Break         243 F3     Operator pressed the Break key or the
                            Attention key.
   Int process   244 F4     Interrupt current process.
   Abort output  245 F5     Cancel output from current process.
   You there?    246 F6     Request acknowledgment.
   Erase char    247 F7     Request that operator erase the previous
                              character.
   Erase line    248 F8     Request that operator erase the previous
                              line.
   Go ahead!     249 F9     End of input for half-duplex connections.
   SubNegotiate  250 FA     Begin option subnegotiation.
   Will Use      251 FB     Agreement to use the specified option.
   Won’t Use     252 FC     Reject the proposed option.
   Start use     253 FD     Request to start using specified option.
   Stop Use      254 FE     Demand to stop using specified option.
   IAC           255 FF     Interpret as command.
 *
 **/
#define TELNET_END 236
#define TELNET_BREAK                            0xF3
#define TELNET_INTERRUPT                    0xF4
#define TELNET_SUB_OPTION_BEGIN     0xfa
#define TELNET_SUB_OPTION_END       0xf0
#define TELNET_DO                           0xfd
#define TELNET_WONT                     0xfc
#define TELNET_WILL                         0xfb
#define TELNET_DONT                     0xfe
#define TELNET_IAC                          0xff
#define TELNET_OPTION_ECHO          1
#define TELNET_OPTION_LINEMODE  34

enum {
    SIG_WIFI
};


Telnet::Telnet(const char *name)
    : VerticleTask(name, 3000, 5),_outputBuffer(1024),_inputBuffer(100),_inputLine(100)
{
}

void Telnet::start()
{
    config.setNameSpace(name());
    config.get("port",_port,23);
    eb.on("wifi/connected",[this](Message& msg) {
        _wifiConnected=true;
        signal(SIG_WIFI);
    });

    eb.on("wifi/disconnected",[this](Message& msg) {
        _wifiConnected=false;
        signal(SIG_WIFI);
    });
    VerticleTask::start();
}


void Telnet::run()
{
    err_t err;
    while (true) {
        while(!_wifiConnected) {
            waitSignal(5000);
        };
        while(_wifiConnected) {

#if LWIP_IPV6
            _acceptConn = netconn_new(NETCONN_TCP_IPV6);
            netconn_bind(_acceptConn, IP6_ADDR_ANY, _port);
#else  /* LWIP_IPV6 */
            _acceptConn = netconn_new(NETCONN_TCP);
            netconn_bind(_acceptConn, IP_ADDR_ANY, _port);
#endif /* LWIP_IPV6 */
            LWIP_ERROR("tcpecho: invalid conn", (_acceptConn != NULL), return;);

            while(_wifiConnected) {
                netconn_listen(_acceptConn); /* Tell connection to go into listening mode. */
                while (true) {
                    /* Grab new connection. */
                    err = netconn_accept(_acceptConn, &_ttyConn);
                    /*INFO("accepted new connection %p\n", newconn);*/
                    /* Process the new connection. */
                    if (err == ERR_OK) {
                        INFO(" connection accepted.");
                        while(true) {
                            write("\n");
                            write (Sys::hostname());
                            write(" $ ");
                            _inputBuffer.clear();
                            Erc erc = readLine(_inputLine);
                            if ( erc ==E_OK) { // handle Line
                                command(_inputLine);
                            } else if ( erc==ENODATA) {
                                break;
                            }
                        }
                        netconn_close(_ttyConn); /* Close connection and discard connection identifier. */
                    }
                }

            }
            netconn_delete(_acceptConn);;
        }
    }
}

#define MAX_ARGS 6

void Telnet::command(Str& line)
{
    _outputBuffer.clear();
    const char* arg[MAX_ARGS];
    char * pch;
    pch = strtok ((char*)line.c_str()," \t");
    uint32_t count=0;
    while (pch != NULL) {
        arg[count++]=pch;
        INFO(" arg[%d] = '%s'",count-1,arg[count-1]);
        pch = strtok (NULL, " \t");
    }

    if ( count >= 2 ) {
        if ( strcmp(arg[0],"config")==0) {
            if ( strcmp(arg[1],"show")==0) {
                config.load();
                config.printPretty(_outputBuffer);
            } else if ( strcmp(arg[1],"set")==0 && count == 5) {
                config.setNameSpace(arg[2]);
                config.set(arg[3],arg[4]);
                _outputBuffer.format(" done - %s %s=%s ",arg[2],arg[3],arg[4]);
            } else if ( strcmp(arg[1],"delete")==0 ) {
                config.setNameSpace(arg[2]);
                config.remove(arg[3]);
                _outputBuffer.format(" deleted %s in NS %s ",arg[3],arg[2]);
            } else if ( strcmp(arg[1],"save")==0 ) {
                config.save();
                _outputBuffer=" config saved.";
            } else if ( strcmp(arg[1],"clear")==0 ) {
                config.clear();
                _outputBuffer=" config saved.";
            }else {
                _outputBuffer="unknown command";
            }
        } else if ( strcmp(arg[0],"system")==0) {
            if ( strcmp(arg[1],"reset")==0) {
 //               sdk_system_restart();
                esp_restart();
            } else  if ( strcmp(arg[1],"show")==0) {
                _outputBuffer.format(" CPU : %s \n",Sys::getProcessor());
                _outputBuffer.format(" heap:%d \n",Sys::getFreeHeap());
                _outputBuffer.format(" serial : %X \n",Sys::getSerialId());
                _outputBuffer.format(" sdk : %s \n",esp_get_idf_version());
                _outputBuffer.format(" build : %s \n",Sys::getBuild());
            } else {
                _outputBuffer="unknown command";
            }
        } else {
            _outputBuffer="unknown service";
        }
    } else {
        _outputBuffer=" not enough arguments.";
    }
    write(_outputBuffer);
}

void Telnet::write(uint8_t* data,uint32_t len )
{
    netconn_write(_ttyConn, data ,len, NETCONN_COPY);
}

void Telnet::write(Str& str)
{
    write(str.data(), str.length());
}

void Telnet::write(const char* line)
{
    write((uint8_t*)line,strlen(line));
}

void Telnet::write(char c)
{
    write((uint8_t*)&c,1);
}



Erc Telnet::read(uint8_t* pb)
{
    if ( _inputBuffer.hasData())  {
        *pb = _inputBuffer.read();
        return  E_OK;
    }
    _inputBuffer.clear();

    struct netbuf *buf;
    err_t err;
    char *data;
    u16_t len;
    err = netconn_recv(_ttyConn, &buf);
    if ( err != ERR_OK ) return ENODATA;

    do {
        netbuf_data(buf, (void**)&data, &len);
        INFO(" len : %d ",len);
        _inputBuffer.write((uint8_t*)data,0,len);
    } while (netbuf_next(buf) >= 0);
    netbuf_delete(buf);

    _inputBuffer.offset(0);
    *pb =  _inputBuffer.read();
    return E_OK;
}
/*
 *  IGNOR TELNET COMMANDS
 * */
char Telnet::getChar()
{
    uint8_t b,cmd,option;
    Erc erc;
    while(true) {
        erc = read(&b);
        if ( erc == ENODATA ) return 0;
        if ( b == TELNET_IAC ) {
            read(&cmd);
            read(&option);
            INFO(" cmd : %d option: %d ",cmd,option);
            if ( cmd == TELNET_WONT ) {

            } else if ( cmd == TELNET_WILL ) {
                if ( option == TELNET_OPTION_ECHO ) {
                    INFO(" DONT echo ");
                    uint8_t arr[]= {TELNET_IAC,TELNET_DONT,TELNET_OPTION_ECHO};
                    write(arr,sizeof(arr));
                }
                /*               if ( option == TELNET_OPTION_LINEMODE ) {
                                   INFO(" DONT linemode ");
                                   uint8_t arr[]= {TELNET_IAC,TELNET_DONT,TELNET_OPTION_LINEMODE};
                                   write(arr,sizeof(arr));
                               }*/
            } else if ( cmd == TELNET_DO ) {


            }  else if ( cmd == TELNET_DONT ) {

            } else if ( cmd==TELNET_INTERRUPT  || cmd==TELNET_BREAK || cmd==TELNET_END) {
                return 0;
            }
        } else {
            return b;
        }
    }
}


Erc Telnet::readLine(Str& str)
{
    size_t i = 0;
    uint32_t buf_size=str.capacity();
    char *buffer=(char*)str.data();
    int c;

    while (true) {
        c = getChar();
        INFO("%x",c);
        if ( c == 0 ) {
            return ENODATA;
        } else if (c == '\r' ) {
            // ignore
        } else if (c == '\n' ) {
            break;
        } else if (c == '\b' || c == 0x7f) {
            if (i) {
                if (_echo) {
                    write("\b \b");
                }
                i--;
            }
        } else if (c < 0x20) {
            /* Ignore other control characters */
        } else if (i >= buf_size - 1) {
            if (_echo) {
                write('\a');
            }
        } else {
            buffer[i++] = c;
            if (_echo) {
                write(c);
            }
        }
    }

    str.length(i);
    buffer[i] = 0;
    return E_OK;
}


/*

   Option ID     Option Codes     Description
   Dec Hex
   -------------------------------------------------------------------
    0  0         Binary Xmit      Allows transmission of binary data.
    1  1         Echo Data        Causes server to echo back
                                  all keystrokes.
    2  2         Reconnect        Reconnects to another TELNET host.
    3  3         Suppress GA      Disables Go Ahead! command.
    4  4         Message Sz       Conveys approximate message size.
    5  5         Opt Status       Lists status of options.
    6  6         Timing Mark      Marks a data stream position for
                                  reference.
    7  7         R/C XmtEcho      Allows remote control of terminal
                                  printers.
    8  8         Line Width       Sets output line width.
    9  9         Page Length      Sets page length in lines.
   10  A        CR Use            Determines handling of carriage returns.
   11  B        Horiz Tabs        Sets horizontal tabs.
   12  C        Hor Tab Use       Determines handling of horizontal tabs.
   13  D        FF Use            Determines handling of form feeds.
   14  E        Vert Tabs         Sets vertical tabs.
   15  F        Ver Tab Use       Determines handling of vertical tabs.
   16 10       Lf Use             Determines handling of line feeds.
   17 11       Ext ASCII          Defines extended ASCII characters.
   18 12       Logout             Allows for forced log-off.
   19 13       Byte Macro         Defines byte macros.
   20 14       Data Term          Allows subcommands for Data Entry to
                                  be sent.
   21 15       SUPDUP             Allows use of SUPDUP display protocol.
   22 16       SUPDUP Outp        Allows sending of SUPDUP output.
   23 17       Send Locate        Allows terminal location to be sent.
   24 18       Term Type          Allows exchange of terminal type
                                  information.
   25 19       End Record         Allows use of the End of record code
                                  (0xEF).
   26 1A       TACACS ID          User ID exchange used to avoid more
                                  than 1 log-in.
   27 1B       Output Mark        Allows banner markings to be sent on
                                  output.
   28 1C       Term Loc#          A numeric ID used to identify terminals.
   29 1D       3270 Regime        Allows emulation of 3270 family
                                  terminals.
   30 1E       X.3 PAD            Allows use of X.3 protocol emulation.
   31 1F       Window Size        Conveys window size for emulation
                                  screen.
   32 20       Term Speed         Conveys baud rate information.
   33 21       Remote Flow        Provides flow control (XON, XOFF).
   34 22       Linemode           Provides linemode bulk character
                                  transactions.
   255 FF      Extended           options list  Extended options list.

*/
