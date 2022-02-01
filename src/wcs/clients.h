/**
 * Created January 22, 2022
 */

#ifndef ESP_CLIENTS_H
#define ESP_CLIENTS_H

#include "./ESP_Mail_FS.h"

#if defined(ENABLE_CUSTOM_CLIENT)

#include "custom/Custom_Client.h"
#define TCP_CLIENT Custom_Client


#else

#if defined(ESP32)

#include "esp32/ESP32_TCP_Client.h"
#define TCP_CLIENT ESP32_TCP_Client

#elif defined(ESP8266)

#include <ESP8266WiFi.h>
#include "esp8266/ESP8266_TCP_Client.h"
#define TCP_CLIENT ESP8266_TCP_Client

#elif defined(MB_MCU_ATMEL_ARM) && !defined(ARDUINO_SAMD_MKR1000) || defined(MB_MCU_RP2040)

#include "samd/WiFiNINA_TCP_Client.h"
#define TCP_CLIENT WiFiNINA_TCP_Client

#else

#include "custom/Custom_Client.h"
#define TCP_CLIENT Custom_Client

#endif

#endif

#endif /* ESP_CLIENTS_H */
