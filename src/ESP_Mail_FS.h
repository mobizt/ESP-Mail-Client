

#pragma once

#ifndef ESP_MAIL_CONFIG_H
#define ESP_MAIL_CONFIG_H

#include "ESP_Mail_Client_Version.h"
#if !VALID_VERSION_CHECK(30400)
#error "Mixed versions compilation."
#endif

#include <Arduino.h>
#include "extras/MB_MCU.h"

/** 📌 Predefined Options
 * ⛔ Use following build flag to disable all predefined options.
 * -D DISABLE_ALL_OPTIONS or -DDISABLE_ALL_OPTIONS in PlatformIO
 */

/**📍 Enable the NTP server time reading
 * ⛔ Use following build flag to disable.
 * -D DISABLE_NTP_TIME or -DDISABLE_NTP_TIME in PlatformIO
 */
#define ENABLE_NTP_TIME

/**📍 Enable the error string from error reason
 * ⛔ Use following build flag to disable.
 * -D DISABLE_ERROR_STRING or -DDISABLE_ERROR_STRING in PlatformIO
 */
#define ENABLE_ERROR_STRING

/**📍 Enable IMAP class compilation option
 * ⛔ Use following build flag to disable.
 * -D DISABLE_IMAP or -DDISABLE_IMAP in PlatformIO
 */
#define ENABLE_IMAP // comment this line to disable or exclude it

/**📍 Enable SMTP class compilation option
 * ⛔ Use following build flag to disable.
 * -D DISABLE_SMTP or -DDISABLE_SMTP in PlatformIO
 */
#define ENABLE_SMTP // comment this line to disable or exclude it

/**📍 PSRAM compilation option for ESP32/ESP8266 module
 * ⛔ Use following build flag to disable.
 * -D DISABLE_PSRAM or -DDISABLE_PSRAM in PlatformIO
 */
#define ESP_MAIL_USE_PSRAM

/**📌 Flash Filesystem compilation options
 *
 * 📍 For SPIFFS
 * #define ESP_MAIL_DEFAULT_FLASH_FS SPIFFS
 *
 *
 * 📍 For LittleFS Filesystem
 * #include <LittleFS.h>
 * #define ESP_MAIL_DEFAULT_FLASH_FS LittleFS
 *
 *
 * 📍 For SPIFFS Filesystem
 * #if defined(ESP32)
 * #include <SPIFFS.h>
 * #endif
 * #define ESP_MAIL_DEFAULT_FLASH_FS SPIFFS
 *
 *
 * 📍 For FAT Filesystem
 * #include <FFat.h>
 * #define ESP_MAIL_DEFAULT_FLASH_FS FFat  //For ESP32 FAT
 *
 * 🚫 Use following build flags to disable.
 * -D DISABLE_FLASH or -DDISABLE_FLASH in PlatformIO
 */

#if defined(ESP32) || defined(ESP8266) || defined(MB_ARDUINO_PICO)

#if defined(ESP8266) || defined(MB_ARDUINO_PICO)

#include <LittleFS.h>
#define ESP_MAIL_DEFAULT_FLASH_FS LittleFS

#elif defined(ESP_ARDUINO_VERSION) /* ESP32 core >= v2.0.x */ /* ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2, 0, 0) */

#include <LittleFS.h>
#define ESP_MAIL_DEFAULT_FLASH_FS LittleFS

#else

#include <SPIFFS.h>
#define ESP_MAIL_DEFAULT_FLASH_FS SPIFFS

#endif

#endif

// For ESP32, format SPIFFS or FFat if mounting failed
#define ESP_MAIL_FORMAT_FLASH_IF_MOUNT_FAILED 1

/**📌 SD Filesystem compilation options
 *
 * 📍 For SD
 * #include <SD.h>
 * #define ESP_MAIL_DEFAULT_SD_FS SD
 * #define ESP_MAIL_CARD_TYPE_SD 1
 *
 * 📍 For SD MMC (ESP32)
 * #include <SD_MMC.h>
 * #define ESP_MAIL_DEFAULT_SD_FS SD_MMC //For ESP32 SDMMC
 * #define ESP_MAIL_CARD_TYPE_SD_MMC 1
 *
 * 📍 For SdFat on ESP32 and other devices except for ESP8266
 * #include <SdFat.h> //https://github.com/greiman/SdFat
 * static SdFat sd_fat_fs;   //should declare as static here
 * #define ESP_MAIL_DEFAULT_SD_FS sd_fat_fs
 * #define ESP_MAIL_CARD_TYPE_SD 1
 * #define ESP_MAIL_SD_FS_FILE SdFile
 *
 *
 * ⛔ Use following build flags to disable.
 * -D DISABLE_SD or -DDISABLE_SD in PlatformIO
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

/** 🔖 Optional macros
 *
 * 🏷️ To enable silent mode (no debug printing and callback)
 * #define SILENT_MODE
 *
 * 🏷️ To use ESP8266 ENC28J60 Ethernet module
 * #define ENABLE_ESP8266_ENC28J60_ETH
 *
 * 🏷️ To use ESP8266 W5500 Ethernet module
 * #define ENABLE_ESP8266_W5500_ETH
 *
 * 🏷️ To use ESP8266 W5100 Ethernet module
 * #define ENABLE_ESP8266_W5100_ETH
 *
 * 🏷️ To disable on-board WiFi
 * #define ESP_MAIL_DISABLE_ONBOARD_WIFI
 *
 * 🏷️ To disable native Ethernet (Ethernet interfaces that supported by SDK)
 * #define ESP_MAIL_DISABLE_NATIVE_ETHERNET
 *
 * 🏷️ To disable SSL
 * #define ESP_MAIL_DISABLE_SSL
 *
 * 🏷️ To assign debug port
 * #define ESP_MAIL_DEFAULT_DEBUG_PORT Serial
 */

#if __has_include("Custom_ESP_Mail_FS.h")
#include "Custom_ESP_Mail_FS.h"
#endif

#include "extras/Options.h"

#endif
