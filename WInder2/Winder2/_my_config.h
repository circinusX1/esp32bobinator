
#ifndef _DEV_CONFIG_H
#define _DEV_CONFIG_H


#define ATOMIC_FS_UPDATE


/////////////////////////////////////////////////////////////////////////////////////////
#define MY_BOARD            ESP32
#define ASYNC_WEB           1

/////////////////////////////////////////////////////////////////////////////////////////
#define VERSION             MKVER(1,0,0)

/////////////////////////////////////////////////////////////////////////////////////////
#define   SERIAL_BPS        115200

//#define MY_BOARD            MY_BOARD_RELAY
//#define MY_BOARD            ESP_8266_GENERIC
//#define MY_BOARD            ESP_WITH_RELAY_7_30V
//#define MY_BOARD            GLOBE_SMART_PLUG
//define MY_BOARD             ESP_WROOM

/////////////////////////////// END OF USER CONFIG ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
#if defined LOW             // use LED_ON OFF, RELAY ON OFF from following macros
#undef LOW
#undef HIGH
#endif
/////////////////////////////////////////////////////////////////////////////////////////
#if MY_BOARD==ESP_8266_GENERIC
#   define  MAX_PROGRAM_SZ  480000
#   define I2C_SDA          4
#   define I2C_SCL          5
#   define LED              2
#   define RELAY            14   // D5
#   define BUTTON           0
#   define WITH_GRAPH       1
#   define WITH_NTP         0
#   define  BUTTON_DOWN     0
#   define  BUTTON_UP       1
#   define  LED_ON          1
#   define  LED_OFF         0
#   define  RELAY_ON        0
#   define  RELAY_OFF       1
//////////////////////////////////////////////////////////////////////////////////////////
#elif MY_BOARD==ESP_WITH_RELAY
#   define   MAX_PROGRAM_SZ  540000
#   define I2C_SDA          12
#   define I2C_SCL          13   // on a side
#   define LED              2
#   define RELAY            4
#   define BUTTON           0
#   define WITH_GRAPH       1
#   define WITH_NTP         1
#   define  BUTTON_DOWN     0
#   define  BUTTON_UP       1
#   define  LED_ON          1
#   define  LED_OFF         0
#   define  RELAY_ON        1
#   define  RELAY_OFF       0
//////////////////////////////////////////////////////////////////////////////////////////
#elif MY_BOARD==GLOBE_SMART_PLUG || MY_BOARD==EZVIZ_T30
    #define   MAX_PROGRAM_SZ  340000
#   define I2C_SDA          0
#   define I2C_SCL          0
#   define LED              4
#   define RELAY            12
#   define BUTTON           14
#   define WITH_GRAPH       0
#   define WITH_NTP         0
#   define BUTTON_DOWN      0
#   define BUTTON_UP        1
#   define LED_ON           0
#   define LED_OFF          1
#   define  RELAY_ON        1
#   define  RELAY_OFF       0
//////////////////////////////////////////////////////////////////////////////////////////
#elif MY_BOARD==ESP_WITH_RELAY_7_30V
#   define  MAX_PROGRAM_SZ  480000
#   define I2C_SDA          12
#   define I2C_SCL          13
#   define LED              2
#   define RELAY            4          // relay on D5
#   define BUTTON           0
#   define WITH_GRAPH       1
#   define WITH_NTP         1
#   define BUTTON_DOWN      0
#   define BUTTON_UP        1
#   define LED_ON           0
#   define LED_OFF          1
#   define RELAY_ON         1
#   define RELAY_OFF        0
#endif

//////////////////////////////////////////////////////////////////////////////////////////
#if WITH_RTOS
//#include "freertos/FreeRTOS.h"
#endif


#endif // _DEV_CONFIG_H
