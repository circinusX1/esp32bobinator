/**
   TestEspApi by Max Vilimpoc
   This code is released to the public domain.

   Test out the Expressif ESP8266 Non-OS API, exhaustively trying out
   as many of the built-in functions as possible.

   Some of the code is based on examples in:
   "20A-ESP8266__RTOS_SDK__Programming Guide__EN_v1.3.0.pdf"
*/

#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif
#include <Wire.h>
#include <SoftwareSerial.h>

// Set up output serial port (could be a SoftwareSerial
// if really wanted).

//void ehSerial(Serial);

// Wired to the blue LED on an ESP-01 board, active LOW.
//
// Cannot be used simultaneously with Serial.print/println()
// calls, as TX is wired to the same pin.
//
// UNLESS: You swap the TX pin using the alternate pinout.
const uint8_t LED_PIN = 1;

const char* const RST_REASONS[] = { "REASON_DEFAULT_RST", "REASON_WDT_RST", "REASON_EXCEPTION_RST", "REASON_SOFT_WDT_RST", "REASON_SOFT_RESTART", "REASON_DEEP_SLEEP_AWAKE", "REASON_EXT_SYS_RST" };

const char* const FLASH_SIZE_MAP_NAMES[] = { "FLASH_SIZE_4M_MAP_256_256", "FLASH_SIZE_2M", "FLASH_SIZE_8M_MAP_512_512", "FLASH_SIZE_16M_MAP_512_512", "FLASH_SIZE_32M_MAP_512_512", "FLASH_SIZE_16M_MAP_1024_1024", "FLASH_SIZE_32M_MAP_1024_1024" };

const char* const OP_MODE_NAMES[]{ "NULL_MODE", "STATION_MODE", "SOFTAP_MODE", "STATIONAP_MODE" };

const char* const AUTH_MODE_NAMES[]{ "AUTH_OPEN", "AUTH_WEP", "AUTH_WPA_PSK", "AUTH_WPA2_PSK", "AUTH_WPA_WPA2_PSK", "AUTH_MAX" };

const char* const PHY_MODE_NAMES[]{ "", "PHY_MODE_11B", "PHY_MODE_11G", "PHY_MODE_11N" };

const char* const EVENT_NAMES[]{ "EVENT_STAMODE_CONNECTED", "EVENT_STAMODE_DISCONNECTED", "EVENT_STAMODE_AUTHMODE_CHANGE", "EVENT_STAMODE_GOT_IP", "EVENT_SOFTAPMODE_STACONNECTED", "EVENT_SOFTAPMODE_STADISCONNECTED", "EVENT_MAX" };

const char* const EVENT_REASONS[]{
  "",
  "REASON_UNSPECIFIED",
  "REASON_AUTH_EXPIRE",
  "REASON_AUTH_LEAVE",
  "REASON_ASSOC_EXPIRE",
  "REASON_ASSOC_TOOMANY",
  "REASON_NOT_AUTHED",
  "REASON_NOT_ASSOCED",
  "REASON_ASSOC_LEAVE",
  "REASON_ASSOC_NOT_AUTHED",
  "REASON_DISASSOC_PWRCAP_BAD",
  "REASON_DISASSOC_SUPCHAN_BAD",
  "REASON_IE_INVALID",
  "REASON_MIC_FAILURE",
  "REASON_4WAY_HANDSHAKE_TIMEOUT",
  "REASON_GROUP_KEY_UPDATE_TIMEOUT",
  "REASON_IE_IN_4WAY_DIFFERS",
  "REASON_GROUP_CIPHER_INVALID",
  "REASON_PAIRWISE_CIPHER_INVALID",
  "REASON_AKMP_INVALID",
  "REASON_UNSUPP_RSN_IE_VERSION",
  "REASON_INVALID_RSN_IE_CAP",
  "REASON_802_1X_AUTH_FAILED",
  "REASON_CIPHER_SUITE_REJECTED",
};

const char* const EVENT_REASONS_200[]{ "REASON_BEACON_TIMEOUT", "REASON_NO_AP_FOUND" };

void print_system_info()
 {
  const rst_info* resetInfo = system_get_rst_info();
  Serial.print(F("system_get_rst_info() reset reason: "));
  Serial.println(RST_REASONS[resetInfo->reason]);

  Serial.print(F("system_get_free_heap_size(): "));
  Serial.println(system_get_free_heap_size());

  Serial.print(F("system_get_os_print(): "));
  Serial.println(system_get_os_print());
  system_set_os_print(1);
  Serial.print(F("system_get_os_print(): "));
  Serial.println(system_get_os_print());

  system_print_meminfo();

  Serial.print(F("system_get_chip_id(): 0x"));
  Serial.println(system_get_chip_id(), HEX);

  Serial.print(F("system_get_sdk_version(): "));
  Serial.println(system_get_sdk_version());

  Serial.print(F("system_get_boot_version(): "));
  Serial.println(system_get_boot_version());

  Serial.print(F("system_get_userbin_addr(): 0x"));
  Serial.println(system_get_userbin_addr(), HEX);

  Serial.print(F("system_get_boot_mode(): "));
  Serial.println(system_get_boot_mode() == 0 ? F("SYS_BOOT_ENHANCE_MODE") : F("SYS_BOOT_NORMAL_MODE"));

  Serial.print(F("system_get_cpu_freq(): "));
  Serial.println(system_get_cpu_freq());

  Serial.print(F("system_get_flash_size_map(): "));
  Serial.println(FLASH_SIZE_MAP_NAMES[system_get_flash_size_map()]);
}
