/**
 *
 * The Network Upgradable ESP8266 Secure TCP Client Class, ESP8266_TCP_Client.cpp v1.0.5
 *
 * Created April 17, 2022
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

#ifndef ESP8266_TCP_Client_CPP
#define ESP8266_TCP_Client_CPP


#ifdef ESP8266

#include "ESP8266_TCP_Client.h"

ESP8266_TCP_Client::ESP8266_TCP_Client()
{
}

ESP8266_TCP_Client::~ESP8266_TCP_Client()
{
  if (wcs)
  {
    wcs->stop();
    wcs.reset(nullptr);
    wcs.release();
  }

#ifndef USING_AXTLS
  if (x509)
    delete x509;
#endif
}

void ESP8266_TCP_Client::setCACert(const char *caCert)
{

#ifndef USING_AXTLS
  wcs->setBufferSizes(bsslRxSize, bsslTxSize);
#endif

  if (caCert)
  {
#ifndef USING_AXTLS
    if (x509)
      delete x509;
    x509 = new X509List(caCert);
    wcs->setTrustAnchors(x509);
    wcs->setTA(true);
#else
    wcs->setCACert_P(caCert, strlen_P(caCert));
#endif
    baseSetCertType(esp_mail_cert_type_data);
  }
  else
  {
#ifndef USING_AXTLS
    wcs->setInsecure();
#endif
    baseSetCertType(esp_mail_cert_type_none);
    wcs->setTA(false);
  }
}

void ESP8266_TCP_Client::setCertFile(const char *certFile, mb_fs_mem_storage_type storageType)
{

#ifndef USING_AXTLS
  wcs->setBufferSizes(bsslRxSize, bsslTxSize);
#endif

  if (!mbfs)
    return;

  if (clockReady && strlen(certFile) > 0)
  {
    MB_String filename = certFile;
    if (filename.length() > 0)
    {
      if (filename[0] != '/')
        filename.prepend('/');
    }

    int len = mbfs->open(filename, storageType, mb_fs_open_mode_read);
    if (len > -1)
    {
      uint8_t *der = (uint8_t *)mbfs->newP(len);
      if (mbfs->available(storageType))
        mbfs->read(storageType, der, len);
      mbfs->close(storageType);

      if (x509)
        delete x509;
      x509 = new X509List(der, len);
      wcs->setTrustAnchors(x509);
      wcs->setTA(true);
      mbfs->delP(&der);

      baseSetCertType(esp_mail_cert_type_file);
    }
  }
  wcs->setNoDelay(true);
}

void ESP8266_TCP_Client::setTimeout(uint32_t timeoutSec)
{
  if (wcs)
    wcs->setTimeout(timeoutSec * 1000);

  baseSetTimeout(timeoutSec);
}

bool ESP8266_TCP_Client::ethLinkUp()
{
  if (!session)
    return false;

  bool ret = false;
#if defined(ESP8266) && defined(ESP8266_CORE_SDK_V3_X_X)

#if defined(INC_ENC28J60_LWIP)
  if (session->spi_ethernet_module.enc28j60)
  {
    ret = session->spi_ethernet_module.enc28j60->status() == WL_CONNECTED;
    goto ex;
  }
#endif
#if defined(INC_W5100_LWIP)
  if (session->spi_ethernet_module.w5100)
  {
    ret = session->spi_ethernet_module.w5100->status() == WL_CONNECTED;
    goto ex;
  }
#endif
#if defined(INC_W5100_LWIP)
  if (session->spi_ethernet_module.w5500)
  {
    ret = session->spi_ethernet_module.w5500->status() == WL_CONNECTED;
    goto ex;
  }
#endif

  return ret;

ex:
  // workaround for ESP8266 Ethernet
  delayMicroseconds(0);
#endif

  return ret;
}

void ESP8266_TCP_Client::ethDNSWorkAround()
{

  if (!session)
    return;

#if defined(ESP8266_CORE_SDK_V3_X_X)

#if defined(INC_ENC28J60_LWIP)
  if (session->spi_ethernet_module.enc28j60)
    goto ex;
#endif
#if defined(INC_W5100_LWIP)
  if (session->spi_ethernet_module.w5100)
    goto ex;
#endif
#if defined(INC_W5100_LWIP)
  if (session->spi_ethernet_module.w5500)
    goto ex;
#endif

  return;

ex:
  WiFiClient client;
  client.connect(session->server.host_name.c_str(), session->server.port);
  client.stop();
#endif
}

bool ESP8266_TCP_Client::networkReady()
{
  return WiFi.status() == WL_CONNECTED || ethLinkUp();
}

void ESP8266_TCP_Client::networkReconnect()
{
  WiFi.reconnect();
}

void ESP8266_TCP_Client::networkDisconnect()
{
  WiFi.disconnect();
}

String ESP8266_TCP_Client::fwVersion()
{
  return String();
}

esp_mail_client_type ESP8266_TCP_Client::type()
{
  return esp_mail_client_type_internal;
}

bool ESP8266_TCP_Client::isInitialized() { return true; }

int ESP8266_TCP_Client::hostByName(const char *name, IPAddress &ip)
{
  return WiFi.hostByName(name, ip);
}

bool ESP8266_TCP_Client::begin(const char *host, uint16_t port)
{
  if (strcmp(this->host.c_str(), host) != 0)
    mflnChecked = false;

  this->host = host;
  this->port = port;

  // probe for fragmentation support at the specified size
  if (!mflnChecked)
  {
    fragmentable = WiFiClientSecure::probeMaxFragmentLength(this->host.c_str(), this->port, _chunkSize);
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

  return true;
}

bool ESP8266_TCP_Client::connect(bool secured, bool verify)
{
  wcs->setSecure(secured);
  wcs->setVerify(verify);

  if (connected())
  {
    while (wcs->available() > 0)
      wcs->read();
    return true;
  }

  if (!wcs->connect(host.c_str(), port))
    return false;

  return connected();
}

bool ESP8266_TCP_Client::connectSSL(bool verify)
{
  if (!wcs)
    return false;
  return wcs->connectSSL(verify);
}

void ESP8266_TCP_Client::stop()
{
  if (connected())
    return wcs->stop();
}

bool ESP8266_TCP_Client::connected()
{
  if (wcs)
    return wcs->connected();
  return false;
}

int ESP8266_TCP_Client::write(uint8_t *data, int len)
{
  if (!data)
    return TCP_CLIENT_ERROR_SEND_DATA_FAILED;

  if (len == 0)
    return TCP_CLIENT_ERROR_SEND_DATA_FAILED;

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
  char *buf = (char *)mbfs->newP(64);
  sprintf(buf, (const char *)FPSTR("%d"), data);
  int ret = send(buf);
  mbfs->delP(&buf);
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
  char *buf = (char *)mbfs->newP(64);
  sprintf(buf, (const char *)FPSTR("%d\r\n"), data);
  int ret = send(buf);
  mbfs->delP(&buf);
  return ret;
}

int ESP8266_TCP_Client::available()
{
  if (!wcs)
    return TCP_CLIENT_ERROR_NOT_INITIALIZED;

  return wcs->available();
}

int ESP8266_TCP_Client::read()
{
  if (!wcs)
    return TCP_CLIENT_ERROR_NOT_INITIALIZED;

  return wcs->read();
}

int ESP8266_TCP_Client::readBytes(uint8_t *buf, int len)
{
  if (!wcs)
    return TCP_CLIENT_ERROR_NOT_INITIALIZED;

  return wcs->readBytes(buf, len);
}

int ESP8266_TCP_Client::readBytes(char *buf, int len)
{
  return readBytes((uint8_t *)buf, len);
}

#endif /* ESP8266 */

#endif /* ESP8266_TCP_Client_CPP */