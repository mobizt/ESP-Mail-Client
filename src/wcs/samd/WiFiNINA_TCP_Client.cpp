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

#ifndef WiFiNINA_TCP_Client_CPP
#define WiFiNINA_TCP_Client_CPP

#include "WiFiNINA_TCP_Client.h"

WiFiNINA_TCP_Client::WiFiNINA_TCP_Client()
{
}

WiFiNINA_TCP_Client::~WiFiNINA_TCP_Client()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
  if (wcs)
  {
    wcs->stop();
    delete wcs;
  }

  if (wc)
  {
    wc->stop();
    delete wc;
  }
#pragma GCC diagnostic pop
  host.clear();
}

void WiFiNINA_TCP_Client::setCACert(const char *caCert)
{
  if (caCert)
    baseSetCertType(esp_mail_cert_type_data);
  else
    baseSetCertType(esp_mail_cert_type_none);
  //wcs->setNoDelay(true);
}

void WiFiNINA_TCP_Client::setCertFile(const char *certFile, mb_fs_mem_storage_type storageType)
{
  baseSetCertType(esp_mail_cert_type_file);
}

void WiFiNINA_TCP_Client::setTimeout(uint32_t timeoutSec)
{
  baseSetTimeout(timeoutSec);
}

void WiFiNINA_TCP_Client::ethDNSWorkAround()
{
}

bool WiFiNINA_TCP_Client::networkReady()
{
  return WiFi.status() == WL_CONNECTED;
}

void WiFiNINA_TCP_Client::networkReconnect()
{
  
}

void WiFiNINA_TCP_Client::networkDisconnect()
{
  WiFi.disconnect();
}

time_t WiFiNINA_TCP_Client::getTime()
{
  now = WiFi.getTime();
  return now;
}

String WiFiNINA_TCP_Client::fwVersion()
{
  MB_String s = ", Fw v";
  s += WiFi.firmwareVersion();
  firmwareBuildNumber();
  if (fwBuild > 0)
  {
    s += "+";
    s += fwBuild;
  }
  return s.c_str();
}

esp_mail_client_type WiFiNINA_TCP_Client::type()
{
  return esp_mail_client_type_internal;
}

bool WiFiNINA_TCP_Client::isInitialized() { return true; }

int WiFiNINA_TCP_Client::hostByName(const char *name, IPAddress &ip)
{
  return WiFi.hostByName(name, ip);
}

int WiFiNINA_TCP_Client::firmwareBuildNumber()
{
  if (fwBuild < 0)
  {
    MB_String build = WiFi.getBuild();
    fwBuild = atoi(build.c_str());
    if (fwBuild < 21060)
      fwBuild = 0;
  }
  return fwBuild;
}

bool WiFiNINA_TCP_Client::begin(const char *host, uint16_t port)
{
  this->host = host;
  this->port = port;
  return true;
}

bool WiFiNINA_TCP_Client::connected()
{
  if (fwBuild > 0 || secured)
  {
    if (wcs)
      return wcs->connected();
  }
  else
  {
    if (wc)
      return wc->connected();
  }

  return false;
}

bool WiFiNINA_TCP_Client::connect(bool secured, bool verify)
{

  this->secured = secured;
  verifyRootCA = verify;

  firmwareBuildNumber();

  if (connected())
  {
    if (fwBuild > 0 || secured)
    {
      while (wcs->available() > 0)
        wcs->read();
    }
    else
    {
      while (wc->available() > 0)
        wc->read();
    }
    return true;
  }

  if (fwBuild <= 0)
  {
    if (secured)
    {
      if (!wcs)
        wcs = new WiFiSSLClient();

      //use the default SSL connection
      if (wcs->connect(host.c_str(), port) == 0)
        return false;
    }
    else
    {
      if (!wc)
        wc = new WiFiClient();

      //use the default TCP connection
      if (wc->connect(host.c_str(), port) == 0)
        return false;
    }
  }
  else
  {

    if (!wcs)
      wcs = new WiFiSSLClient();

    //use upgradable connection
    if (wcs->ns_connect(host.c_str(), port) == 0)
      return false;

    if (secured)
    {
      if (wcs->ns_connectSSL(host.c_str(), port, verify) == 0)
        return false;
    }
  }

  return connected();
}

bool WiFiNINA_TCP_Client::connectSSL(bool verify)
{

  if (fwBuild <= 0)
    return false;

  verifyRootCA = verify;

  //upgrade connection
  if (!wcs->ns_connectSSL(host.c_str(), port, verify))
    return false;

  secured = connected();
  return secured;
}

void WiFiNINA_TCP_Client::stop()
{
  if (connected())
  {
    if (fwBuild > 0 || secured)
      wcs->stop();
    else
      wc->stop();
  }
}

int WiFiNINA_TCP_Client::write(uint8_t *data, int len)
{
  
  if (!connect(secured, verifyRootCA))
    return TCP_CLIENT_ERROR_CONNECTION_REFUSED;

  int toSend = chunkSize;
  int sent = 0;
  while (sent < len)
  {
    if (sent + toSend > len)
      toSend = len - sent;

    if (fwBuild > 0 || secured)
    {
      if ((int)wcs->write(data + sent, toSend) != toSend)
        return TCP_CLIENT_ERROR_SEND_DATA_FAILED;
    }
    else
    {
      if ((int)wc->write(data + sent, toSend) != toSend)
        return TCP_CLIENT_ERROR_SEND_DATA_FAILED;
    }

    sent += toSend;
  }

  return len;
}

int WiFiNINA_TCP_Client::send(const char *data)
{
  return write((uint8_t *)data, strlen(data));
}

int WiFiNINA_TCP_Client::print(int data)
{
  char buf[64];
  memset(buf, 0, 64);
  sprintf(buf, "%d", data);
  int ret = send(buf);
  return ret;
}

int WiFiNINA_TCP_Client::print(const char *data)
{
  return send(data);
}

int WiFiNINA_TCP_Client::println(const char *data)
{
  int len = send(data);
  if (len < 0)
    return len;
  int sz = send("\r\n");
  if (sz < 0)
    return sz;
  return len + sz;
}

int WiFiNINA_TCP_Client::println(int data)
{
  char buf[64];
  memset(buf, 0, 64);
  sprintf(buf, "%d\r\n", data);
  int ret = send(buf);
  return ret;
}

int WiFiNINA_TCP_Client::available()
{
  if (fwBuild > 0 || secured)
  {
    if (!wcs)
      return TCP_CLIENT_ERROR_NOT_INITIALIZED;

    return wcs->available();
  }
  else
  {

    if (!wc)
      return TCP_CLIENT_ERROR_NOT_INITIALIZED;

    return wc->available();
  }

  return TCP_CLIENT_ERROR_NOT_INITIALIZED;
}

int WiFiNINA_TCP_Client::read()
{
  if (fwBuild > 0 || secured)
  {
    if (!wcs)
      return TCP_CLIENT_ERROR_NOT_INITIALIZED;

    return wcs->read();
  }
  else
  {
    if (!wc)
      return TCP_CLIENT_ERROR_NOT_INITIALIZED;

    return wc->read();
  }

  return TCP_CLIENT_ERROR_NOT_INITIALIZED;
}

int WiFiNINA_TCP_Client::readBytes(uint8_t *buf, int len)
{
  if (fwBuild > 0 || secured)
  {
    if (!wcs)
      return TCP_CLIENT_ERROR_NOT_INITIALIZED;

    return wcs->readBytes(buf, len);
  }
  else
  {
    if (!wc)
      return TCP_CLIENT_ERROR_NOT_INITIALIZED;

    return wc->readBytes(buf, len);
  }

  return TCP_CLIENT_ERROR_NOT_INITIALIZED;
}

int WiFiNINA_TCP_Client::readBytes(char *buf, int len)
{
  return readBytes((uint8_t *)buf, len);
}

#endif

#endif
