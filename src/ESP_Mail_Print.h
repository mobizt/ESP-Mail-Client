#ifndef ESP_MAIL_PRINT_H_
#define ESP_MAIL_PRINT_H_

#include "ESP_Mail_Client_Version.h"
#if !VALID_VERSION_CHECK(30400)
#error "Mixed versions compilation."
#endif

#if defined(MB_ARDUINO_ESP) || defined(MB_ARDUINO_PICO)

#define ESP_MAIL_PRINTF ESP_MAIL_DEFAULT_DEBUG_PORT.printf

#else


#define ESP_MAIL_PRINTF MailClient.printf

#endif

#endif