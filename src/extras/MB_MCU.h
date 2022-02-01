#ifndef MB_MCU_H
#define MB_MCU_H

#if defined(ESP8266) || defined(ESP32)
#define MB_MCU_ESP 1
#endif

#if defined(__arm__)
#define MB_MCU_ARM 2
#endif

#if defined(ARDUINO_ARCH_SAMD)
#define MB_MCU_ATMEL_ARM 3
#endif

#if defined(ARDUINO_NANO_RP2040_CONNECT)
#define MB_MCU_RP2040 4
#endif

#if defined(TEENSYDUINO)
#define MB_MCU_TEENSY_ARM 5
#endif



#endif