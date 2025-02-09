#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "espxxsrv.h"
#include "eeprom.h"
#include "clock_t.h"
#include "application.h"
#include "css.h"
#include "libraries/ESP8266Ping/src/ESP8266Ping.h"


#define LAST_PING_INTERVAL  60000
/////////////////////////////////////////////////////////////////////////////////////////
static const unsigned long CON_RETRY_SEC   = 30;    // wifi client losy con retry
static const unsigned long MAX_FAILS       = 10;    // max retry client then goes to AP

/////////////////////////////////////////////////////////////////////////////////////////
espxxsrv_t*     espxxsrv_t::WifiSrv;
bool            espxxsrv_t::hot = false;
bool            espxxsrv_t::_faren = false;

//////////////////////////////////////////////////////////////////////////////////////////
espxxsrv_t::espxxsrv_t():_esp(80)
{
    espxxsrv_t::WifiSrv = this;

    static IPAddress ip;   ip.fromString("10.5.5.1");
    static IPAddress mask; mask.fromString("255.255.0.0");
    LOG("AP=1");
    WiFi.softAPConfig(ip, ip, mask);
    if(CFG(mcteck)[0]==0)
    {
        ::strcpy(CFG(mcteck),"mcteckh2");
        ::strcpy(CFG(mtkpsw),"mariusss");
    }
    WiFi.softAP(CFG(mcteck), CFG(mtkpsw));
    Serial.printf("Credentials %s/%s\r\n",CFG(mcteck), CFG(mtkpsw));
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    _esp.onNotFound(espxxsrv_t::handleUser);
    _esp.on("/", HTTP_GET, espxxsrv_t::handleRoot);
    _esp.on("/index.html", espxxsrv_t::handleRoot);
    _esp.on("/fetch", espxxsrv_t::handleFetch);

    _esp.on("/wifi", espxxsrv_t::handleWifi);
    _esp.on("/cssp", espxxsrv_t::handleCss);
    _esp.on("/js", espxxsrv_t::handleJs);
    _esp.on("/error", espxxsrv_t::handleError);
    _esp.on("/sensor", espxxsrv_t::handleSensor);
    _esp.on("/wifisave", espxxsrv_t::handleWifiSave);
    _esp.on("/config", espxxsrv_t::handleConfig);
    _esp.on("/configsave", espxxsrv_t::handleConfigSave);
    _esp.on("/ota", espxxsrv_t::handleOta);
    _esp.on("/time", espxxsrv_t::handleTime);
    _esp.on("/graph", espxxsrv_t::handleGraph);
#if ASYNC_WEB
    _esp.on("/check", [](RQ_TYPE_AND_PARAM){LOG("CHK");RQ_OBJECT.send(200, "text/html", "OK");});
    _esp.on("/favico.ico", [&](RQ_TYPE_AND_PARAM) {
        RQ_OBJECT.send(200,"text/html","");
    });
    _esp.on("/factory", [](RQ_TYPE_AND_PARAM) {
        LOG("/reset page");
        app_t::TheApp->factory();
    });
    _esp.on("/fileup", HTTP_POST, [](RQ_TYPE_AND_PARAM) {
        RQ_OBJECT.send(200);
    },espxxsrv_t::handleUpload);
#else
    _esp.on("/check", [](){LOG("CHK");espxxsrv_t::WifiSrv->_esp.send(200, "text/html", "OK");});
    _esp.on("/favico.ico", [&]() {
        _esp.send(200,"text/html","");
    });
    _esp.on("/detect", [&]() {
        app_t::TheApp->detect();
    });
    _esp.on("/factory", []() {
        LOG("factory");
        app_t::TheApp->factory();
    });
    _esp.on("/fileup", HTTP_POST, []() {
        app_t::TheApp->reboot_board(Update.hasError());
    }, []() {
        HTTPUpload& upload = espxxsrv_t::WifiSrv->_esp.upload();
        if (upload.status == UPLOAD_FILE_START) {
            espxxsrv_t::WifiSrv->_otaing = true;
            app_t::TheApp->_ae._flash_writes++;
            ::memset(&Ramm,0,sizeof(Ramm));
            hot_store(true);
            app_t::TheApp->save();
            Serial.printf("Update: %s\n", upload.filename.c_str());
            if(strncmp(upload.filename.c_str(),"esp_mtek",8) ||
                    !strstr(upload.filename.c_str(),".bin"))
            {
                LOG("Update: wrong filename");
                app_t::TheApp->reboot_board(1);
            }
            if (!Update.begin(900000)) { //start with max available size
                Update.printError(Serial);
            }
            espxxsrv_t::WifiSrv->_bytes = 0;

        } else if (upload.status == UPLOAD_FILE_WRITE) {
            static bool toggle=false;
            espxxsrv_t::WifiSrv->_bytes = upload.totalSize;
            //LOG("so far %d ", upload.totalSize);
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
            {
                Update.printError(Serial);
            }
            app_t::TheApp->led_ctrl(toggle=!toggle);
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
            if (Update.end(true)) { //true to set the size to the current progress
                Serial.printf("\nUpdate Success: %u rebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
void  espxxsrv_t::end()
{
    _sta = _ap = false;
    WiFi.softAPdisconnect (true);
    WiFi.disconnect();
}

/////////////////////////////////////////////////////////////////////////////////////////
void  espxxsrv_t::begin()
{
    _esp.begin();
    delay(32);
    _ap  = false;
    _sta = false;
    _con_time = 0;
    _sta_lost = 0;
    _con_time = Sclk.seconds();
    if(SSID_CONF())
    {
        LOG("CLI->");
        _connect_cli();
    }
    else
    {
        LOG("STA=0");
        _sta_lost = MAX_FAILS;
    }
    _began = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::loop()
{
    bool show=false;
    if(!_began){
        return;
    }
    if(_sta_lost>0)
    {
        if(Sclk.diff_time(Sclk.seconds(),_con_time) > CON_RETRY_SEC)
        {
            --_sta_lost;
            LOG("STA=%d/0",_sta_lost);
            show=true;
            if(_sta_lost>0)
            {
                LOG("CLI-->");
                _connect_cli();
                _sta = true;
            }
            else
            {
                LOG("AP=1");
                _enable_ap();
                _sta_lost = 0;
                _sta = false;
            }
            _con_time = Sclk.seconds();
        }
    }

    int s = WiFi.status();
    if(s != _wstate)
    {
        show=true;
        LOG("WIFI=%d", s);
        _wstate = s;
        if(_sta)
        {
            if (s != WL_CONNECTED)
            {
                LOG("WIFI=0");
                WiFi.disconnect();
                if(_sta_lost==0)
                {
                    LOG("STA!=%d", MAX_FAILS);
                    _sta_lost = MAX_FAILS;
                }
                app_t::TheApp->on_wifi_state(false);

            }
            else
            {
                LOG("STA=1");
                _sta_lost = 0;
                _sta = true;
                app_t::TheApp->on_wifi_state(true);
            }
        }
        // no  config
        if(!SSID_CONF())
        {
            LOG("SSID=0");
            if(_ap==false)
            {
                LOG("AP=1");
                _enable_ap();
                _sta       = false;
                _sta_lost  = 0;
            }
        }
    }
    if(_dnsServer != nullptr)
    {
        _dnsServer->processNextRequest();
    }
#if !ASYNC_WEB
    _esp.handleClient();
#endif
    if(_otaing){
        return;
    }
    if(sta_connected())
    {
        _schedule();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::_connect_cli()
{
    if(SSID_CONF())
    {
        LOG(">CON %s %s", CFG(ssid), CFG(ip));
        LOG(">DNS=%s DNS=%s", CFG(dnsa), CFG(dnsb));
        LOG(">GW=%s MSK=%s", CFG(gw), CFG(mask));
        LOG(">CON WIFI...");
        if(WiFi.status() == WL_CONNECTED){ WiFi.disconnect();}
        IPAddress ip;       ip.fromString(CFG(ip));
        IPAddress gw;       gw.fromString(CFG(gw));
        IPAddress mask;     mask.fromString(CFG(mask));
        WiFi.begin(CFG(ssid), CFG(passwd));
        IPAddress dns1;     dns1.fromString(CFG(dnsa));
        IPAddress dns2;     dns2.fromString(CFG(dnsb));
        WiFi.hostname(CFG(mcteck));
        _sta = true;
        if (!WiFi.config(ip, gw, mask, dns1, dns2))
        {
            LOG("! WIFI=ERR");
            goto WIFI_STA_ERR;
        }
        else
        {
            _wstate = WiFi.waitForConnectResult();
            LOG("Connection Result: %d", _wstate);
            Serial.printf("ip:%s gw:%s sn:%s dns:%s dns:%s \r\n", CFG(ip), CFG(gw),
                          CFG(mask), CFG(dnsa), CFG(dnsb));
            if(_wstate == WL_CONNECTED)
            {
                LOG("CON=%s,%s", CFG(ssid), CFG(ip));
                _con_retry = 0;

                if(CFG(hsta)==false)
                {
                    _disable_ap();
                }
                else
                {
                    _enable_ap();
                }

                Serial.print("DNSS: ");
                WiFi.dnsIP().printTo(Serial);
                Serial.print(", ");
                WiFi.dnsIP(1).printTo(Serial);
                Serial.println();
                delay(100);
                _sta = true;
                _sta_lost = 0;
                app_t::TheApp->on_wifi_state(true);

            }
            else
            {
                app_t::TheApp->on_wifi_state(false);
                goto WIFI_STA_ERR;
            }
        }
    }
    return;
WIFI_STA_ERR:
    if(_sta_lost!=0)
    {
        LOG("CON!=%s, RETRY", CFG(ssid));
    }
    else
    {
        LOG("CON!=%s, AP->1", CFG(ssid));
    }
    if(WiFi.status() == WL_CONNECTED){ WiFi.disconnect();}
    _wstate = -1;
    TRACE();
}

/////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::_start_dns()
{
    LOG(__FUNCTION__);
    if(nullptr==_dnsServer)
    {
        IPAddress APIP(10, 5, 1, 1);
        _dnsServer = new DNSServer();
        _dnsServer->start(53, "*", APIP);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::_stop_dns()
{
    LOG(__FUNCTION__);
    if(_dnsServer)
        delete _dnsServer;
    _dnsServer=nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::_disable_ap()
{
    LOG("Closing AP 10.5.5.1");
    _stop_dns();
    WiFi.softAPdisconnect (true);
    WiFi.mode(WIFI_STA);
    _ap = false;

}

/////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::_enable_ap()
{
    LOG("AP=10.5.5.1");

    WiFi.enableAP(true);
    if(WiFi.status() == WL_CONNECTED)
        WiFi.mode(WIFI_AP_STA);
    else
        WiFi.mode(WIFI_AP);
    _start_dns();
    _ap=true;
}

/////////////////////////////////////////////////////////////////////////////////////////
bool espxxsrv_t::ping(const char* host)
{
    if(WifiSrv->sta_connected())
    {
        if(host && *host && Ping.ping(host,2))
        {
            return true;
        }
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::_schedule(const char* value)
{
    return;
    if(_wstate!=WL_CONNECTED)
    {
        return;
    }
    else{
        eeprom_t e;

        size_t diff = Sclk.diff_time(Sclk.seconds(),_last_remote_get);
        if(value || diff > (CFG(get_interval) + _delay))
        {
            LOG("CLIENT SCHEDULE FOR VALUE = %s", value ? value : "NULL");
            _last_remote_get = Sclk.seconds();

            if(e->h_get[0])                                 /// remote commands
            {
                strcpy(e->h_host,"enjoydecor.com");
                e->h_finger[0]=0;
                e->h_port=80;

                String              line,url;
                WiFiClient*         pclient = nullptr;
                const char*         host = e->h_host;
                short               nport = 80, np;
                HTTPClient          https;

                ESP.resetFreeContStack();

                if(e->h_finger[0] && host[0])
                {
                    pclient = new BearSSL::WiFiClientSecure();
                    char finger[64] = {0};
                    for(int i=0;e->h_finger[i];i++)
                    {
                        if(e->h_finger[i]==':')finger[i]=' ';
                        else finger[i]=e->h_finger[i];
                    }
                    nport = 443;
                    if(e->h_finger[0])
                    {
                        Serial.printf("FP: %s\r\n", finger);
                        if (!((WiFiClientSecure*)pclient)->setFingerprint(e->h_finger))
                        {
                            goto DIONE;
                        }
                    }
                }
                else
                {
                    pclient = new WiFiClient();
                    nport = 80;
                }
                np =  e->h_port ? e->h_port : nport;
                url = host;  if(!url.endsWith("/")) url+="/";
                if(value == nullptr)                           // we get if any is remote
                {
                    url += e->h_get;
                    if(url.endsWith("/"))  url+="?RELAY=";
                    else if(url.indexOf('&')!=-1) url+="&RELAY=";
                    url += String(app_t::TheApp->get_relay());
                }
                else
                {
                    url += e->h_put;
                    if(url.endsWith("/"))  url+="?";
                    else if(url.indexOf('&')!=-1) url+="&";
                    url += value;
                }
                Serial.println(url.c_str());
                LOG(" on %s port %d", e->h_host, np);
                if(https.begin(*pclient, e->h_host, np, url))
                {
                    int httpCode = https.GET();
                    if (httpCode > 0) {
                        // HTTP header has been send and Server response header has been handled
                        Serial.printf("[HTTPS] GET... code: %d\r\n", httpCode);

                        // file found at server
                        if (httpCode == HTTP_CODE_OK ||
                                httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                        {
                            String payload = https.getString();
                            Serial.println(payload);
                        }
                        _delay=0;
                    } else {
                        Serial.printf("CON=0%s\r\n", https.errorToString(httpCode).c_str());
                        bool ret = Ping.ping(e->h_host);
                        LOG("PING %s = %d", e->h_host, ret);
                        _delay=30;
                    }
                    https.end();
                }
DIONE:
                pclient->stop();
                uint32_t freeStackEnd = ESP.getFreeContStack();
                delete pclient;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::on_relay_event(const sensdata_t* value)
{
    char svalue[32];

    ::sprintf(svalue,"RELAY=%d",int(value->u.uc));
    _schedule(svalue);
}

//////////////////////////////////////////////////////////////////////////////////////////
void espxxsrv_t::on_sensor_event(const sensor_th_t* sensor)
{
#if I2C_SDA
    char    svalue[32];
    const   th_t& th = sensor->get_th();
    ::sprintf(svalue,"TEMP=%f4.2&HUM=%4.2f",th.t, th.h);
    _schedule(svalue);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// time:Rl:Temp:Hum:Temp:Hum...
void espxxsrv_t::_handleFetch(RQ_TYPE_AND_PARAM)
{
#if I2C_SDA
    String page;
    int  mysecond = Sclk.minutes() / TIME_STEP;
    page += String(mysecond)+":";
    page += String(app_t::TheApp->get_relay(mysecond))+":";
    const sensors_t& ss =  app_t::TheApp->sensors();
    for(int s = 0; s< MAX_SENS; s++)
    {
        const sensor_th_t* th = ss.sensor(s);
        if(th)
        {
            const th_t& thh =  th->get_th();
            page += String(int(thh.t))+":";
            page += String(int(thh.h))+":";
        }
        else
        {
            page += "0:0:";
        }
    }
    LOG("FH=%s",page.c_str());
#if ASYNC_WEB
    request->send(200, "text/html",page);
#else
    _esp.send(200, "text/html", page);
#endif
#endif
}

#if ASYNC_WEB
void espxxsrv_t::handleUpload(AsyncWebServerRequest *request, String filename,
                              size_t index, uint8_t *data, size_t len, bool final)
{
    if(!index){
        espxxsrv_t::WifiSrv->_otaing = true;
        app_t::TheApp->_ae._flash_writes++;
        app_t::TheApp->save();
        Serial.printf("Update: %s\n", filename.c_str());
        if(strncmp(filename.c_str(),"esp_mtek",8) ||
                !strstr(filename.c_str(),".bin"))
        {
            LOG("Update: wrong filename");
            app_t::TheApp->reboot_board(1);

        }
        if (!Update.begin(900000)) { //start with max available size
            Update.printError(Serial);
        }
        espxxsrv_t::WifiSrv->_bytes = 0;
    }
    {
        static bool toggle=false;
        if(len)
        {
            espxxsrv_t::WifiSrv->_bytes += len;
            if (Update.write(data, len) != len)
            {
                Update.printError(Serial);
            }
            app_t::TheApp->led_ctrl(toggle=!toggle);
        }
    }
    if(final)
    {
        if (Update.end(true)) { //true to set the size to the current progress
            Serial.printf("\nUpdate Success: %u rebooting...\n", espxxsrv_t::WifiSrv->_bytes);
        } else {
            Update.printError(Serial);
        }
    }
}
#endif

