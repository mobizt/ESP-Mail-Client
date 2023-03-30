

#pragma once

#ifndef ESP_MAIL_CONFIG_H
#define ESP_MAIL_CONFIG_H

#include "ESP_Mail_Client_Version.h"
#if !VALID_VERSION_CHECK(30105)
#error "Mixed versions compilation."
#endif

#include <Arduino.h>
#include "extras/MB_MCU.h"

/* 📌 Enable silent mode (no debug printing and callback) */
// #define SILENT_MODE

/* 📌 Enable the NTP server time synching */
#define ENABLE_NTP_TIME

/* 📌 Enable the error string from error reason */
#define ENABLE_ERROR_STRING

/* 📌 Enable IMAP class compilation option */
#define ENABLE_IMAP // comment this line to disable or exclude it

/* 📌 Enable SMTP class compilation option */
#define ENABLE_SMTP // comment this line to disable or exclude it

/* 📌 PSRAM compilation option for ESP32/ESP8266 module */
#if defined(ESP32) || defined(ESP8266)
#define ESP_MAIL_USE_PSRAM
#endif

/** 📌Flash Filesystem compilation options
 *
 * ::::::: To use SPIFFS :::::::
 *
 * #define ESP_MAIL_DEFAULT_FLASH_FS SPIFFS
 *
 *
 * ::::::: To use LittleFS Filesystem :::::::
 *
 * #include <LittleFS.h>
 * #define ESP_MAIL_DEFAULT_FLASH_FS LittleFS
 *
 *
 * ::::::: To use SPIFFS Filesystem :::::::
 *
 * #if defined(ESP32)
 * #include <SPIFFS.h>
 * #endif
 * #define ESP_MAIL_DEFAULT_FLASH_FS SPIFFS
 *
 *
 * ::::::: To use FAT Filesystem :::::::
 *
 * #include <FFat.h>
 * #define ESP_MAIL_DEFAULT_FLASH_FS FFat  //For ESP32 FAT
 *
 */
#if defined(ESP32) || defined(ESP8266) || defined(MB_ARDUINO_PICO)

#if defined(ESP8266) || defined(MB_ARDUINO_PICO)
// Use LittleFS as default flash filesystem for ESP8266

#include <LittleFS.h>
#define ESP_MAIL_DEFAULT_FLASH_FS LittleFS

#elif defined(ESP_ARDUINO_VERSION) /* ESP32 core >= v2.0.x */ /* ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2, 0, 0) */
// Use LittleFS as default flash filesystem for ESP32 core v2.0.x

#include <LittleFS.h>
#define ESP_MAIL_DEFAULT_FLASH_FS LittleFS

#else
// Use SPIFFS as default flash filesystem for ESP32 core v1.0.6 and earlier

#include <SPIFFS.h>
#define ESP_MAIL_DEFAULT_FLASH_FS SPIFFS

#endif

#endif

// For ESP32, format SPIFFS or FFat if mounting failed
#define ESP_MAIL_FORMAT_FLASH_IF_MOUNT_FAILED 1

/** 📌 SD Filesystem compilation options
 *
 * ::::::: To use SD SPI interface :::::::
 *
 * #include <SD.h>
 * #define ESP_MAIL_DEFAULT_SD_FS SD
 * #define ESP_MAIL_CARD_TYPE_SD 1
 *
 * ::::::: To Use SD MMC interface on ESP32 :::::::
 *
 * #include <SD_MMC.h>
 * #define ESP_MAIL_DEFAULT_SD_FS SD_MMC //For ESP32 SDMMC
 * #define ESP_MAIL_CARD_TYPE_SD_MMC 1
 *
 * ::::::: To use SdFat on ESP32 and other devices except for ESP8266 :::::::
 *
 * #include <SdFat.h> //https://github.com/greiman/SdFat
 * static SdFat sd_fat_fs;   //should declare as static here
 * #define ESP_MAIL_DEFAULT_SD_FS sd_fat_fs
 * #define ESP_MAIL_CARD_TYPE_SD 1
 * #define ESP_MAIL_SD_FS_FILE SdFile
 *
 * The SdFat (https://github.com/greiman/SdFat) is already implemented as wrapper class in ESP8266 core library.
 * Do not include SdFat.h library in ESP8266 target code which it conflicts with the wrapper one.
 */
#if defined(ESP32) || defined(ESP8266)
#include <SD.h>
#define ESP_MAIL_DEFAULT_SD_FS SD
#define ESP_MAIL_CARD_TYPE_SD 1
#elif defined(MB_ARDUINO_PICO)
// Use SDFS (ESP8266SdFat) instead of SD
#include <SDFS.h>
#define ESP_MAIL_DEFAULT_SD_FS SDFS
#define ESP_MAIL_CARD_TYPE_SD 1
#endif

/* 📌 Debug port compilation option */
#ifdef ESP_MAIL_DEBUG_PORT
#define ESP_MAIL_DEFAULT_DEBUG_PORT ESP_MAIL_DEBUG_PORT
#endif

#ifndef ESP_MAIL_DEFAULT_DEBUG_PORT
#define ESP_MAIL_DEFAULT_DEBUG_PORT Serial
#endif

/** 📌 External Client Enable compilation option
 *
 * This macro allows library to use external basic Client and external SSL Client interface.
 * The associated callback functions should be assigned based on port functions.
 */
// #define ENABLE_CUSTOM_CLIENT

/* 📌 To use ESP8266 ENC28J60 Ethernet module */
// #define ENABLE_ESP8266_ENC28J60_ETH

/* 📌 To use ESP8266 W5500 Ethernet module */
// #define ENABLE_ESP8266_W5500_ETH

/* 📌 To use ESP8266 W5100 Ethernet module */
// #define ENABLE_ESP8266_W5100_ETH

/** 📌 ESP8266/ESP32/RP2040 SSL engine for basic Client compilation option
 *
 * This macro allows library to use ESP8266 and ESP32 devices with
 * basic Clients (EthernetClient, WiFiClient and GSMClient)
 * directly without external SSL client required.
 */
#define ESP_MAIL_USE_SDK_SSL_ENGINE

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::://
// You can create your own header file "Custom_ESP_Mail_FS.h" in the same diectory of
// "ESP_Mail_FS.h" and put your own custom config to overwrite or
// change the default config in "ESP_Mail_FS.h".
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::://

/** This is an example of "Custom_ESP_Mail_FS.h"

#pragma once

#ifndef Custom_ESP_Mail_FS_H
#define Custom_ESP_Mail_FS_H

// Use custom client instead of internal client
#define ENABLE_CUSTOM_CLIENT // define to use custom client

// Use LittleFS instead of SPIFFS
#include "LittleFS.h"
#undef DEFAULT_FLASH_FS // remove Flash FS defined macro
#define DEFAULT_FLASH_FS LittleFS

// Use SD_MMC instead of SD
#if defined(ESP32)
#include <SD_MMC.h>
#undef ESP_MAIL_DEFAULT_SD_FS // remove SD defined macro
#undef ESP_MAIL_CARD_TYPE_SD_MMC // remove SD defined macro
#define ESP_MAIL_DEFAULT_SD_FS SD_MMC
#define ESP_MAIL_CARD_TYPE_SD_MMC 1
#endif


#endif

*/
#if __has_include("Custom_ESP_Mail_FS.h")
#include "Custom_ESP_Mail_FS.h"
#endif

#endif
