/*
 * ESP32 TCP Client Library v1.0.3.
 * 
 * November 16, 2021 
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

#include <Arduino.h>
#include <WiFiClient.h>
#include <FS.h>
#include <SPIFFS.h>
#if defined(ESP_MAIL_DEFAULT_SD_FS)
#if ESP_MAIL_DEFAULT_SD_FS == SD
#include <SD.h>
#endif
#endif
#include "ESP32_WCS.h"



#define TCP_CLIENT_ERROR_CONNECTION_REFUSED (-1)
#define TCP_CLIENT_ERROR_SEND_DATA_FAILED (-2)
#define TCP_CLIENT_DEFAULT_TCP_TIMEOUT_SEC 30

enum esp_mail_file_storage_type
{
  esp_mail_file_storage_type_none,
  esp_mail_file_storage_type_flash,
  esp_mail_file_storage_type_sd
};

class ESP32_TCP_Client
{
public:
  ESP32_TCP_Client();
  ~ESP32_TCP_Client();

  /**
    * Initialization of new TCP connection.
    * \param host - Host name without protocols.
    * \param port - Server's port.
    * \return True by default.
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
  ESP32_WCS *stream(void);

  /**
   * Set insecure mode
  */
  void setInsecure();


  int tcpTimeout = 40000;
  bool connect(void);
  bool connect(bool secured, bool verify);
  void setCACert(const char *caCert);
  void setCertFile(const char *caCertFile, esp_mail_file_storage_type storageType);
  void setDebugCallback(DebugMsgCallback cb);

  int _certType = -1;
  MBSTRING _caCertFile;
  esp_mail_file_storage_type _caCertFileStoreageType = esp_mail_file_storage_type::esp_mail_file_storage_type_none;

protected:
  DebugMsgCallback _debugCallback = NULL;
  std::unique_ptr<ESP32_WCS> _wcs = std::unique_ptr<ESP32_WCS>(new ESP32_WCS());

  MBSTRING _host;
  uint16_t _port = 0;
};

#endif //ESP32

#endif //ESP32_TCP_Client_H
