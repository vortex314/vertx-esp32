#ifndef MDNS_H
#define MDNS_H
#include <vertx.h>
class Mdns : public VerticleCoRoutine
{
public:
    Mdns(const char* name);
    ~Mdns();
    void start();
    void run();
};

#endif // MDNS_H
