#include "Compass.h"

Compass::Compass(const char* name, I2C& i2c) : VerticleCoRoutine(name), _hmc(i2c),_v(
{
    0,0,0
})
{
};
Compass::Compass(const char* name, Connector& connector)
    : VerticleCoRoutine(name), _hmc(connector),_v(
{
    0,0,0
})
{

};


void Compass::start()
{
    UID.add("x");
    UID.add("y");
    UID.add("z");
    if (_hmc.init()) {
        INFO("HMC5883L initialized.")
    } else {
        ERROR("HMC5883L initialization failed.");
    }
    _hmc.setRange(HMC5883L_RANGE_1_3GA);

    // Set measurement mode
    _hmc.setMeasurementMode(HMC5883L_CONTINOUS);

    // Set data rate
    _hmc.setDataRate(HMC5883L_DATARATE_30HZ);

    // Set number of samples averaged
    _hmc.setSamples(HMC5883L_SAMPLES_8);

    // Set calibration offset. See HMC5883L_calibration.ino
    _hmc.setOffset(0, 0);
    new PropertyReference<float>("compass/x",_v.XAxis,1000);
    new PropertyReference<float>("compass/y",_v.YAxis,1000);
    new PropertyReference<float>("compass/z",_v.ZAxis,1000);
    VerticleCoRoutine::start();
}

void Compass::run()
{

    PT_BEGIN();
    while(true) {
        PT_WAIT_SIGNAL(500);
        _v = _hmc.readNormalize();
        INFO("%f:%f:%f", _v.XAxis, _v.YAxis, _v.ZAxis);
    }
    PT_END();
}
