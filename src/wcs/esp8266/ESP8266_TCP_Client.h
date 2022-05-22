/**
 *
 * The Network Upgradable ESP8266 Secure TCP Client Class, ESP8266_TCP_Client.h v1.0.6
 *
 * Created May 22, 2022
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

#ifndef ESP8266_TCP_Client_H
#define ESP8266_TCP_Client_H

#ifdef ESP8266

#include <Arduino.h>
#include <core_version.h>
#include <time.h>
#include <string>
#include "extras/SDK_Version_Common.h"

#ifndef ARDUINO_ESP8266_GIT_VER
#error Your ESP8266 Arduino Core SDK is outdated, please update. From Arduino IDE go to Boards Manager and search 'esp8266' then select the latest version.
#endif

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "ESP8266_WCS.h"
#include "./wcs/base/TCP_Client_Base.h"

#define ESP8266_TCP_CLIENT

class ESP8266_TCP_Client : public TCP_Client_Base
{

public:
  ESP8266_TCP_Client();
  ~ESP8266_TCP_Client();

  void setCACert(const char *caCert);

  void setCertFile(const char *certFile, mb_fs_mem_storage_type storageType);

  void setTimeout(uint32_t timeoutSec);

  bool ethLinkUp();

  void ethDNSWorkAround();

  bool networkReady();

  void networkReconnect();

  void networkDisconnect();

  String fwVersion();

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

  uint8_t sdPin = 15;
  uint16_t bsslRxSize = 1024;
  uint16_t bsslTxSize = 1024;
  bool fragmentable = false;
  int _chunkSize = 1024;
  int maxRXBufSize = 16384; // SSL full supported 16 kB
  int maxTXBufSize = 16384;
  bool mflnChecked = false;
  int rxBufDivider = maxRXBufSize / _chunkSize;
  int txBufDivider = maxRXBufSize / _chunkSize;

private:
  std::unique_ptr<ESP8266_WCS> wcs = std::unique_ptr<ESP8266_WCS>(new ESP8266_WCS());

#ifndef USING_AXTLS
  X509List *x509 = nullptr;
#endif
};

#endif /* ESP8266 */

#endif /* ESP8266_TCP_Client_H */