/*
 * WiFiNINA TCP Client for ESP Mail Client, version 1.0.8
 *
 *
 * June 19, 2022
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
   * Set TCP connection time out in seconds.
   * @param timeoutSec The time out in seconds.
   */
  void setTimeout(uint32_t timeoutSec);

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
   * Get the device timestamp.
   * @return The timestamp value.
   */
  time_t getTime();

  /**
   * Get firmware version string.
   * @return The firmware version string.
   */
  String fwVersion();

  /**
   * Get firmware build number.
   * @return The firmware build number.
   */
  int firmwareBuildNumber();

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
   * Store the host name and port.
   * @param host The host name to connect.
   * @param port The port to connect.
   * @return true.
   */
  bool begin(const char *host, uint16_t port);

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

  /**
   * Wait to all receive buffer read.
   */
  void flush();

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
