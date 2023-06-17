#ifndef ESP_MAIL_PRINT_H_
#define ESP_MAIL_PRINT_H_

#include "ESP_Mail_Client_Version.h"
#if !VALID_VERSION_CHECK(30111)
#error "Mixed versions compilation."
#endif

#if defined(MB_ARDUINO_ESP) || defined(MB_ARDUINO_PICO)

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