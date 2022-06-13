/*
 * ESP32 TCP Client Library v1.0.8
 *
 * June 13, 2022
 *
 * The MIT License (MIT)
 * Copyright (c) 2021 K. Suwatchai (Mobizt)
 *
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

  /**
   * Set Root CA certificate to verify.
   * @param caCert The certificate.
   */
  void setCACert(const char *caCert);

  /**
   * Set Root CA certificate to verify.
   * @param certFile The certificate file path.
   * @param storageType The storage type mb_fs_mem_storage_type_flash or mb_fs_mem_storage_type_sd.
   */
  void setCertFile(const char *certFile, mb_fs_mem_storage_type storageType);

  /**
   * Set the debug callback function.
   * @param cb The callback function that accepts const char* as parameter.
   */
  void setDebugCallback(DebugMsgCallback cb);

  /**
   * Set TCP connection time out in seconds.
   * @param timeoutSec The time out in seconds.
   */
  void setTimeout(uint32_t timeoutSec);

  /**
   * Disable certificate verification and authentication.
   */
  void setInsecure();

  /**
   * Get the ethernet link status.
   * @return true for link up or false for link down.
   */
  bool ethLinkUp();

  /**
   * Ethernet DNS workaround.
   */
  void ethDNSWorkAround();

  /**
   * Get the network status.
   * @return true for connected or false for not connected.
   */
  bool networkReady();

  /**
   * Reconnect the network.
   */
  void networkReconnect();

  /**
   * Disconnect the network.
   */
  void networkDisconnect();

  /**
   * Get firmware version string.
   * @return The firmware version string.
   */
  String fwVersion();

  /**
   * Get the Client type.
   * @return The esp_mail_client_type enum value.
   */
  esp_mail_client_type type();

  /**
   * Get the Client initialization status.
   * @return The initialization status.
   */
  bool isInitialized();

  /**
   * Set Root CA certificate to verify.
   * @param name The host name.
   * @param ip The ip address result.
   * @return 1 for success or 0 for failed.
   */
  int hostByName(const char *name, IPAddress &ip);

  /**
   * Start TCP connection using stored host name and port.
   * @param secure The secure mode option.
   * @param verify The Root CA certificate verification option.
   * @return true for success or false for error.
   */
  bool connect(bool secured, bool verify);

  /**
   * Upgrade the current connection by setting up the SSL and perform the SSL handshake.
   *
   * @param verify The Root CA certificate verification option
   * @return operating result.
   */
  bool connectSSL(bool verify);

  /**
   * Stop TCP connection.
   */
  void stop();

  /**
   * Get the TCP connection status.
   * @return true for connected or false for not connected.
   */
  bool connected();

  /**
   * The TCP data write function.
   * @param data The data to write.
   * @param len The length of data to write.
   * @return The size of data that was successfully written or 0 for error.
   */
  int write(uint8_t *data, int len);

  /**
   * The TCP data send function.
   * @param data The data to send.
   * @return The size of data that was successfully send or 0 for error.
   */
  int send(const char *data);

  /**
   * The TCP data print function.
   * @param data The data to print.
   * @return The size of data that was successfully print or 0 for error.
   */
  int print(const char *data);

  /**
   * The TCP data print function.
   * @param data The data to print.
   * @return The size of data that was successfully print or 0 for error.
   */
  int print(int data);

  /**
   * The TCP data print with new line function.
   * @param data The data to print.
   * @return The size of data that was successfully print or 0 for error.
   */
  int println(const char *data);

  /**
   * The TCP data print with new line function.
   * @param data The data to print.
   * @return The size of data that was successfully print or 0 for error.
   */
  int println(int data);

  /**
   * Get available data size to read.
   * @return The avaiable data size.
   */
  int available();

  /**
   * The TCP data read function.
   * @return The read value or -1 for error.
   */
  int read();

  /**
   * The TCP data read function.
   * @param buf The data buffer.
   * @param len The length of data that read.
   * @return The size of data that was successfully read or negative value for error.
   */
  int readBytes(uint8_t *buf, int len);

  /**
   * The TCP data read function.
   * @param buf The data buffer.
   * @param len The length of data that read.
   * @return The size of data that was successfully read or negative value for error.
   */
  int readBytes(char *buf, int len);

private:
  DebugMsgCallback debugCallback = NULL;
  std::unique_ptr<ESP32_WCS> wcs = std::unique_ptr<ESP32_WCS>(new ESP32_WCS());
  char *cert_buf = NULL;
};

#endif // ESP32

#endif // ESP32_TCP_Client_H
