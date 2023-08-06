#ifndef ESP_MAIL_NETWORKS_PROVIDER_H
#define ESP_MAIL_NETWORKS_PROVIDER_H

#include "../ESP_Mail_FS.h"

#include "../ESP_Mail_Client_Version.h"
#if !VALID_VERSION_CHECK(30400)
#error "Mixed versions compilation."
#endif

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_RASPBERRY_PI_PICO_W) || __has_include(<WiFiNINA.h>) ||__has_include(<WiFi101.h>)

#define ESP_MAIL_WIFI_IS_AVAILABLE

#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#endif

#if defined(ESP32) && __has_include(<ETH.h>)
#include <ETH.h>
#endif

#if __has_include(<WiFiNINA.h>) || __has_include(<WiFi101.h>)
#define ESP_MAIL_HAS_WIFI_TIME
#endif

#if defined(MB_ARDUINO_PICO) && __has_include(<WiFiMulti.h>)
#define ESP_MAIL_HAS_WIFIMULTI
#endif

#endif


#if defined(TINY_GSM_MODEM_SIM800) || \
defined(TINY_GSM_MODEM_SIM808) || \
defined(TINY_GSM_MODEM_SIM868) || \
defined(TINY_GSM_MODEM_SIM900) || \
defined(TINY_GSM_MODEM_SIM7000) || \
defined(TINY_GSM_MODEM_SIM7000) || \
defined(TINY_GSM_MODEM_SIM7000SSL) || \
defined(TINY_GSM_MODEM_SIM7070) || \
defined(TINY_GSM_MODEM_SIM7080) || \
defined(TINY_GSM_MODEM_SIM7090) || \
defined(TINY_GSM_MODEM_SIM5320) || \
defined(TINY_GSM_MODEM_SIM5360) || \
defined(TINY_GSM_MODEM_SIM5300) || \
defined(TINY_GSM_MODEM_SIM7100) || \
defined(TINY_GSM_MODEM_SIM7600) || \
defined(TINY_GSM_MODEM_SIM7800) || \
defined(TINY_GSM_MODEM_SIM7500) || \
defined(TINY_GSM_MODEM_UBLOX) || \
defined(TINY_GSM_MODEM_SARAR4) || \
defined(TINY_GSM_MODEM_M95) || \
defined(TINY_GSM_MODEM_BG96) || \
defined(TINY_GSM_MODEM_A6) || \
defined(TINY_GSM_MODEM_A7) || \
defined(TINY_GSM_MODEM_M590) || \
defined(TINY_GSM_MODEM_MC60) || \
defined(TINY_GSM_MODEM_MC60E) || \
defined(TINY_GSM_MODEM_XBEE) || \
defined(TINY_GSM_MODEM_SEQUANS_MONARCH)
#define ESP_MAIL_TINYGSM_IS_AVAILABLE
#endif

#if defined(ESP_MAIL_TINYGSM_IS_AVAILABLE) && __has_include(<TinyGsmClient.h>)
#include <TinyGsmClient.h>
#define ESP_MAIL_HAS_TINYGSM
#endif

#if defined(ESP_MAIL_WIFI_IS_AVAILABLE) && !defined(ESP_MAIL_NOT_USE_ONBOARD_WIFI)
#define WiFI_CONNECTED (WiFi.status() == WL_CONNECTED)
#else
#define WiFI_CONNECTED false
#endif

#endif