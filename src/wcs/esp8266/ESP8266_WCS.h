/**
 *
 * The Network Upgradable ESP8266 Secure WiFi Client Class, ESP8266_WCS.h v1.0.4
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

#ifdef ESP8266_CORE_SDK_V3_X_X
#define OVERRIDING override;
#else
#define OVERRIDING
#endif

class ESP8266_WCS : public WCS_CLASS
{
public:
  ESP8266_WCS();
  ~ESP8266_WCS() OVERRIDING;

  /**
   * Connect to server.
   * @param name The server host name to connect.
   * @param port The server port to connecte.
   * @return 1 for success or 0 for error.
   */
  int connect(const char *name, uint16_t port) OVERRIDING;

  /**
   * Get TCP connection status.
   * @return 1 for connected or 0 for not connected.
   */
  uint8_t connected() OVERRIDING;

  /**
   * Get available data size to read.
   * @return The avaiable data size.
   */
  int available() OVERRIDING;

  /**
   * The TCP data read function.
   * @return A byte data that was successfully read or -1 for error.
   */
  int read() OVERRIDING;

  /**
   * The TCP data read function.
   * @param buf The data buffer.
   * @param size The length of data that read.
   * @return The size of data that was successfully read or -1 for error.
   */
  int read(uint8_t *buf, size_t size) OVERRIDING;

  /**
   * The TCP data write function.
   * @param buf The data to write.
   * @param size The length of data to write.
   * @return The size of data that was successfully written or 0 for error.
   */
  size_t write(const uint8_t *buf, size_t size) OVERRIDING;

  /**
   * The TCP data write function.
   * @param buf The data to write.
   * @param size The length of data to write.
   * @return The size of data that was successfully written or 0 for error.
   */
  size_t write_P(PGM_P buf, size_t size) OVERRIDING;

  /**
   * Read one byte from stream.
   * @return The data that was successfully read or -1 for error.
   */
  int peek() override;

  /**
   * Read data from Stream with time out.
   * @param buffer The data buffer.
   * @param length The length of data read.
   * @return The size of data that was successfully read.
   */
  size_t peekBytes(uint8_t *buffer, size_t length) OVERRIDING;


  /**
   * Set the status which used when certificates were installed and ready to verify.
   * @param hasTA The status to set.
   */
  void setTA(bool hasTA);

  /**
   * Set the secure TCP connection mode.
   * @param secure The secure option.
   */
  void setSecure(bool secure);

  /**
   * Set the Root CA certificate verification.
   * @param verify The Root CA certificate verification option.
   */
  void setVerify(bool verify);

  /**
   * Get the secure mode connection status.
   * @return The secure mode connection status.
   */
  bool isSecure();

  /**
   * Get the Root CA certificate verification mode status.
   * @return The Root CA certificate verification mode status.
   */
  bool isVerify();

  /**
   * Upgrade the current connection by setting up the SSL and perform the SSL handshake.
   *
   * @param verify The Root CA certificate verification option
   * @return operating result.
   */
  bool connectSSL(bool verify);

private:
  /**
   * The non-secure mode TCP data write function.
   * @param b The data to write.
   * @return 1 for success or 0 for error.
   */
  size_t ns_write(uint8_t b);

  /**
   * The non-secure mode TCP data write function.
   * @param buf The data to write.
   * @param size The length of data to write.
   * @return The size of data that was successfully written or 0 for error.
   */
  size_t ns_write(const uint8_t *buf, size_t size);

  /**
   * The non-secure mode TCP data write function.
   * @param stream The Stream to write until timed out.
   * @param unused Unused param.
   * @return The size of data that was successfully written until timed out.
   */
  size_t ns_write(Stream &stream, size_t unused);

  /**
   * The non-secure mode TCP data write function.
   * @param stream The Stream to write.
   * @return The size of data that was successfully written.
   */
  size_t ns_write(Stream &stream);

  /**
   * The non-secure mode TCP data write function.
   * @param buf The data to write.
   * @param size The length of data to write.
   * @return The size of data that was successfully written or 0 for error.
   */
  size_t ns_write_P(PGM_P buf, size_t size);

  /**
   * Get the non-secure mode available data size to read.
   * @return The avaiable data size.
   */
  int ns_available();

  /**
   * The non-secure mode TCP data read function.
   * @return The read value or -1 for error.
   */
  int ns_read();

  /**
   * The non-secure mode TCP data read function.
   * @param buf The data buffer.
   * @param size The length of data that read.
   * @return The size of data that was successfully read or 0 for error.
   */
  int ns_read(uint8_t *buf, size_t size);

  /**
   * Non-secure mode read one byte from stream.
   * @return The data that was successfully read or -1 for error.
   */
  int ns_peek();

  /**
   * Non-secure mode read data from Stream with time out.
   * @param buffer The data buffer.
   * @param length The length of data read.
   * @return The size of data that was successfully read.
   */
  size_t ns_peekBytes(uint8_t *buffer, size_t length);

  /**
   * Get non-secure TCP connection status.
   * @return 1 for connected or 0 for not connected.
   */
  uint8_t ns_connected();

  bool _secured = false;
  MB_String _host_name;
  bool _has_ta = false;
  bool _base_use_insecure = false;
};

#endif /* ESP8266 */

#endif /* ESP8266_WCS_H */
