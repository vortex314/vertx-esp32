#ifndef COMPASS_H
#define COMPASS_H

#include <vertx.h>
#include <HMC5883L.h>
#include <Property.h>

class Compass : public VerticleCoRoutine {
  HMC5883L _hmc;
  struct Vector _v;

 public:
  Compass(const char* name, I2C& i2c) ;
  Compass(const char* name, Connector& connector);
  virtual ~Compass(){};

  void start();
  void run() ;
};

#endif // COMPASS_H
