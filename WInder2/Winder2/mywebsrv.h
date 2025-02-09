#ifndef MYWEBSRV_H
#define MYWEBSRV_H

#include "_my_config.h"
#include "_utils.h"



#if ASYNC_WEB
#define MyWebSrv AsyncWebServer
#define  RQ_TYPE_AND_PARAM    AsyncWebServerRequest* request
#define  RQ_PARAM             request
#define  RQ_OBJECT             (*request)
#else


#define MyWebSrv ESP8266WebServer
#define  RQ_TYPE_AND_PARAM
#define  RQ_PARAM
#endif

#endif // MYWEBSRV_H
