/**
 * 
 * The Network Upgradable ESP8266 Secure WiFi Client Class, ESP8266_WCS.h v1.0.3
 * 
 * The MIT License (MIT)
 * Copyright (c) 2022 K. Suwatchai (Mobizt)
 * 
 * 
 * Permission is hereby granted, free of charge, to any person returning a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef ESP8266_WCS_H
#define ESP8266_WCS_H

#ifdef ESP8266

#include <vector>
#include <WiFiClient.h>
#include <bearssl/bearssl.h>
#include "extras/SDK_Version_Common.h"
#include <WiFiClientSecure.h>

#include "ESP_Mail_FS.h"

#define _ESP_MAIL_USE_PSRAM_ ESP_MAIL_USE_PSRAM
#if defined(ESP_MAIL_USE_PSRAM)
#define MB_STRING_USE_PSRAM
#endif

#include "extras/MB_String.h"

#define MB_String MB_String

#ifdef DEBUG_ESP_SSL
#if defined(DEBUG_ESP_PORT)
#define DEBUG_BSSL(fmt, ...) DEBUG_ESP_PORT.printf_P((PGM_P)PSTR("BSSL:" fmt), ##__VA_ARGS__)
#else
#define DEBUG_BSSL(fmt, ...) ESP_MAIL_DEFAULT_DEBUG_PORT.printf((PGM_P)PSTR("BSSL:" fmt), ##__VA_ARGS__)
#endif
#else
#define DEBUG_BSSL(...)
#endif

#if !defined(USING_AXTLS) && defined(ESP8266_CORE_SDK_V3_X_X)
#define WCS_CLASS WiFiClientSecureCtx
#else
#define WCS_CLASS WiFiClientSecure
#endif
#define WC_CLASS WiFiClient

class ESP8266_WCS : public WCS_CLASS
{
public:
  ESP8266_WCS();
#ifdef ESP8266_CORE_SDK_V3_X_X
  ~ESP8266_WCS() override;
  int connect(const char *name, uint16_t port) override;
  uint8_t connected() override;
  int available() override;
  int read() override;
  int read(uint8_t *buf, size_t size) override;
  size_t write(const uint8_t *buf, size_t size) override;
  size_t write_P(PGM_P buf, size_t size) override;
  int peek() override;
  size_t peekBytes(uint8_t *buffer, size_t length) override;
#else
  ~ESP8266_WCS();
  int connect(const char *name, uint16_t port);
  uint8_t connected();
  int available();
  int read();
  int read(uint8_t *buf, size_t size);
  size_t write(const uint8_t *buf, size_t size);
  size_t write_P(PGM_P buf, size_t size);
  int peek();
  size_t peekBytes(uint8_t *buffer, size_t length);
#endif

  void setTA(bool hasTA);
  void setSecure(bool secure);
  void setVerify(bool verify);
  bool isSecure();
  bool isVerify();
  bool connectSSL(bool verify);

private:
  size_t ns_write(uint8_t b);
  size_t ns_write(const uint8_t *buf, size_t size);
  size_t ns_write(Stream &stream, size_t unused);
  size_t ns_write(Stream &stream);
  size_t ns_write_P(PGM_P buf, size_t size);
  int ns_available();
  int ns_read();
  int ns_read(uint8_t *buf, size_t size);
  int ns_peek();
  size_t ns_peekBytes(uint8_t *buffer, size_t length);
  uint8_t ns_connected();

  bool _secured = false;
  MB_String _host_name;
  bool _has_ta = false;
  bool _base_use_insecure = false;
};

#endif /* ESP8266 */

#endif /* ESP8266_WCS_H */
