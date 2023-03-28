/**
 *
 * The Network Upgradable ESP8266 Secure TCP Client Class, ESP8266_TCP_Client.h v2.0.11
 *
 * Created March 28, 2023
 *
 * The MIT License (MIT)
 * Copyright (c) 2023 K. Suwatchai (Mobizt)
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

#include <Arduino.h>
#include "ESP_Mail_FS.h"

#if (defined(ESP8266) || defined(MB_ARDUINO_PICO)) && (defined(ENABLE_SMTP) || defined(ENABLE_IMAP))

#include <time.h>
#include <string>
#include <WiFiClient.h>

#if defined(ESP8266)
#include <core_version.h>
#include "extras/SDK_Version_Common.h"

#ifndef ARDUINO_ESP8266_GIT_VER
#error Your ESP8266 Arduino Core SDK is outdated, please update. From Arduino IDE go to Boards Manager and search 'esp8266' then select the latest version.
#endif

#include <ESP8266WiFi.h>
#elif defined(MB_ARDUINO_PICO)
#include <WiFi.h>
#endif

using namespace BearSSL;

#include "ESP8266_WCS.h"

#define ESP8266_TCP_CLIENT

class ESP8266_TCP_Client
{

public:
  ESP8266_TCP_Client();
  ~ESP8266_TCP_Client();

  /**
   * Set the client.
   * @param client The Client interface.
   */
  void setClient(Client *client);

  /**
   * Set Root CA certificate to verify.
   * @param caCert The certificate.
   */
  void setCACert(const char *caCert);

  /**
   * Set Root CA certificate to verify.
   * @param certFile The certificate file path.
   * @param storageType The storage type mb_fs_mem_storage_type_flash or mb_fs_mem_storage_type_sd.
   * @return true when certificate loaded successfully.
   */
  bool setCertFile(const char *certFile, mb_fs_mem_storage_type storageType);

  /**
   * Set TCP connection time out in seconds.
   * @param timeoutSec The time out in seconds.
   */
  void setTimeout(uint32_t timeoutSec);

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
   * @return The size of data that was successfully sent or 0 for error.
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
   * Wait for all receive buffer data read.
   */
  void flush();

  void setMBFS(MB_FS *mbfs) { wcs->mbfs = mbfs; }

#if defined(ENABLE_SMTP) || defined(ENABLE_IMAP)
  void setSession(ESP_Mail_Session *session_config)
  {
    wcs->session_config = session_config;
  }
#endif

  void setClockReady(bool rdy)
  {
    wcs->clockReady = rdy;
  }
  esp_mail_cert_type getCertType()
  {
    return wcs->certType;
  }

  int getProtocol(uint16_t port)
  {
    return wcs->getProtocol(port);
  }

  void setDebugLevel(int debug)
  {
#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
    wcs->setDebugLevel(debug);
#endif
  }

  void reset_tlsErr()
  {
    wcs->tls_required = false;
    wcs->tls_error = false;
  }

  void setExtClientType(esp_mail_external_client_type type)
  {
    wcs->ext_client_type = type;
  }

  void set_tlsErrr(bool err)
  {
    wcs->tls_error = err;
  }

  bool tlsErr()
  {
    return wcs->tls_error;
  }

  void set_tlsRequired(bool req)
  {
    wcs->tls_required = req;
  }

  bool tlsRequired()
  {
    return wcs->tls_required;
  }

  int tcpTimeout()
  {
    return wcs->tmo;
  }

  void disconnect(){};

#if defined(ENABLE_CUSTOM_CLIENT)
  /**
   * Set the connection request callback.
   * @param connectCB The callback function that accepts the host name (const char*) and port (int) as parameters.
   */
  void connectionRequestCallback(ConnectionRequestCallback connectCB)
  {
    connection_cb = connectCB;
#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
    wcs->connectionRequestCallback(connectCB);
#endif
  }

  /**
   * Set the connection upgrade request callback.
   * @param upgradeCB The callback function to do the SSL setup and handshake.
   */
  void connectionUpgradeRequestCallback(ConnectionUpgradeRequestCallback upgradeCB)
  {
    this->connection_upgrade_cb = upgradeCB;
  }

  /**
   * Set the network connection request callback.
   * @param networkConnectionCB The callback function that handles the network connection.
   */
  void networkConnectionRequestCallback(NetworkConnectionRequestCallback networkConnectionCB)
  {
    network_connection_cb = networkConnectionCB;
  }

  /**
   * Set the network disconnection request callback.
   * @param networkConnectionCB The callback function that handles the network disconnection.
   */
  void networkDisconnectionRequestCallback(NetworkDisconnectionRequestCallback networkDisconnectionCB)
  {
    network_disconnection_cb = networkDisconnectionCB;
  }

  /**
   * Set the network status request callback.
   * @param networkStatusCB The callback function that calls the setNetworkStatus function to set the network status.
   */
  void networkStatusRequestCallback(NetworkStatusRequestCallback networkStatusCB)
  {
    network_status_cb = networkStatusCB;
  }

  /**
   * Set the network status which should call in side the networkStatusRequestCallback function.
   * @param status The status of network.
   */
  void setNetworkStatus(bool status)
  {
    networkStatus = status;
  }

#endif

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
  MB_String _host;
  uint16_t _port;
  std::unique_ptr<ESP8266_WCS> wcs = std::unique_ptr<ESP8266_WCS>(new ESP8266_WCS());

#if defined(WCS_USE_BEARSSL)
#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
  X509List *x509 = nullptr;
#else
  X509List *x509 = nullptr;
#endif
#endif

#if defined(ENABLE_CUSTOM_CLIENT)
  ConnectionRequestCallback connection_cb = NULL;
  ConnectionUpgradeRequestCallback connection_upgrade_cb = NULL;
  NetworkConnectionRequestCallback network_connection_cb = NULL;
  NetworkDisconnectionRequestCallback network_disconnection_cb = NULL;
  NetworkStatusRequestCallback network_status_cb = NULL;
  volatile bool networkStatus = false;
#endif
};

#endif /* ESP8266 */

#endif /* ESP8266_TCP_Client_H */