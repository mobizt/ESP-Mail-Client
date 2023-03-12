/**
 * Created March 1, 2023
 */

#ifndef ESP_TCP_CLIENTS_H
#define ESP_TCP_CLIENTS_H

#include <Arduino.h>
#include "./ESP_Mail_FS.h"
#include "./extras/MB_MCU.h"

#if defined(ENABLE_SMTP) || defined(ENABLE_IMAP)

#ifdef ENABLE_CUSTOM_CLIENT
#define ESP_MAIL_ENABLE_CUSTOM_CLIENT
#endif

#if !defined(ESP32) && !defined(ESP8266) && !defined(MB_ARDUINO_PICO) && !(defined(MB_ARDUINO_ARCH_SAMD) && !defined(ARDUINO_SAMD_MKR1000) || defined(MB_ARDUINO_NANO_RP2040_CONNECT)) && !defined(MB_ARDUINO_NANO_RP2040_CONNECT)
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

#elif defined(ESP8266) || defined(MB_ARDUINO_PICO)

#include <ESP8266WiFi.h>
#include "esp8266/ESP8266_TCP_Client.h"
#define ESP_MAIL_TCP_CLIENT ESP8266_TCP_Client

#elif defined(MB_ARDUINO_ARCH_SAMD) && !defined(ARDUINO_SAMD_MKR1000) || defined(MB_ARDUINO_NANO_RP2040_CONNECT)

// To do:
// - Add support board SSL engine for external basic client
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT)
#include "custom/Custom_TCP_Client.h"
#define ESP_MAIL_TCP_CLIENT Custom_TCP_Client
#else
#include "samd/WiFiNINA_TCP_Client.h"
#define ESP_MAIL_TCP_CLIENT WiFiNINA_TCP_Client
#endif

#else

#include "custom/Custom_TCP_Client.h"
#define ESP_MAIL_TCP_CLIENT Custom_TCP_Client

#endif

#endif

#endif

#endif /* ESP_CLIENTS_H */
