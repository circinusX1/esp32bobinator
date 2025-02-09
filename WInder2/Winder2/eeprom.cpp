#include "eeprom.h"


config_t eeprom_t::Conf = {
    "1:30-2:20 4:00:23:00",
    0,
    0,
    40,
    32,
    1,
    0,
    {0},
    {0},
    "meeiot.mine.nu",
    "put/5bdd8abe7b78305b7f4be6d52b850053362f945e5a0f915347a425/",              // remote
    "get/5bdd8abe7b78305b7f4be6d52b850053362f945e5a0f915347a425/",              // remote
    "6D F9 FD AA A1 60 42 96 0E C5 29 D5 79 60 07 1F 90 21 4D 99",                                     // finger
    443,     // host port
    "",
    "",
    "",
    "",
    0,
    30,     // seconds
    -5,
    "pool.ntp.org",
    "",                 // ip
    "192.168.1.1",      // gw
    "255.255.255.0",    // mask
    "8.8.8.8",
    "4.4.4.4",
    "",
    "",
    "mcteckh2",
    "mariusss",
    MAGIC,
};

void def_factory()
{
    static config_t local = {
        "1:30-2:20 4:00:23:00",
        0,
        0,
        40,
        32,
        1,
        0,
        {0},
        {0},
        "meeiot.mine.nu",
        "put/5bdd8abe7b78305b7f4be6d52b850053362f945e5a0f915347a425/",              // remote
        "get/5bdd8abe7b78305b7f4be6d52b850053362f945e5a0f915347a425/",              // remote
        "6D F9 FD AA A1 60 42 96 0E C5 29 D5 79 60 07 1F 90 21 4D 99",                                     // finger
        443,     // host port
        "",
        "",
        "",
        "",
        0,
        30,     // seconds
        -5,
        "pool.ntp.org",
        "",                 // ip
        "192.168.1.1",      // gw
        "255.255.255.0",    // mask
        "8.8.8.8",
        "4.4.4.4",
        "",
        "",
        "mcteck2",
        "mariusss",
        MAGIC,
    };
    memcpy(&eeprom_t::Conf,&local,sizeof(config_t));
}

bool eeprom_t::Dirty = false;
int eeprom_t::Offset = 0;
int eeprom_t::Start = 0;
