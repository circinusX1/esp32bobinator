#ifndef WIFISRV_H
#define WIFISRV_H

#include <DNSServer.h>
#include "_my_config.h"
#include "_utils.h"
#include "mywebsrv.h"

class simple_clock_t;
class sensor_th_t;
class espxxsrv_t
{
public:
    espxxsrv_t();
    ~espxxsrv_t(){};

    void begin();
    void end();
    void loop();
    int mode()const{
        if(_sta_lost) return 3;
        if(_sta) return 10;
        if(_ap) return 2;
        return 1;
    }
    bool is_otaing()const{return _otaing;}
    void on_relay_event(const sensdata_t* );
    void on_sensor_event(const sensor_th_t* );
#if ASYNC_WEB
    static void  end_htm(String& page, bool endht, RQ_TYPE_AND_PARAM);
#else
    static void  end_htm(String& page, bool endht=true);
#endif
    static const String& start_htm(bool content=true);
    bool  sta_connected(){return _sta && _sta_lost==0;}
    bool  ap_active(){return _ap;}
    static bool ping(const char*);
    void flush(){
#if !ASYNC_WEB
        char buf[8]={0};
        _esp.sendContent((const char *)buf);
#endif
    }
    void send(const char* p){
#if !ASYNC_WEB
        char buf[8]={0};
        _esp.send(200,"text/html",p);
#endif
    }
    static String toStrIp(const IPAddress& ip);
    static bool _isIp(const String& str);
private:
    static void handleUser(RQ_TYPE_AND_PARAM);
    static void handleCss(RQ_TYPE_AND_PARAM);
    static void handleJs(RQ_TYPE_AND_PARAM);
    static void handleRoot(RQ_TYPE_AND_PARAM);
    static void handleWifi(RQ_TYPE_AND_PARAM);
    static void handleOta(RQ_TYPE_AND_PARAM);
    static void handleTime(RQ_TYPE_AND_PARAM);
    static void handleWifiSave(RQ_TYPE_AND_PARAM);
    static void handleFetch(RQ_TYPE_AND_PARAM);
    static void handleConfig(RQ_TYPE_AND_PARAM);
    static void handleConfigSave(RQ_TYPE_AND_PARAM);
    static void handleError(RQ_TYPE_AND_PARAM);
    static void handleSensor(RQ_TYPE_AND_PARAM);
    static void handleGraph(RQ_TYPE_AND_PARAM);
#if ASYNC_WEB
    static void handleUpload(AsyncWebServerRequest *request, String filename,
                        size_t index, uint8_t *data, size_t len, bool final);
#endif

    static bool _capturePage(RQ_TYPE_AND_PARAM);
    void _schedule(const char* value=nullptr);
    void _handleFetch(RQ_TYPE_AND_PARAM);
    void _connect_cli();
    void _enable_ap();
    void _disable_ap();
    void _start_dns();
    void _stop_dns();
private:
    bool                _ap = false;
    bool                _sta = false;
    size_t              _last_remote_get = 0;
    int                 _wstate = -1;
    size_t              _con_time = 0;
    int                 _con_retry = 0;
    int                 _delay = 0;
    bool                _otaing=false;
    int                 _sta_lost = 0;
    bool                _began = false;
    DNSServer*          _dnsServer = nullptr;
public:
    MyWebSrv                    _esp;
    static espxxsrv_t*          WifiSrv;
    static bool                 hot;
    size_t                      _bytes = 0;
    bool                        _error=0;
    bool                        _myfile=0;
    static  bool                _faren;
    unsigned long               _last_ping_msecs;
};

#define SSID_CONF()    (CFG(ip[0]) && CFG(ssid[0]))

#if ASYNC_WEB
#   define REQ_PARAM       RQ_TYPE_AND_PARAM    (request)
#   define REQ_OBJECT     (*request)
#else
#   define REQ_PARAM       RQ_TYPE_AND_PARAM    (espxxsrv_t::WifiSrv->_esp)
#   define REQ_OBJECT   (espxxsrv_t::WifiSrv->_esp)
#endif


#endif // WIFISRV_H
