
//#include "wirewire.h"
#include "application.h"
#include "systeminfo.h"
#include "css.h"
#include "sensors.h"
#include <pgmspace.h>

#define PGM_P       const char *
#define PGM_VOID_P  const void *
#define PSTR(s) (__extension__({static const char __c[] PROGMEM = (s); &__c[0];}))


/////////////////////////////////////////////////////////////////////////////////////////
class MyApp : public app_t
{
public:
    MyApp(){
        ::strncpy(_ae._reason,ESP.getResetInfo().c_str(),sizeof(_ae._reason)-1);
    }
    ~MyApp(){}

    void begin(){
        app_t::begin();
    }

    void loop(){
        app_t::loop();
    }
#if ASYNC_WEB
    void http_get(RQ_TYPE_AND_PARAM, String& page)
#else
    void http_get(ESP8266WebServer* request, String& page)
#endif
    {
        page = espxxsrv_t::start_htm();
#if ASYNC_WEB
        const String& uri = request->url();
#else
        const String& uri = request->uri();
#endif
        page += "<h1 class='header'>HOME</h1>";

        if(uri=="/i2c")
        {
            page += F("<form method='GET' action='/i2c'>"
                      "<li><input type='text' hidden name='i2c'/>"
                      "<li><input type='text' placeholder='SDA' name='sda'/>"
                      "<li><input type='text' placeholder='SCL  ' name='scl'/>"
                      "<li><input type='submit' style='float:right' value='Apply' /></form>");
            page += F("<hr>");
            if(request->hasArg("sda") && request->hasArg("scl"))
            {
#if I2C_SDA
                char sda[8] = {0};
                char scl[8] = {0};
                page += F("<pre>");
                request->arg("sda").toCharArray(sda,6);
                request->arg("scl").toCharArray(scl,6);
                if(::atoi(sda) && atoi(scl))
                {
                    LOG("I2C %d %d", ::atoi(sda), ::atoi(scl));
                    ::i2c_begin(::atoi(sda), atoi(scl));
                    ::i2c_scan(&page,nullptr);
                    page += F("</pre>");
                    ::i2c_end();
                }
#endif // #if I2C_SDA
            }
        }
        if(request->hasArg("canvas"))
            app_t::TheApp->canvas_it();
        if(request->hasArg("clean"))
        {
            Ramm.loopfail=0;
            Ramm.loops=0;
            hot_store();
            reboot_board();
            return;
        }
        if(request->hasArg("time"))
        {
            int md = request->arg("time").toInt();
            if(md > MINS_PER_DAY)md = MINS_PER_DAY;
            Sclk.set_seconds(md*60);
        }
        if(request->hasArg("test"))
        {
#if WITH_GRAPH
            static int Times=0;
            const sensor_th_t* p = app_t::TheApp->_sensors.sensor("SHT21");
            if(p)
                _canvas.add_th(Times,p,_relay_state);
            Times+=TIME_STEP;
            if(Times>SAMPLES_PER_DAY)
                Times=0;
#endif
        }

        if(request->hasArg("rentp"))
        {
            Sclk.re_ntp();
            delay(256);
        }

        if(request->hasArg("reboot"))
        {
            app_t::TheApp->save();
            app_t::TheApp->reboot_board();
        }
        if(request->hasArg("relay"))
        {
            if(request->arg("relay")=="1"){
                _relay_ctrl(true);
            }
            if(request->arg("relay")=="0"){
                _relay_ctrl(false);
            }
        }
        if(request->hasArg("gpio"))
        {
            int nLED = request->arg("gpio").toInt();
            pinMode(nLED, OUTPUT);
            digitalWrite(nLED, LED_OFF);
            delay (1000);
            digitalWrite(nLED, LED_ON);
            delay (1000);
            digitalWrite(nLED, LED_OFF);
        }
        page += F("<table width='100%'><tr>\n");
        page +=  F("<td colspan='2'><a href='?relay=1'><button id='r_on'>RELAY ON</button></a></td>\n");
        page +=  F("<td colspan='2'><a href='?relay=0'><button id='r_off'>RELAY OFF</button></a></td>\n");
#if I2C_SDA
        for(int i=0; i <MAX_SENS;i++)
        {
            const sensor_th_t* pt = _sensors.sensor(i);
            if(pt)
            {
                const th_t& th = pt->get_th();
                String unit;
                String unit2;
                if(CFG(faren)){
                    unit = F("F / ");
                    unit2 = F("F");
                }else{
                    unit = F("C / ");
                    unit2 = F("C");
                }
                if(i==CFG(sid))
                {
                    page +=  F("<tr><th class='green'>("); page+= pt->name(); page+=F(") Temp / Trig</th><td class='green'> ")+String(th.t)+unit +CFG(trg)+unit2+F("</td>");
                    page +=  F("<th  class='green'>(");     page+= pt->name(); page+=F(") Hum / Trig</th><td class='green'> ")+String(th.h)+"% / " +CFG(hrg)+F("%</td></tr>");
                }
                else
                {
                    page +=  F("<tr><th>("); page+= pt->name(); page+=F(") Temp</th><td> ")+String(th.t) +unit+F("</td>");
                    page +=  F("<th>(");     page+= pt->name(); page+=F(") Hum </th><td> ")+String(th.h)+ F("%</td></tr>");
                }
            }
        }
#endif // #if I2C_SDA
        page +=  F("<tr><th>H/T Logic</th><td>");
        page +=  get_logic(CFG(trigger_rule));
        page +=  F("</td>");

        page += F("<th>Sensors </th><td>");
        for(int i=0;i<MAX_SENS;i++)
        {
            const sensor_th_t* ps = _sensors.sensor(i);
            if(ps){
                page += F("<a href='/sensor?sens=");
                page += ps->name();
                page += F("'>");
                page += ps->name();
                page += F("</a>, ");
            }
            else
            {
                if(Ramm.sensfail)
                    page += " ! ";
                else
                    page +=" - ";
            }
        }
        uint32_t free = system_get_free_heap_size();
        page +=  F("</td></tr>");
        page+= F("<tr><th>Memory</th><td>")+String(free)+F("</td>");
        page+= F("<th>Device Time</th><td>")+Sclk.get_time()+F("</td></tr>");

        page += F("<tr><th>");
        if(_wifi->sta_connected()){
            page += F("STA connected to</th><td>");
            page += String(CFG(ip));
            page += F("</td>");
        }
        else
        {
            page += F("STA not connected</th><td>");
            page += F("...</td>");
        }
        page += F("<th>Access Point</th><td>");
        page += String(CFG(mcteck));
        page += F(" / 10.5.5.1");
        if(_wifi->ap_active()){
            page += F(" On");
        }else{
            page += F(" Off");
        }
        page += F("</td></tr></table>");
        page += F("<table><tr><th style='background:#112' colspan='7'>Time: ");
        page += Sclk.full_date_time() + " USAGE ";
        if(_wifi->sta_connected() && CFG(ntp_srv[0]))
        {
            page += F(" NTP ");
            if(espxxsrv_t::ping(CFG(ntp_srv))==false)
                page += F(" <a href='/?rentp'><font color='red'>NTP failed. Restart NTP</font></a>");
            else
                page += F("<font color='green'>Time in sync</font>");
        }
        else
        {
            page += F("NTP Off. (WIFI is Off or NTP is blank)");
        }

        page += F("</th></tr><th>Today</th><th>Yesterday</th><th>This month</th>"
                  "<th>Last month</th><th>This year</th><th>Last year</th><th>Forever</th></tr>");
        page += F("<tr><td>");
        page += simple_clock_t::to_ddhms(_ae._uptime[TODAYY].tot);
        page += "<br><sub>";
        page += String(_ae._uptime[TODAYY].tot);
        page += F("s</sub></td>");
        page += F("<td>");
        page += simple_clock_t::to_ddhms(_ae._uptime[YESTERDAY].tot);
        page += "<br><sub>";
        page += String(_ae._uptime[YESTERDAY].tot);
        page += "";
        page += F("s</sub></td>");
        page += F("<td>");
        page += simple_clock_t::to_ddhms(_ae._uptime[MONTH].tot);
        page += "<br><sub>";
        page += String(_ae._uptime[MONTH].tot);

        page += F("s</sub></td>");
        page += F("<td>");
        page += simple_clock_t::to_ddhms(_ae._uptime[LASTMONTH].tot);
        page += "<br><sub>";
        page += String(_ae._uptime[LASTMONTH].tot);

        page += F("s</sub></td>");
        page += F("<td>");
        page += simple_clock_t::to_ddhms(_ae._uptime[YEAR].tot);
        page += "<br><sub>";
        page += String(_ae._uptime[YEAR].tot);

        page += F("s</sub></td>");
        page += F("<td>");
        page += simple_clock_t::to_ddhms(_ae._uptime[LASTYEAR].tot);
        page += "<br><sub>";
        page += String(_ae._uptime[LASTYEAR].tot);

        page += F("s</sub></td>");
        page += F("<td>");
        page += simple_clock_t::to_ddhms(_ae._uptime[FOREVER].tot);
        page += "<br><sub>";
        page += String(_ae._uptime[FOREVER].tot);

        page += F("s</sub></td>");
        page += F("</tr></table>");
DONE:
        _draw_canvas(page);
        page += F("<br/><sub style='float:right;color:black;'>Build:")+
                String(__DATE__) + F("/") + String(__TIME__)+
                F(" Dev mins:") + String(Sclk.minutes());
        page += F(" Version:") + String( (VERSION>>16)&0xFF );
        page += F(".") + String( (VERSION>>8)&0xFF );
        page += F(".") + String( (VERSION)&0xFF );

        page+= "<table width='100%'><tr><td>";
        for( int i=0;i<DAYS_IN_YEAR;i++){
            if(_ae._year[i]){
                page+=String(i);
                page+=F(":");
                page+=String(int(_ae._year[i]));
                page+=F(" ");
            }
        }
        page+= "</td></tr></table>";

#if ASYNC_WEB
        espxxsrv_t::end_htm(page, true, RQ_PARAM);
#else
        espxxsrv_t::end_htm(page, true);
#endif
    }

    void on_init()
    {
        app_t::on_init();
    }
private:
};

/////////////////////////////////////////////////////////////////////////////////////////
MyApp* pApp;


/////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
    pApp = new MyApp();
    pApp->begin();
    LOG("BDT %s %s", __DATE__, __TIME__);
}

/////////////////////////////////////////////////////////////////////////////////////////
void loop()
{
    pApp->loop();
}


//////////////////////////////////////////////////////////////////////////////////////////
