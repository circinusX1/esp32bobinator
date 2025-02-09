#ifndef APPLICATION_C_H
#define APPLICATION_C_H


#include "eeprom.h"
#include "hot_ram.h"
#include "espxxsrv.h"
#include "clock_t.h"
#if  WITH_GRAPH
#include "canvas.h"
#endif

#define DAYS_IN_YEAR 366

enum{
    TODAYY=0,
    YESTERDAY,
    MONTH,
    LASTMONTH,
    YEAR,
    LASTYEAR,
    FOREVER,
    DATES
};

/////////////////////////////////////////////////////////////////////////////////////////
struct app_cfg_t
{
    size_t          _eprom_writes = 0;
    size_t          _flash_writes = 0;
    size_t          _reboots = 0;
    char            _reason[24]={0};
    struct{
        unsigned long s;
        unsigned long tot;
        }           _uptime[DATES];
    uint8_t         _year[DAYS_IN_YEAR];
};

/////////////////////////////////////////////////////////////////////////////////////////
class app_t
{
public:

    app_t();
    virtual ~app_t();

    virtual void begin();
    virtual void loop();
    virtual void on_wifi_state(bool state);

    virtual void on_web_event(const sensdata_t* ev);
    virtual void on_relay_event(const sensdata_t* ev);
    virtual void on_button_event(const sensdata_t* ev);
    virtual void on_sensor_event(const sensor_th_t* ps);

    virtual void on_relay_control(int sid, float temp, float hum);
    virtual void on_day_changed(bool fromINT);
    virtual void on_month_changed();
    virtual void on_year_changed();
    virtual void on_hour_changed(bool fromINT);

#if ASYNC_WEB
    virtual void http_get(RQ_TYPE_AND_PARAM, String& page)=0;
#else
    virtual void http_get(MyWebSrv* ps, String& page)=0;
#endif
    virtual void on_init();
    bool get_relay(int daymin=-1)const;
    static void pet(){ESP.wdtFeed();}
    static void led_ctrl(bool rm);
#if I2C_SDA
    const sensors_t& sensors()const{return _sensors;}
    int   sens_count(){return _sensors.count();}
#endif
    void reboot_board(int err=0, const String* replacement=nullptr);
    void save();
    void load();
    void convert_graph(bool from, bool to);
    void reset_graph()
    {
#if  WITH_GRAPH
        _canvas.reset();
#endif
    }
    void reset_hours(){
        memset(_ae._uptime,0,sizeof(_ae._uptime));
        save();
    }
    void factory();
    void canvas_it();
    void detect();
public:
    static void tick_10();

public:
    static       app_t* TheApp;
#if  WITH_GRAPH
    canvas          _canvas;
#endif
protected:
    void _tick10();
    void _relay_ctrl(bool rm);
    void _led_ctrl(bool rm);
    void _draw_canvas(String& page);
    void _sav_data();
    void _load_data();
protected:
    app_cfg_t       _app_cfg;
    espxxsrv_t*     _wifi = nullptr;

    Ticker          _timer;
    bool            _led_state=false;
    bool            _relay_state=0;
    bool            _inevent=false;
    bool            _init = false;

    bool            _tenth = true;
    size_t          _decisecs;
    size_t          _last_active_loop;
    int             _temp = 0;
    size_t          _lastrelay_ctrl = 0;
    size_t          _buttonstate[2]={0,0};
    volatile bool   _day_changed = false;
    int             _lasthour = 0;


public:

    app_cfg_t      _ae;

};


struct my_custom_app_desc_t{
    char name[16];
} ;

extern bool	GlobalError;
extern bool Running;
#endif // APPLICATION_C_H
