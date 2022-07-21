

/**
 *  The Supported Protocols based on the devices and usage opttions
 *
 *  |:: Devices and Usage options ::::::::::::::::::::::|:::::::: Plain ::::::::|:::::::: SSL ::::::::|:::::::: TLS ::::::::|
 *
 *      ESP8266/ESP32 or                                          ✔️                      ✔️                   ✔️
 *      SAMD21/RPI2040 + Custom NINA Firmware
 *
 *      ESP8266/ESP32 + External basic Client (4)                 ✔️(1)                   ✔️(1)                ✔️(1)
 *
 *      ESP8266/ESP32 + External SSL Client                       ❌ or ✔️(2)             ✔️                   ❌ or ✔️(3)
 *
 *      SAMD21/RPI2040 w/o Custom NINA Firmware                   ✔️                      ✔️                   ❌
 *
 *      SAMD21/RPI2040 + External basic Client                    ✔️(1)                   ❌                   ❌
 *
 *      SAMD21/RPI2040 + External SSL Client                      ❌ or ✔️(2)             ✔️                   ❌ or ✔️(3)
 *
 *      Other Arduino devices (5) + External basic Client         ✔️(1)                   ❌                   ❌
 *
 *      Other Arduino devices (5) + External SSL Client           ❌ or ✔️(2)             ✔️                   ❌ or ✔️(3)
 *
 *
 *
 *  Notes:
 *  1) Required connection callback function.
 *  2) Required connection callback function and use SSL Client that supports plain text connection e.g. https://github.com/mobizt/SSLClient
 *  3) Required both connection callback function and connection upgrade callback function and use SSL Client that supports connection upgrade e.g. https://github.com/mobizt/SSLClient
 *  4) If macro ESP_MAIL_USE_SDK_SSL_ENGINE was assigned.
 *  5) Arduino Devices with at least 80 k program memory
 *
 */

#pragma once

#ifndef ESP_MAIL_CONFIG_H
#define ESP_MAIL_CONFIG_H

#include <Arduino.h>
#include "extras/MB_MCU.h"


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
 * #define ESP_MAIL_DEFAULT_FLASH_FS LittleFS //For ESP8266 LitteFS
 *
 *
 * ::::::: To use FAT Filesystem :::::::
 *
 * #include <FFat.h>
 * #define ESP_MAIL_DEFAULT_FLASH_FS FFat  //For ESP32 FAT
 *
 */
#if defined(ESP32) || defined(ESP8266)
#if defined(ESP32)
#include <SPIFFS.h>
#endif
#define ESP_MAIL_DEFAULT_FLASH_FS SPIFFS
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



/* 📌 ESP8266 W5100 Ethernet module Enable compilation option */
// #define ENABLE_ESP8266_W5100_ETH



/** 📌 ESP8266/ESP32 SSL engine for basic Client compilation option
 *
 * This macro allows library to use ESP8266 and ESP32 devices with
 * basic Clients (EthernetClient, WiFiClient and GSMClient)
 * directly without external SSL client required.
 */
#define ESP_MAIL_USE_SDK_SSL_ENGINE

#endif
