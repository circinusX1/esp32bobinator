#include "_my_config.h"
#include "_utils.h"
#include "hot_ram.h"

ram_str_t    Ramm;
bool         Dirty;

#define  RTC_MEM_START 64

void hot_store(bool force)
{
    if(Ramm.loops<=OKAY_MAX_LOOPS || force)
    {
        if(Ramm.loops==OKAY_MAX_LOOPS)
        {
            Ramm.loops++;
        }
        Ramm.sig = MAGIC;
        ::system_rtc_mem_write(RTC_MEM_START, &Ramm, sizeof(Ramm));
    }
}

bool hot_restore()
{
    ::system_rtc_mem_read(RTC_MEM_START, &Ramm, sizeof(Ramm));
    if(Ramm.sig != MAGIC)
    {
        LOG("RAM=0");
        memset(&Ramm,0,sizeof(Ramm));
        Ramm.sig = MAGIC;
        ::system_rtc_mem_write(RTC_MEM_START, &Ramm, sizeof(Ramm));
    }
    else
    {
        LOG("<-RAM");
    }
    return Ramm.sig == MAGIC;
}
