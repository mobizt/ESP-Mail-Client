/*
 * ESP32 TCP Client Library v1.0.7.
 *
 * May 22, 2022
 *
 * The MIT License (MIT)
 * Copyright (c) 2021 K. Suwatchai (Mobizt)
 *
 * TCPClient Arduino library for ESP32
 *
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the TCPClient for Arduino.
 * Port to ESP32 by Evandro Luis Copercini (2017),
 * changed fingerprints to CA verification.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef ESP32_TCP_Client_H
#define ESP32_TCP_Client_H

#ifdef ESP32

#define ESP32_TCP_CLIENT

#include <Arduino.h>
#include <WiFiClient.h>
#include <ETH.h>
#include "ESP32_WCS.h"
#include "./wcs/base/TCP_Client_Base.h"

extern "C"
{
#include <esp_err.h>
#include <esp_wifi.h>
}

class ESP32_TCP_Client : public TCP_Client_Base
{
public:
  ESP32_TCP_Client();
  ~ESP32_TCP_Client();

  void setCACert(const char *caCert);

  void setCertFile(const char *certFile, mb_fs_mem_storage_type storageType);

  void setDebugCallback(DebugMsgCallback cb);

  void setTimeout(uint32_t timeoutSec);

  void setInsecure();

  bool ethLinkUp();

  void ethDNSWorkAround();

  bool networkReady();

  void networkReconnect();

  void networkDisconnect();

  String fwVersion();

  esp_mail_client_type type();

  bool isInitialized();

  int hostByName(const char *name, IPAddress &ip);

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
  DebugMsgCallback debugCallback = NULL;
  std::unique_ptr<ESP32_WCS> wcs = std::unique_ptr<ESP32_WCS>(new ESP32_WCS());
  char *cert_buf = NULL;
};

#endif // ESP32

#endif // ESP32_TCP_Client_H
