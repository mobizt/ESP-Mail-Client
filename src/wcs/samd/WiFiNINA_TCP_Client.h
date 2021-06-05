/*
 * WiFiNINA TCP Client for ESP Mail Client, version 1.0.0
 *
 * 
 * June 1, 2021
 * 
 * 
 * 
 * The MIT License (MIT)
 * Copyright (c) 2021 K. Suwatchai (Mobizt)
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

#if defined(ARDUINO_ARCH_SAMD)

#ifndef WiFiNINA_TCP_Client_H
#define WiFiNINA_TCP_Client_H

#include "ESP_Mail_FS.h"


#include <Arduino.h>
#include "lib/WiFiNINA.h"
#include <string>
#include <vector>

#if defined(ESP_Mail_DEFAULT_FLASH_FS)
#define ESP_MAIL_FLASH_FS ESP_Mail_DEFAULT_FLASH_FS
#endif

#if defined(ESP_Mail_DEFAULT_SD_FS)
#define ESP_MAIL_SD_FS ESP_Mail_DEFAULT_SD_FS
#endif

#define TCP_CLIENT_ERROR_CONNECTION_REFUSED (-1)
#define TCP_CLIENT_ERROR_SEND_DATA_FAILED (-2)
#define TCP_CLIENT_DEFAULT_TCP_TIMEOUT_SEC 30

enum esp_mail_file_storage_type
{
  esp_mail_file_storage_type_none,
  esp_mail_file_storage_type_flash,
  esp_mail_file_storage_type_sd
};

class WiFiNINA_TCP_Client
{
  friend class ESP_Mail_Client;

public:
  WiFiNINA_TCP_Client();
  ~WiFiNINA_TCP_Client();

  /**
    * Initialization of new TCP connection.
    * \param host - Host name without protocols.
    * \param port - Server's port.
    * \return True as default.
    * If no certificate string provided, use (const char*)NULL to CAcert param 
    */
  bool begin(const char *host, uint16_t port);

  /**
    * Check the TCP connection status.
    * \return True if connected.
    */
  bool connected();

  /**
    * Establish TCP connection when required and send data.
    * \param data - The data to send.
    * \return TCP status code, Return zero if new TCP connection and data sent.
    */
  int send(const char *data);

  /**
    * Get the WiFi client pointer.
    * \return WiFi client pointer.
    */
  WiFiSSLClient *stream(void);

  bool connect(bool secured, bool verify);
  bool connectSSL(bool verify);
  int firmwareBuildNumber();

private:
  

  WiFiSSLClient *_wcs = nullptr;
  WiFiClient *_wc = nullptr;
  bool _secured = false;
  bool _verifyRootCA = false;
  std::string _host = "";
  uint16_t _port = 0;
  int _sock = -1;
  int _fwBuild = -1;
  unsigned long tcpTimeout = 5000;
};

#endif

#endif /* Firebase_Arduino_WiFiNINA_TCP_Client_H */
