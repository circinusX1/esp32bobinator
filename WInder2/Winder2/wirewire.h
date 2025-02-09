#ifndef WIREWIRE_H
#define WIREWIRE_H

#include "_my_config.h"
#include "_utils.h"
#include <twi.h>
#include <Wire.h>
#if I2C_SDA
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

extern bool _taken;
extern int  _i2c_sda,_i2c_scl;

inline void i2c_begin(int sda, int scl)
{
    LOG("i2cbegin(%d %d)", sda, scl);
    _i2c_sda = sda; _i2c_scl = scl;
    if(!_taken)
    {
        LOG(__FUNCTION__);
        Wire.begin(sda,scl);
        _taken = true;
    }
    else
    {
        Wire.flush();
        LOG("I2C=1");
    }
    delay(128);
}

inline void i2c_begin(int sda, int scl, int address)
{
    _i2c_sda = sda; _i2c_scl = scl;
    if(!_taken)
    {
        LOG(__FUNCTION__);
        Wire.begin(sda,scl,address);
        _taken = true;
    }
    else
    {
        Wire.flush();
        LOG("I2C=1");
    }
}

inline void i2c_end()
{
    if(_taken)
    {
        LOG(__FUNCTION__);
        Wire.flush();
        pinMode(_i2c_sda, INPUT);
        pinMode(_i2c_scl, INPUT);
        _taken=false;
    }
    else
    {
        LOG("I2C=OK");
    }
}

bool i2c_scan(String* page, uint8_t* paddrs);

#endif //#if I2C_SDA

#endif // WIREWIRE_H
