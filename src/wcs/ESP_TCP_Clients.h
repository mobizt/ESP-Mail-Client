/**
 * Created July 20, 2022
 */

#ifndef ESP_TCP_CLIENTS_H
#define ESP_TCP_CLIENTS_H

#include "./ESP_Mail_FS.h"

#ifdef ENABLE_CUSTOM_CLIENT
#define ESP_MAIL_ENABLE_CUSTOM_CLIENT
#endif

#if !defined(ESP32) && !defined(ESP8266) && !(defined(MB_MCU_ATMEL_ARM) && !defined(ARDUINO_SAMD_MKR1000) || defined(MB_MCU_RP2040)) && !defined(MB_MCU_RP2040)
#ifndef ESP_MAIL_ENABLE_CUSTOM_CLIENT
#define ESP_MAIL_ENABLE_CUSTOM_CLIENT
#endif
#endif

#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && !defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
#include "custom/Custom_TCP_Client.h"
#define ESP_MAIL_TCP_CLIENT Custom_TCP_Client
#else

#if defined(ESP32)

#include "esp32/ESP32_TCP_Client.h"
#define ESP_MAIL_TCP_CLIENT ESP32_TCP_Client

#elif defined(ESP8266)

#include <ESP8266WiFi.h>
#include "esp8266/ESP8266_TCP_Client.h"
#define ESP_MAIL_TCP_CLIENT ESP8266_TCP_Client

#elif defined(MB_MCU_ATMEL_ARM) && !defined(ARDUINO_SAMD_MKR1000) || defined(MB_MCU_RP2040)

#include "samd/WiFiNINA_TCP_Client.h"
#define ESP_MAIL_TCP_CLIENT WiFiNINA_TCP_Client

#else

#include "custom/Custom_TCP_Client.h"
#define ESP_MAIL_TCP_CLIENT Custom_TCP_Client

#endif

#endif

#endif /* ESP_CLIENTS_H */
