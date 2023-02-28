#ifndef ESP_MAIL_PRINT_H_
#define ESP_MAIL_PRINT_H_

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_RP2040)

#define ESP_MAIL_PRINTF ESP_MAIL_DEFAULT_DEBUG_PORT.printf

#else

#include "extras/mb_print/mb_print.h"

extern "C" __attribute__((weak)) void
mb_print_putchar(char c)
{
    ESP_MAIL_DEFAULT_DEBUG_PORT.print(c);
}

#define ESP_MAIL_PRINTF mb_print_printf

#endif

#endif