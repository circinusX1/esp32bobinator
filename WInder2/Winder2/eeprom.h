#ifndef EEPROM_C_H
#define EEPROM_C_H
#include <EEPROM.h>
//#include <SPIFFS.h>
#include "_utils.h"

//////////////////////////////////////////////////////////////////////////////////////////
#define APP_ADDR     1024
#define SENS_OR(x)  (1024+(1024*(x+1)))


#define          PACK_ALIGN_1   __attribute__((packed, aligned(1)))
struct config_t
{
    char            schedule[64];
    int8_t          faren;
    bool            hsta;
    uint8_t         trg;
    uint8_t         hrg;
    char            trigger_rule;
    uint8_t         sid;
    float           stunet[MAX_SENS*4];  // tweak at 4 points [0,30,60,100]
    float           stuneh[MAX_SENS*4];
    char            h_host[64];
    char            h_put[80];
    char            h_get[80];
    char            h_finger[62];
    short           h_port;
    char            mq_ttbroker[64];
    char            mq_topic[64];
    char            mq_user[16];
    char            mq_pass[9];
    short           mq_port;
    int             get_interval;
    long            ntp_offset=0;
    char            ntp_srv[24];
    char            ip[17];
    char            gw[17];
    char            mask[17];
    char            dnsa[17];
    char            dnsb[17];
    char            ssid[17];
    char            passwd[17];
    char            mcteck[17];
    char            mtkpsw[17];
    uint8_t         magic;
}PACK_ALIGN_1;

/////////////////////////////////////////////////////////////////////////////////////////
class   eeprom_t
{
public:
    eeprom_t(int reread=0):_reread(reread)
    {
        if(reread)
        {
            EEPROM.begin(4096);
            _get();
        }
    }
    ~eeprom_t()
    {
        if(_reread==1)
        {
            LOG("<-EP");
            _put();
        }
        if(_reread){
            EEPROM.end();
        }
    }
    void save(){
        _put();
        EEPROM.commit();
        delay(32);
        EEPROM.end();
        delay(32);
        _reread=0;
    }
    void load(){
        EEPROM.begin(4096);
        _get();
        EEPROM.end();
        _reread=0;
    }
    config_t* operator->(){return &Conf;}


    static void eprom_start()
    {
        LOG(__FUNCTION__);
        eeprom_t::Dirty=false;
        EEPROM.end();
        delay(32);
        EEPROM.begin(4096);
        delay(32);
        Offset = 1024;
        Start = 1024;
    }

    static void eprom_write(uint8_t byte)
    {
        eeprom_t::Dirty=true;
        if(Offset>=1024)
        {
            EEPROM.write(Offset, byte);
            Offset++;
            /*
            if(Offset-Start>1024)
            {
                EEPROM.commit();
                delay(32);
                EEPROM.end();
                delay(32);
                EEPROM.begin(1024);
                Start=Offset;
                LOG("Offset=%d",Offset);
            }
            */
        }
        else
        {
            LOG("! eprom_start ?");
        }
    }

    static uint8_t eprom_read()
    {
        if(Offset>=1024)
        {
            uint8_t by = EEPROM.read(Offset++);
            return by;
        }
        else
        {
            LOG("ERROR. eprom_start not called");
        }
        return 0;
    }

    static void eprom_writes(const uint8_t* buf, size_t bytes)
    {
        LOG("EP<-%d",bytes);
        for(int b=0;b<bytes;b++)
            eprom_write(*(buf+b));
    }

    static void eprom_reads(uint8_t* buf, size_t bytes)
    {
        LOG("EP->%d",bytes);
        for(int b=0;b<bytes;b++)
            *(buf+b) = eprom_read();
    }

    static void eprom_end()
    {
        if(eeprom_t::Dirty){
            EEPROM.commit();
            LOG("EP<-%d", Offset-Start);
        }else{
            LOG("EP->%d", Offset-Start);
        }
        EEPROM.end();
        Offset = 0;
        Start = 0;
        eeprom_t::Dirty=0;
    }

private:
    void _get()
    {
        size_t  magicoff = sizeof(config_t)-1;
        LOG("reading magic at address %d", magicoff);
        uint8_t magic = EEPROM.read(magicoff);
        LOG("magic is %X", magic);

        if( magic==MAGIC)
        {
            LOG("EP=OK");
            uint8_t* p = (uint8_t*)&Conf;
            for (size_t i = 0; i < sizeof(Conf); i++)
                *(p+i) = EEPROM.read(i);
        }
        else
        {
            LOG("EP=0 %hhu",  magic);
            _put();
        }
    }

    void _put()
    {
        LOG("EP<-%d", sizeof(Conf));
        Conf.magic = MAGIC;
        const uint8_t* p = (uint8_t*)&Conf;
        for (size_t i = 0; i <  sizeof(Conf); i++)
            EEPROM.write(i, *(p+i));

        size_t  magicoff = sizeof(config_t)-1;
        uint8_t magic = EEPROM.read(magicoff);
    }
    int             _reread = 0;

public:
    static config_t Conf;
    static  int     Offset;
    static  bool    Dirty;
    static int      Start;
};

//////////////////////////////////////////////////////////////////////////////////////////
#define CFG(x)    eeprom_t::Conf.x
void def_factory();


#endif // EEPROM_C_H
