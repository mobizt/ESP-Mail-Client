#pragma once

#ifndef ESP_MAIL_CONFIG_H
#define ESP_MAIL_CONFIG_H

#include <Arduino.h>
#include "extras/MB_MCU.h"

/**
 * To use other flash file systems
 *
 * LittleFS File system
 *
 * #include <LittleFS.h>
 * #define ESP_MAIL_DEFAULT_FLASH_FS LittleFS //For ESP8266 LitteFS
 *
 *
 * FAT File system
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

/** Use PSRAM for supported ESP32/ESP8266 module */
#if defined(ESP32) || defined(ESP8266)
#define ESP_MAIL_USE_PSRAM
#endif

/**
 * To use SD card file systems with different hardware interface
 * e.g. SDMMC hardware bus on the ESP32
 * https://github.com/espressif/arduino-esp32/tree/master/libraries/SD#faq
 *
 #include <SD_MMC.h>
 #define ESP_MAIL_DEFAULT_SD_FS SD_MMC //For ESP32 SDMMC
 #define ESP_MAIL_CARD_TYPE_SD_MMC 1
 *
*/

/**
 * To use SdFat on ESP32 and other devices except for ESP8266

#include <SdFat.h> //https://github.com/greiman/SdFat
static SdFat sd_fat_fs;   //should declare as static here
#define ESP_MAIL_DEFAULT_SD_FS sd_fat_fs
#define ESP_MAIL_CARD_TYPE_SD 1
#define ESP_MAIL_SD_FS_FILE SdFile


* The SdFat (https://github.com/greiman/SdFat) is already implemented as wrapper class in ESP8266 core library.
* Do not include SdFat.h library in ESP8266 target code which it conflicts with the wrapper one.
#include <SD.h>
#define ESP_MAIL_DEFAULT_SD_FS SD
#define ESP_MAIL_CARD_TYPE_SD 1
*/

#if defined(ESP32) || defined(ESP8266)
#include <SD.h>
#define ESP_MAIL_DEFAULT_SD_FS SD
#define CARD_TYPE_SD 1
#endif

// For ESP32, format SPIFFS or FFat if mounting failed
#define ESP_MAIL_FORMAT_FLASH_IF_MOUNT_FAILED 1

#ifdef ESP_MAIL_DEBUG_PORT
#define ESP_MAIL_DEFAULT_DEBUG_PORT ESP_MAIL_DEBUG_PORT
#endif

#ifndef ESP_MAIL_DEFAULT_DEBUG_PORT
#define ESP_MAIL_DEFAULT_DEBUG_PORT Serial
#endif

// Enable IMAP class
#define ENABLE_IMAP // comment this line to disable or exclude it

// Enable SMTP class
#define ENABLE_SMTP // comment this line to disable or exclude it

//#define ENABLE_CUSTOM_CLIENT

#endif