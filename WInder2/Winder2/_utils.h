#pragma once

#include "_my_config.h"
#if MY_BOARD==ESP32
#   include <WiFi.h>
#else
#   include <ESP8266WiFi.h>
#endif
#include <WiFiClient.h>
#include <LittleFS.h>
#if ASYNC_WEB
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#else
#include <ESP8266WebServer.h>
#endif

#include <DNSServer.h>

#if MY_BOARD==ESP32
#   include <ESPmDNS.h>
#else
#   include <ESP8266mDNS.h>
#endif



#include <EEPROM.h>
#include <Ticker.h>
#if WITH_NTP
#   include <NTPClient.h>
#endif


/////////////////////////////////////////////////////////////////////////////////////////
#define USER_SIG        0x13
#define RAMSIG          0x21
#define BOGUS_VAL       999999.99
#define MKVER(a,b,c)    int(a<<16|b<<8|c)
#define DEBUG   1
//////////////////////////////////////////////////////////////////////////////////////////
#if DEBUG
#   define TRACE()         Serial.print(__FUNCTION__); Serial.println(__LINE__);
#else
#   define TRACE()
#endif

//////////////////////////////////////////////////////////////////////////////////////////
inline void LOG( const char * format, ... )
{
    char lineBuffer[64];
    va_list args;
    va_start( args, format );
    int len = vsnprintf( lineBuffer, sizeof lineBuffer, (char*) format, args );
    Serial.print( lineBuffer );
    Serial.println();
    va_end( args );
    Serial.flush();
}

/////////////////////////////////////////////////////////////////////////////////////////

inline void __nill(){}

/////////////////////////////////////////////////////////////////////////////////////////
enum ETYP{
    eNA=0,
    eTIME,
    eRELAY,
    eTEMP,
    eHUM,
    ePRESS,
};


//////////////////////////////////////////////////////////////////////////////////////////
class sens_data_t;
class sensdata_t
{
public:
    sensdata_t(ETYP e, float d):type(e){u.f=d;}
    sensdata_t(ETYP e, bool d):type(e){u.uc=d;}
    sensdata_t(ETYP e, int d):type(e){u.i=d;}
    sensdata_t(ETYP e):type(e){}
    ~sensdata_t(){;}

public:
    ETYP  type;
    union{ float f; int z; uint8_t uc; int16_t ss; uint16_t us; int i;}u;
};


#define FORCE_INLINE __attribute__((always_inline)) inline

#define COUNT_OF(a_) sizeof(a_)/sizeof(a_[0])

#define BTIME __TIME__

#define _S(xx)  String(xx)

extern bool GTest;
