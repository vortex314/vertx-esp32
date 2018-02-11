#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <vertx.h>
#include <Property.h>
#include "HCSR04.h"

class UltraSonic : public VerticleCoRoutine {
  HCSR04 _hcsr;

 public:
  UltraSonic(const char* name, Connector& connector);
  UltraSonic(const char* name, DigitalOut& pinTrigger, DigitalIn& pinEcho);
  virtual ~UltraSonic(){};

  void start() ;
  void run() ;
};

#endif // ULTRASONIC_H
