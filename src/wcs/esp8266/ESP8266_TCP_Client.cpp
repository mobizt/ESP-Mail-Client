/**
 *
 * The Network Upgradable ESP8266 Secure TCP Client Class, ESP8266_TCP_Client.cpp v2.0.6
 *
 * Created March 2, 2023
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

#ifndef ESP8266_TCP_Client_CPP
#define ESP8266_TCP_Client_CPP

#include <Arduino.h>
#include "ESP_Mail_FS.h"
#if defined(ESP8266) || defined(MB_ARDUINO_PICO)

#include "ESP8266_TCP_Client.h"

ESP8266_TCP_Client::ESP8266_TCP_Client()
{
}

ESP8266_TCP_Client::~ESP8266_TCP_Client()
{
  _host.clear();
  if (wcs)
  {
    wcs->stop();
    wcs.reset(nullptr);
    wcs.release();
  }

#if defined(WCS_USE_BEARSSL)
  if (x509)
    delete x509;
#endif
}

void ESP8266_TCP_Client::setClient(Client *client)
{
  wcs->setClient(client);
}

void ESP8266_TCP_Client::setCACert(const char *caCert)
{

#if defined(WCS_USE_BEARSSL)
  wcs->setBufferSizes(bsslRxSize, bsslTxSize);
#endif

  if (caCert)
  {
#if defined(WCS_USE_BEARSSL)
    if (x509)
      delete x509;
#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
    x509 = new X509List(caCert);
#else
    x509 = new X509List(caCert);
#endif
    wcs->setTrustAnchors(x509);
#else
    wcs->setCACert_P(caCert, strlen_P(caCert));
#endif
    wcs->baseSetCertType(esp_mail_cert_type_data);
    wcs->setTA(true);
  }
  else
  {
#ifndef USING_AXTLS
    wcs->setInsecure();
#endif
    wcs->baseSetCertType(esp_mail_cert_type_none);
    wcs->setTA(false);
  }
}

bool ESP8266_TCP_Client::setCertFile(const char *certFile, mb_fs_mem_storage_type storageType)
{

#if defined(WCS_USE_BEARSSL)
  wcs->setBufferSizes(bsslRxSize, bsslTxSize);
#endif

  if (!wcs->mbfs)
    return false;

  if (wcs->clockReady && strlen(certFile) > 0)
  {
    MB_String filename = certFile;
    if (filename.length() > 0)
    {
      if (filename[0] != '/')
        filename.prepend('/');
    }

    int len = wcs->mbfs->open(filename, storageType, mb_fs_open_mode_read);
    if (len > -1)
    {
      uint8_t *der = (uint8_t *)wcs->mbfs->newP(len);
      if (wcs->mbfs->available(storageType))
        wcs->mbfs->read(storageType, der, len);
      wcs->mbfs->close(storageType);

#if defined(WCS_USE_BEARSSL)
      if (x509)
        delete x509;
#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
      x509 = new X509List(der, len);
#else
      x509 = new X509List(der, len);
#endif
      wcs->setTrustAnchors(x509);
#endif

      wcs->setTA(true);
      wcs->mbfs->delP(&der);

      wcs->baseSetCertType(esp_mail_cert_type_file);
    }
  }

  return getCertType() == esp_mail_cert_type_file;
}

void ESP8266_TCP_Client::setTimeout(uint32_t timeoutSec)
{

  wcs->setTimeout(timeoutSec * 1000);

  wcs->baseSetTimeout(timeoutSec);
}

bool ESP8266_TCP_Client::ethLinkUp()
{
  if (!wcs->session_config)
    return false;

  bool ret = false;
#if defined(ESP8266) && defined(ESP8266_CORE_SDK_V3_X_X)

#if defined(INC_ENC28J60_LWIP)
  if (wcs->session_config->spi_ethernet_module.enc28j60)
  {
    ret = wcs->session_config->spi_ethernet_module.enc28j60->status() == WL_CONNECTED;
    goto ex;
  }
#endif
#if defined(INC_W5100_LWIP)
  if (wcs->session_config->spi_ethernet_module.w5100)
  {
    ret = wcs->session_config->spi_ethernet_module.w5100->status() == WL_CONNECTED;
    goto ex;
  }
#endif
#if defined(INC_W5500_LWIP)
  if (wcs->session_config->spi_ethernet_module.w5500)
  {
    ret = wcs->session_config->spi_ethernet_module.w5500->status() == WL_CONNECTED;
    goto ex;
  }
#endif
#elif defined(MB_ARDUINO_PICO)

#endif

  return ret;

#if defined(INC_ENC28J60_LWIP) || defined(INC_W5100_LWIP) || defined(INC_W5500_LWIP)
ex:
#endif

  // workaround for ESP8266 Ethernet
  delayMicroseconds(0);

  return ret;
}

void ESP8266_TCP_Client::ethDNSWorkAround()
{

  if (!wcs->session_config)
    return;

#if defined(ESP8266_CORE_SDK_V3_X_X)

#if defined(INC_ENC28J60_LWIP)
  if (wcs->session_config->spi_ethernet_module.enc28j60)
    goto ex;
#endif
#if defined(INC_W5100_LWIP)
  if wcs
    ->(session_config->spi_ethernet_module.w5100) goto ex;
#endif
#if defined(INC_W5500_LWIP)
  if (wcs->session_config->spi_ethernet_module.w5500)
    goto ex;
#endif

#elif defined(MB_ARDUINO_PICO)

#endif

  return;

#if defined(INC_ENC28J60_LWIP) || defined(INC_W5100_LWIP) || defined(INC_W5500_LWIP)
ex:
  WiFiClient client;
  client.connect(wcs->session_config->server.host_name.c_str(), wcs->session_config->server.port);
  client.stop();
#endif
}

bool ESP8266_TCP_Client::networkReady()
{
#if defined(ENABLE_CUSTOM_CLIENT)

  if (network_status_cb)
    network_status_cb();

  return networkStatus;
#else
  return WiFi.status() == WL_CONNECTED || ethLinkUp();
#endif
}

void ESP8266_TCP_Client::networkReconnect()
{
#if defined(ENABLE_CUSTOM_CLIENT)
  if (network_connection_cb)
    network_connection_cb();
#elif defined(ESP8266)
  WiFi.reconnect();
#endif
}

void ESP8266_TCP_Client::networkDisconnect()
{
#if defined(ENABLE_CUSTOM_CLIENT)
  if (network_disconnection_cb)
    network_disconnection_cb();
#else
  WiFi.disconnect();
#endif
}

String ESP8266_TCP_Client::fwVersion()
{
  return String();
}

esp_mail_client_type ESP8266_TCP_Client::type()
{
#if defined(ENABLE_CUSTOM_CLIENT)
  return esp_mail_client_type_custom;
#else
  return esp_mail_client_type_internal;
#endif
}

bool ESP8266_TCP_Client::isInitialized()
{
#if defined(ENABLE_CUSTOM_CLIENT)

  bool rdy = wcs != nullptr;

  bool upgradeRequired = false;

#if !defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
  if (wcs->getProtocol(_port) == (int)esp_mail_protocol_tls && !connection_upgrade_cb)
    upgradeRequired = true;
#endif

  if (!network_connection_cb || !network_status_cb || upgradeRequired)
  {
    rdy = false;
    if (wcs->debugLevel > 0)
    {
      if (!network_connection_cb)
        esp_mail_debug_print(esp_mail_str_369, true);

      if (!network_status_cb)
        esp_mail_debug_print(esp_mail_str_370, true);

      if (upgradeRequired)
        esp_mail_debug_print(esp_mail_str_368, true);
    }
  }

  return rdy;
#else
  return true;
#endif
}

int ESP8266_TCP_Client::hostByName(const char *name, IPAddress &ip)
{
  return WiFi.hostByName(name, ip);
}

bool ESP8266_TCP_Client::begin(const char *host, uint16_t port)
{
  if (strcmp(_host.c_str(), host) != 0)
    mflnChecked = false;

  _host = host;
  _port = port;

#if defined(WCS_USE_BEARSSL)
  // probe for fragmentation support at the specified size
  if (!mflnChecked)
  {
    // remove WiFClientSecure
    // fragmentable = WiFiClientSecure::probeMaxFragmentLength(this->host.c_str(), this->port, _chunkSize);
    if (fragmentable)
    {
      bsslRxSize = _chunkSize;
      bsslTxSize = _chunkSize;
      wcs->setBufferSizes(bsslRxSize, bsslTxSize);
    }
    mflnChecked = true;
  }

  if (!fragmentable)
    wcs->setBufferSizes(maxRXBufSize / rxBufDivider, maxTXBufSize / txBufDivider);

#endif

  return true;
}

bool ESP8266_TCP_Client::connect(bool secured, bool verify)
{
  wcs->setSecure(secured);
  wcs->setVerify(verify);

  if (connected())
  {
    flush();
    return true;
  }

#if defined(ENABLE_CUSTOM_CLIENT)

  // no client assigned?
  if (!wcs->_basic_client)
  {
    if (wcs->debugLevel > 0)
    {
      MB_String s = esp_mail_str_185;
      s += esp_mail_str_346;
      esp_mail_debug_print(s.c_str(), true);
    }
    return false;
  }

  // no client type assigned?
  if (wcs->ext_client_type == esp_mail_external_client_type_none)
  {
    if (wcs->debugLevel > 0)
      esp_mail_debug_print(esp_mail_str_372, true);
    return false;
  }

  // pain text via ssl client?
  if (!secured && wcs->ext_client_type == esp_mail_external_client_type_ssl)
  {
    if (wcs->debugLevel > 0)
      esp_mail_debug_print(esp_mail_str_366, true);
    return false;
  }

#endif

// internal client or external client with internal ssl engine
#if !defined(ENABLE_CUSTOM_CLIENT) || defined(ESP_MAIL_USE_SDK_SSL_ENGINE)

  // internal or external client with innternal ssl client
  if (!wcs->connect(_host.c_str(), _port))
    return false;

#endif

  bool res = connected();

  if (!res)
    stop();

  return res;
}

bool ESP8266_TCP_Client::connectSSL(bool verify)
{

  bool res = false;

#if defined(ENABLE_CUSTOM_CLIENT) && !defined(ESP_MAIL_USE_SDK_SSL_ENGINE)

  wcs->tls_required = true;

  if (connection_upgrade_cb)
    connection_upgrade_cb();

  res = this->connected();

  if (!res)
    stop();

  return res;

#endif

  res = wcs->connectSSL(verify);

  if (!res)
    wcs->stop();

  return res;
}

void ESP8266_TCP_Client::stop()
{
  _host.clear();
  return wcs->stop();
}

bool ESP8266_TCP_Client::connected()
{
  if (wcs)
    return wcs->_connected();
  return false;
}

int ESP8266_TCP_Client::write(uint8_t *data, int len)
{
  if (!data)
    return TCP_CLIENT_ERROR_SEND_DATA_FAILED;

  if (len == 0)
    return TCP_CLIENT_ERROR_SEND_DATA_FAILED;

  if (!networkReady())
    return TCP_CLIENT_ERROR_NOT_CONNECTED;

  if (!connect(wcs->isSecure(), wcs->isVerify()))
    return TCP_CLIENT_ERROR_CONNECTION_REFUSED;

  int toSend = _chunkSize;
  int sent = 0;
  while (sent < len)
  {
    if (sent + toSend > len)
      toSend = len - sent;

    if ((int)wcs->write(data + sent, toSend) != toSend)
      return TCP_CLIENT_ERROR_SEND_DATA_FAILED;

    sent += toSend;
  }

  return len;
}

int ESP8266_TCP_Client::send(const char *data)
{
  return write((uint8_t *)data, strlen(data));
}

int ESP8266_TCP_Client::print(int data)
{
  char *buf = (char *)wcs->mbfs->newP(64);
  sprintf(buf, (const char *)FPSTR("%d"), data);
  int ret = send(buf);
  wcs->mbfs->delP(&buf);
  return ret;
}

int ESP8266_TCP_Client::print(const char *data)
{
  return send(data);
}

int ESP8266_TCP_Client::println(const char *data)
{
  int len = send(data);
  if (len < 0)
    return len;
  int sz = send((const char *)FPSTR("\r\n"));
  if (sz < 0)
    return sz;
  return len + sz;
}

int ESP8266_TCP_Client::println(int data)
{
  char *buf = (char *)wcs->mbfs->newP(64);
  sprintf(buf, (const char *)FPSTR("%d\r\n"), data);
  int ret = send(buf);
  wcs->mbfs->delP(&buf);
  return ret;
}

int ESP8266_TCP_Client::available()
{
  return wcs->available();
}

int ESP8266_TCP_Client::read()
{
  return wcs->read();
}

int ESP8266_TCP_Client::readBytes(uint8_t *buf, int len)
{
  return wcs->read(buf, len);
}

int ESP8266_TCP_Client::readBytes(char *buf, int len)
{
  return readBytes((uint8_t *)buf, len);
}

void ESP8266_TCP_Client::flush()
{
  while (available() > 0)
    read();
}

#endif /* ESP8266 */

#endif /* ESP8266_TCP_Client_CPP */