#ifndef CLOCK_T_H
#define CLOCK_T_H

#include <time.h>
#include "_utils.h"
#include "eeprom.h"
#include "hot_ram.h"
#define USE_US_TIMER // microsecond timer
#include <osapi.h>

class MyNTP : public NTPClient
{
public:
    MyNTP(UDP& udp, const char* poolServerName, int timeOffset, int updateInterval):
        NTPClient(udp, poolServerName, timeOffset, updateInterval){}

    int getYear() {
        time_t rawtime = this->getEpochTime();
        struct tm * ti;
        ti = localtime (&rawtime);
        int year = ti->tm_year + 1900;

        return year;
    }

    int getMonth() {
        time_t rawtime = this->getEpochTime();
        struct tm * ti;
        ti = localtime (&rawtime);
        int month = (ti->tm_mon + 1) < 10 ? 0 + (ti->tm_mon + 1) : (ti->tm_mon + 1);

        return month;
    }

    int getDate() {
        time_t rawtime = this->getEpochTime();
        struct tm * ti;
        ti = localtime (&rawtime);
        int day = (ti->tm_mday) < 10 ? 0 + (ti->tm_mday) : (ti->tm_mday);

        return day;
    }

    String getFullFormattedTime() {
        time_t rawtime = this->getEpochTime();
        struct tm * ti;
        ti = localtime (&rawtime);

        uint16_t year = ti->tm_year + 1900;
        String yearStr = String(year);

        uint8_t month = ti->tm_mon + 1;
        String monthStr = month < 10 ? "0" + String(month) : String(month);

        uint8_t day = ti->tm_mday;
        String dayStr = day < 10 ? "0" + String(day) : String(day);

        uint8_t hours = ti->tm_hour;
        String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

        uint8_t minutes = ti->tm_min;
        String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

        uint8_t seconds = ti->tm_sec;
        String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

        return yearStr + "-" + monthStr + "-" + dayStr + " " +
                hoursStr + ":" + minuteStr + ":" + secondStr;
    }
};


class simple_clock_t
{
    uint64_t _microS;
    uint64_t _micros;
public:
    simple_clock_t(){
        _microS = micros64();
    };
    ~simple_clock_t(){
#if WITH_NTP
        delete _ntp;
#endif
    };
    //_micros = micros64()-_microS;
    void re_ntp()
    {
        if(_ntp)
        {
            _ntp->end();
            delete _ntp;
            _ntp=nullptr;
            init_ntp();
            update_ntp(true);
        }
    }

    bool try_update_ntp();

    void tick(){
        ++_decisec;
        if(_decisec>=10)  // a sec passed
        {
            _decisec = 0;
            ++_second;
            ++_epochlocal;
            ++_daysec;
            if(_second>=60) // a min passed
            {
                _second=0;
                ++_minute;
                ++_minday;
                if(_minday>=MINS_PER_DAY){_minday=0;}
                if(_minute>=60) //  hour passed
                {
                    _minute = 0;
                    ++_hour;
                    fire_hour_event();
                    if(_hour>=24) // day passes
                    {
                        _hour    = 0;
                        _minute  = 0;
                        _daysec  = 0;
                        _decisec = 0;
                        _second  = 0;
                        _minday  = 0;
                        fire_day_event();
                    }
                }
            }
        }
    }

    int seconds()const{return _daysec;}
    int decisec()const{return _decisec;}
    int minutes()const{return _minday;}
    int hours()const{return _hour;}
    String get_time(){
        char l[32];
        sprintf(l,"%02dH:%02dm",_hour,_minute);
        LOG("DT = %s\n", l);
        return String(l);
    }

    FORCE_INLINE unsigned long diff_time(size_t now, size_t start) {
        if(0==start) return 0xFFFFFFFF;         // edge condition, low prpbbility
        if (now >= start) return now - start;
        else return 0xFFFFFFFF - start + now + 1;
    }
    FORCE_INLINE unsigned long msElapsed(size_t start){
        unsigned long now = ::millis();
        return diff_time(now, start);
    }


    void set_seconds(int seconds){
        _daysec   = seconds;
        _minday   = seconds/60;
        _second   = seconds % 60;
        _hour     = seconds/3600;
        _minute   = (seconds%3600)/60;
        LOG("NTP: H%d:M%d:D%d  DM%d DS%d", _hour, _minute, _second, _minday, _daysec);
    }

    void init_ntp()
    {
#if WITH_NTP
        if(Ramm.ntpfail==false &&
                _ntp == nullptr && CFG(ntp_srv[0]))
        {
            Ramm.ntpfail = true;
            _ntp = new MyNTP(_ntpUDP,CFG(ntp_srv),CFG(ntp_offset)*3600, 600000);
            _ntp->begin();
            Ramm.ntpfail = false;
            LOG("NTP OF %d", CFG(ntp_offset)*3600);
        }
#endif
    }
    unsigned long getEpochTime()
    {
        return _ntp?_ntp->getEpochTime():0;
    }
    unsigned long ntp_epoch_secs()
    {
        return _epochlocal;
    }
    unsigned long year_day()
    {
        return _epochlocal/(24*3600);
    }
    void fire_day_event();
    void fire_hour_event();
    bool update_ntp(bool force=false);
    void    sync_micros(int on)
    {
        // system_print_meminfo();
        // LOG("ull=%d, max days=%llu chipid=%d rtc=%d",
        //     sizeof(uint64_t),
        //     ((int64_t)(-1))/86400000000);
        // _microS = micros64();
        // size_t xm = millis();
        // LOG("   RTC = %lld", system_get_rtc_time());
        // LOG("   MICROS = %lld %lld", _microS, xm);
        // delay(1);
        // LOG("   1MS = %lld",  micros64()-_microS);
        // delay(100);
        // LOG("   100MS = %lld",  micros64()-_microS);
    }
    String full_date_time()
    {
        if(_ntp)
            return _ntp->getFullFormattedTime();
        return String("NTP=0");
    }
    static String to_ddhms(unsigned long secs)
    {
/*
        time_t seconds(secs);            // you have to convert your secs into time_t
        struct tm *p = gmtime(&seconds); // convert to broken down time
        char out[32];
        sprintf(out, "%03d-%02d:%02d:%02d",p->tm_yday,p->tm_hour,p->tm_min,p->tm_sec);
*/
        char out[64];
        const int cseconds_in_day = 86400;
        const int cseconds_in_hour = 3600;
        const int cseconds_in_minute = 60;
        const int cseconds = 1;
        unsigned long long days = secs / cseconds_in_day;
        unsigned long hours = (secs % cseconds_in_day) / cseconds_in_hour;
        unsigned long minutes = ((secs % cseconds_in_day) % cseconds_in_hour) / cseconds_in_minute;
        unsigned long seconds = (((secs % cseconds_in_day) % cseconds_in_hour) % cseconds_in_minute) / cseconds;

        sprintf(out, "%03d %02d:%02d:%02d",days,hours,minutes,seconds);
        return String(out);
    }
    void reroute_event();

private:
    int             _second=0;
    int             _minute=0;
    int             _hour=0;
    int             _daysec=0;
    int             _decisec=0;
    int             _minday=0;
    int             _day=0;
    int             _month=0;
    int             _year=0;
public:
    unsigned long   _epochlocal=0;
    String         _resettitme;
    unsigned long   _ntplast=0;
#if WITH_NTP
    MyNTP*          _ntp = nullptr;
    WiFiUDP         _ntpUDP;
#endif
};

extern simple_clock_t  Sclk;

#endif // CLOCK_T_H
