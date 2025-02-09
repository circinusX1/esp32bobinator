
#include "wirewire.h"

#if I2C_SDA

bool _taken = false;
int  _i2c_sda,_i2c_scl;

bool i2c_scan(String* page, uint8_t* paddrs)
{
    int det = 0;
    bool ret = false;
    byte error, address;

    i2c_begin(I2C_SDA,I2C_SCL);

    Serial.println("Scanning");
    if(page) page->concat("Scanning<br>");
    for(address = 20; address < 70; address++ )
    {
        Wire.beginTransmission(address);
        delay(2);
        error = Wire.endTransmission();
        delay(2);
        if (error == 0)
        {
            delay(16);
            Serial.print("\r\nI2C device: 0x");
            Serial.print(address, HEX);
            Serial.println("");
            if(page) page->concat("Found : 0x");
            if(page) page->concat(String(address,HEX));
            if(page) page->concat("<br />");
            ret = true;
            if(paddrs && det<MAX_SENS)
            {
                paddrs[det] = address;
                ++det;
            }
        }
    }
    Serial.println("Scaning Done");
    if(page) page->concat("Scanning Done<br>");
    return ret;
}
#endif //  #if I2C_SDA
