
#include "application.h"
#include "hot_ram.h"
#include "sens_aht20.h"  //debug granph only
#include "css.h"

app_t* app_t::TheApp;
#define RELAY_DEBOUNCE      30000
#define CHECK_NTP           100000
#define ACTIVE_LOOP_DELAY   10000
#define EPROM_SIG_APP       MAGIC

//////////////////////////////////////////////////////////////////////////////////////////
bool	GlobalError = false;
bool    GTest = false;
bool    Running = false;

//////////////////////////////////////////////////////////////////////////////////////////
app_t::app_t()
{
    eeprom_t eprom(-1);                                     // load
    hot_restore();
    Serial.begin(SERIAL_BPS);
    if(Ramm.loops && Ramm.loops<OKAY_MAX_LOOPS)             // prev run
    {
        LOG("! RUN ERR");
        GlobalError=true;
    }
    LOG("LOOP=%d",Ramm.loops);
}

//////////////////////////////////////////////////////////////////////////////////////////
app_t::~app_t(){
    _wifi->end();
    delete _wifi;
}

//////////////////////////////////////////////////////////////////////////////////////////
void app_t::begin()
{
    app_t::TheApp = this;

#if I2C_SDA
    _sensors.begin(I2C_SDA, I2C_SCL);
#endif

    if(Ramm.timer==false)
    {
        Ramm.timer=true;
        hot_store();
        if(CFG(hrg)==101){                                  // time test 10x faster
            LOG("T=1");
            _timer.attach_ms(1, app_t::tick_10);
        }
        else{
            LOG("T=100");
            _timer.attach_ms(100, app_t::tick_10);
        }
        Ramm.timer=false;
        hot_store();
    }

    _wifi = new espxxsrv_t();
    _wifi->begin();
}

/////////////////////////////////////////////////////////////////////////////////////////
void app_t::loop()
{
    _wifi->loop();
    if(GlobalError){ digitalWrite(LED,LED_ON); return;}                      // no init

    if(!_init)
    {
        _init = true;
        on_init();
    }

    if(_wifi && _wifi->is_otaing())
    {
        return;
    }

    if(_tenth)
    {
        _tenth=false;
        if(Sclk.decisec() % _wifi->mode() == 0)
        {
            _led_ctrl(_led_state=!_led_state);
        }
    }

    if(Sclk.diff_time(millis(), _last_active_loop) > ACTIVE_LOOP_DELAY)
    {
        _last_active_loop = millis();
        if(_wifi)
        {
            if(_wifi->mode()==1)
            {
                LOG("AP: 10.5.5.1");
                LOG(CFG(mcteck));
            }
        }
#if I2C_SDA
        int looped = _sensors.loop();
#endif
        if(_wifi->sta_connected())
        {
            Sclk.update_ntp();
        }
        if(_day_changed)
        {
            _day_changed=false;
            on_day_changed(false);
        }
        else if(_lasthour != Sclk.hours())
        {
            _lasthour = Sclk.hours();
            LOG("H=%d",_lasthour);
            if(_lasthour % 6 == 0)
            {
                // this->save();
            }
        }
    }

    Ramm.loops++;
    hot_store();
}

/////////////////////////////////////////////////////////////////////////////////////////
void app_t::on_init()
{
    LED ? pinMode(LED, OUTPUT) : __nill();
    RELAY ? pinMode(RELAY, OUTPUT) : __nill();

    Sclk.init_ntp();
    LOG("APP OKAY");
    Running = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
void app_t::led_ctrl(bool rm)
{
    app_t::TheApp->_led_ctrl(rm);
}

/////////////////////////////////////////////////////////////////////////////////////////
void app_t::on_wifi_state(bool state)
{
    LOG("WIFI=%d",state);
    if(state)
    {
        Sclk.update_ntp(true);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
void app_t::on_web_event(const sensdata_t* val)
{
    LOG(__FUNCTION__);
    _inevent = true;
    _inevent = false;
}

//////////////////////////////////////////////////////////////////////////////////////////
void app_t::tick_10()
{
    Sclk.tick();
    TheApp->_tick10();
}

//////////////////////////////////////////////////////////////////////////////////////////
void app_t::_tick10()
{
    _tenth = true;
}

////////////////////////////////////////////////////////////////////////////////////////
void app_t::_led_ctrl(bool rm)
{
    digitalWrite(LED, rm ? LED_ON : LED_OFF);
}


//////////////////////////////////////////////////////////////////////////////////////////
void app_t::reboot_board(int err, const String* replacement)
{
    String page;
    if(replacement)
        page=*replacement;
    else
    {
        page  = get_redirect();
        page += F("<p hidden id='ip'>");
        if(_wifi->sta_connected())
        {
#if ASYNC_WEB
            page += espxxsrv_t::toStrIp(WiFi.localIP());
#else
            page += espxxsrv_t::toStrIp(REQ_OBJECT.client().localIP());
#endif
        }
        else
        {
            page += espxxsrv_t::toStrIp(WiFi.softAPIP());
        }
        page += F("</p>");
        if(err==0)
        {
            page += "<h1>Please wait, Board rebooting <p id='cd'></p><br></h1>";
        }
        else
        {
            page += "<h1><font color='red'>ERROR !</font>. Please wait <div style='display:inline' id='cd'></div></h1>";
        }
    }
    page+= "<hr><br /><div class='red'>Do not refresh !</div></br><hr>";
    _wifi->send(page.c_str());
    _wifi->flush();
    delay(128);
    cli();
    do{ static void (*_JMPZERO)(void)=nullptr; (_JMPZERO)(); } while(0);
    for(;;); //wd reset
}


//////////////////////////////////////////////////////////////////////////////////////////
void app_t::_sav_data()
{
    _ae._reboots++;
    _ae._eprom_writes++;
    eeprom_t::eprom_writes(reinterpret_cast<const uint8_t*>(&_ae), sizeof(app_cfg_t));
}

//////////////////////////////////////////////////////////////////////////////////////////
void app_t::_load_data()
{
    eeprom_t::eprom_reads(reinterpret_cast<uint8_t*>(&_ae), sizeof(app_cfg_t));
}

//////////////////////////////////////////////////////////////////////////////////////////
void app_t::load()
{
    LOG("EP->APP");
    eeprom_t::eprom_start();
    uint8_t sig = eeprom_t::eprom_read();
    if(sig==EPROM_SIG_APP)
    {
        LOG("APP<-EP");
        _load_data();
    }
    else
    {
        LOG("EP<-APP");
        _ae._eprom_writes++;
        _sav_data();
    }
    eeprom_t::eprom_end();
    _ae._reboots++;
}

//////////////////////////////////////////////////////////////////////////////////////////
void app_t::save()
{
    LOG("SAVE");
    _ae._eprom_writes++;
    eeprom_t::eprom_start();
    eeprom_t::eprom_write(EPROM_SIG_APP);
    _sav_data();
    eeprom_t::eprom_end();
}

//////////////////////////////////////////////////////////////////////////////////////////
void app_t::factory()
{
    do{
        eeprom_t e(1);
        def_factory();
        e.save();
    }while(0);

    memset(&Ramm,0,sizeof(Ramm));
    hot_store(true);
    memset(_ae._uptime,0,sizeof(_ae._uptime));
    this->save();
    this->reboot_board();
}


void app_t::detect()
{
    String page = espxxsrv_t::start_htm();
    ::memset(&Ramm,0,sizeof(Ramm));
    hot_store();
    sensors_t::detect(page);
    if(espxxsrv_t::ping(CFG(gw)))
        page+="<li>ROUTER OKAY";

    if(espxxsrv_t::ping(CFG(ntp_srv))){
        page+="<li>INET OKAY";
        Sclk.update_ntp();
    }
    espxxsrv_t::end_htm(page);

}
