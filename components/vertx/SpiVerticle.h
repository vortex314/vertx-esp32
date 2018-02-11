#ifndef SPI_H
#define SPI_H

#include <vertx.h>
#include <Hardware.h>

class SpiVerticle : public VerticleCoRoutine
{
    Spi& _spi;
public:
    SpiVerticle(const char* name);
    ~SpiVerticle();
    void start();
    void run();

};

#endif // SPI_H
