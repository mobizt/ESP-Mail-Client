/*
 * WiFiNINA TCP Client for ESP Mail Client, version 1.0.4
 *
 * 
 * February 1, 2022
 * 
 * Add support Arduino Nano RP2040 Connect
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

#if (defined(ARDUINO_ARCH_SAMD) && !defined(ARDUINO_SAMD_MKR1000)) || defined(ARDUINO_NANO_RP2040_CONNECT)

#ifndef WiFiNINA_TCP_Client_H
#define WiFiNINA_TCP_Client_H


#include <Arduino.h>
#include "lib/WiFiNINA.h"

#if !defined(__AVR__)
#include <string>
#endif

#include "./ESP_Mail_Error.h"
#include "./wcs/base/TCP_Client_Base.h"

class WiFiNINA_TCP_Client : public TCP_Client_Base
{
  friend class ESP_Mail_Client;

public:
  WiFiNINA_TCP_Client();
  ~WiFiNINA_TCP_Client();

  void setCACert(const char *caCert);

  void setCertFile(const char *certFile, mb_fs_mem_storage_type storageType);

  void setTimeout(uint32_t timeoutSec);

  void ethDNSWorkAround();

  bool networkReady();

  void networkReconnect();

  void networkDisconnect();

  time_t getTime();

  String fwVersion();

  int firmwareBuildNumber();

  esp_mail_client_type type();

  bool isInitialized();

  int hostByName(const char *name, IPAddress &ip);

  bool begin(const char *host, uint16_t port);

  bool connect(bool secured, bool verify);

  bool connectSSL(bool verify);

  void stop();

  bool connected();

  int write(uint8_t *data, int len);

  int send(const char *data);

  int print(const char *data);

  int print(int data);

  int println(const char *data);

  int println(int data);

  int available();

  int read();

  int readBytes(uint8_t *buf, int len);

  int readBytes(char *buf, int len);

private:
  WiFiSSLClient *wcs = nullptr;
  WiFiClient *wc = nullptr;
  bool secured = false;
  bool verifyRootCA = false;
  int sock = -1;
  int fwBuild = -1;
};

#endif

#endif /* Firebase_Arduino_WiFiNINA_TCP_Client_H */
